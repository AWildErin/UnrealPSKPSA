// Fill out your copyright notice in the Description page of Project Settings.

#include "Factories/PSKFactory.h"
#include "Utils/ActorXUtils.h"
#include "IMeshBuilderModule.h"
#include "Readers/PSKReader.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Widgets/PSKImportOptions.h"
#include "Widgets/SPSKImportOption.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"

/* UTextAssetFactory structors
 *****************************************************************************/

UPSKFactory::UPSKFactory( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	Formats.Add(TEXT("psk;PSK Skeletal Mesh File"));
	SupportedClass = USkeletalMesh::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
	SettingsImporter = CreateDefaultSubobject<UPSKImportOptions>(TEXT("Model Options"));
}

/* UFactory overrides
 *****************************************************************************/

UObject* UPSKFactory::FactoryCreateFile(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, const FString& Filename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	FScopedSlowTask SlowTask(5, NSLOCTEXT("PSKFactory", "BeginReadPSKFile", "Opening PSK file."), true);
	if (Warn->GetScopeStack().Num() == 0)
	{
		// We only display this slow task if there is no parent slowtask, because otherwise it is redundant and doesn't display any relevant information on the
		// progress. It is primarly used to regroup all the smaller import sub-tasks for a smoother progression.
		SlowTask.MakeDialog(true);
	}
	SlowTask.EnterProgressFrame(0);

	// picker
	if (SettingsImporter->bInitialized == false)
	{
		TSharedPtr<SPSKImportOption> ImportOptionsWindow;
		TSharedPtr<SWindow> ParentWindow;
		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("PSK Import Options"))).SizingRule(ESizingRule::Autosized);
		Window->SetContent(SAssignNew(ImportOptionsWindow, SPSKImportOption).WidgetWindow(Window));
		SettingsImporter = ImportOptionsWindow.Get()->Stun;

		// Mark us as a skeletal mesh
		SettingsImporter->bSkeletalMesh = true;

		FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
		bImport = ImportOptionsWindow.Get()->ShouldImport();
		bImportAll = ImportOptionsWindow.Get()->ShouldImportAll();
		bCancel = ImportOptionsWindow.Get()->ShouldCancel();
		SettingsImporter->bInitialized = true;
	}

	// Cancel out the import
	if (bCancel)
	{
		return nullptr;
	}

	auto Data = PSKReader(Filename, SettingsImporter->bLoadProperties);
	if (!Data.Read()) return nullptr;

	TArray<FColor> VertexColorsByPoint;
	VertexColorsByPoint.Init(FColor::Black, Data.VertexColors.Num());
	if (Data.bHasVertexColors)
	{
		for (auto i = 0; i < Data.Wedges.Num(); i++)
		{
			auto FixedColor = Data.VertexColors[i];
			Swap(FixedColor.R, FixedColor.B);
			VertexColorsByPoint[Data.Wedges[i].PointIndex] = FixedColor;
		}
	}
	
	FSkeletalMeshImportData SkeletalMeshImportData;

	for (auto i = 0; i < Data.Normals.Num(); i++)
	{
		Data.Normals[i].Y = -Data.Normals[i].Y; // MIRROR_MESH
	}

	for (auto Vertex : Data.Vertices)
	{
		auto FixedVertex = Vertex;
		FixedVertex.Y = -FixedVertex.Y; // MIRROR_MESH
		SkeletalMeshImportData.Points.Add(FixedVertex);
		SkeletalMeshImportData.PointToRawMap.Add(SkeletalMeshImportData.Points.Num()-1);
	}
	
	auto WindingOrder = {2, 1, 0};
	for (const auto PskFace : Data.Faces)
	{
		SkeletalMeshImportData::FTriangle Face;
		Face.MatIndex = PskFace.MatIndex;
		Face.SmoothingGroups = 1;
		Face.AuxMatIndex = 0;

		for (auto VertexIndex : WindingOrder)
		{
			const auto WedgeIndex = PskFace.WedgeIndex[VertexIndex];
			const auto PskWedge = Data.Wedges[WedgeIndex];
			
			SkeletalMeshImportData::FVertex Wedge;
			Wedge.MatIndex = PskWedge.MatIndex;
			Wedge.VertexIndex = PskWedge.PointIndex;
			Wedge.Color = Data.bHasVertexColors ? VertexColorsByPoint[PskWedge.PointIndex] : FColor::Black;
			Wedge.UVs[0] = FVector2f(PskWedge.U, PskWedge.V);
			for (auto UVIdx = 0; UVIdx < Data.ExtraUVs.Num(); UVIdx++)
			{
				auto UV =  Data.ExtraUVs[UVIdx][PskFace.WedgeIndex[VertexIndex]];
				Wedge.UVs[UVIdx+1] = UV;
			}
			
			Face.WedgeIndex[VertexIndex] = SkeletalMeshImportData.Wedges.Add(Wedge);
			Face.TangentZ[VertexIndex] = Data.bHasVertexNormals ? Data.Normals[PskWedge.PointIndex] : FVector3f::ZeroVector;
			Face.TangentY[VertexIndex] = FVector3f::ZeroVector;
			Face.TangentX[VertexIndex] = FVector3f::ZeroVector;
		}
		Swap(Face.WedgeIndex[0], Face.WedgeIndex[2]);
		Swap(Face.TangentZ[0], Face.TangentZ[2]);

		SkeletalMeshImportData.Faces.Add(Face);
	}

	TArray<FString> AddedBoneNames;
	for (auto i = 0; i < Data.Bones.Num(); i++)
	{
		SkeletalMeshImportData::FBone Bone;
		Bone.Name = Data.Bones[i].Name;
		if (AddedBoneNames.Contains(Bone.Name)) continue;
		AddedBoneNames.Add(Bone.Name);
		
		Bone.NumChildren = Data.Bones[i].NumChildren;
		
		Bone.ParentIndex = (i > 0) ? Data.Bones[i].ParentIndex : INDEX_NONE;
		auto PskBonePos = Data.Bones[i].BonePos;
		
		FTransform3f PskTransform;
		PskTransform.SetLocation(FVector3f(PskBonePos.Position.X, -PskBonePos.Position.Y, PskBonePos.Position.Z));
		PskTransform.SetRotation(FQuat4f(-PskBonePos.Orientation.X, PskBonePos.Orientation.Y, -PskBonePos.Orientation.Z, (Bone.ParentIndex == INDEX_NONE) ? PskBonePos.Orientation.W : -PskBonePos.Orientation.W).GetNormalized());
		
		SkeletalMeshImportData::FJointPos BonePos;
		BonePos.Transform = PskTransform;
		BonePos.Length = PskBonePos.Length;
		BonePos.XSize = PskBonePos.XSize;
		BonePos.YSize = PskBonePos.YSize;
		BonePos.ZSize = PskBonePos.ZSize;

		Bone.BonePos = BonePos;
		SkeletalMeshImportData.RefBonesBinary.Add(Bone);
		AddedBoneNames.Add(Bone.Name);
	}

	for (auto PskInfluence : Data.Influences)
	{
		SkeletalMeshImportData::FRawBoneInfluence Influence;
		Influence.BoneIndex = PskInfluence.BoneIdx;
		Influence.VertexIndex = PskInfluence.PointIdx;
		Influence.Weight = PskInfluence.Weight;
		SkeletalMeshImportData.Influences.Add(Influence);
	}

	for (auto PskMaterial : Data.Materials)
	{
		SkeletalMeshImportData::FMaterial Material;
		Material.MaterialImportName = PskMaterial.MaterialName;

		if (SettingsImporter->bCreateMaterials)
		{
			auto MaterialAdd = FActorXUtils::LocalFindOrCreate<UMaterial>(UMaterial::StaticClass(), Parent, PskMaterial.MaterialName, Flags);
			Material.Material = MaterialAdd;
		}
		else
		{
			Material.Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		SkeletalMeshImportData.Materials.Add(Material);
	}
	
	SkeletalMeshImportData.MaxMaterialIndex = SkeletalMeshImportData.Materials.Num()-1;

	SkeletalMeshImportData.bDiffPose = false;
	SkeletalMeshImportData.bHasNormals = Data.bHasVertexNormals;
	SkeletalMeshImportData.bHasTangents = false;
	SkeletalMeshImportData.bHasVertexColors = true;
	SkeletalMeshImportData.NumTexCoords = 1 + Data.ExtraUVs.Num(); 
	SkeletalMeshImportData.bUseT0AsRefPose = false;
	
	const auto Skeleton = FActorXUtils::LocalCreate<USkeleton>(USkeleton::StaticClass(), Parent,  Name.ToString().Append("_Skeleton"), Flags);

	FReferenceSkeleton RefSkeleton;
	auto SkeletalDepth = 0;
	ProcessSkeleton(SkeletalMeshImportData, Skeleton, RefSkeleton, SkeletalDepth);

	TArray<FVector3f> LODPoints;
	TArray<SkeletalMeshImportData::FMeshWedge> LODWedges;
	TArray<SkeletalMeshImportData::FMeshFace> LODFaces;
	TArray<SkeletalMeshImportData::FVertInfluence> LODInfluences;
	TArray<int32> LODPointToRawMap;
	SkeletalMeshImportData.CopyLODImportData(LODPoints, LODWedges, LODFaces, LODInfluences, LODPointToRawMap);

	FSkeletalMeshLODModel LODModel;
	LODModel.NumTexCoords = FMath::Max<uint32>(1, SkeletalMeshImportData.NumTexCoords);
	
	const auto SkeletalMesh = FActorXUtils::LocalCreate<USkeletalMesh>(USkeletalMesh::StaticClass(), Parent, Name.ToString(), Flags);
	SkeletalMesh->PreEditChange(nullptr);
	SkeletalMesh->InvalidateDeriveDataCacheGUID();
	SkeletalMesh->UnregisterAllMorphTarget();

	SkeletalMesh->GetRefBasesInvMatrix().Empty();
	SkeletalMesh->GetMaterials().Empty();
	SkeletalMesh->SetHasVertexColors(true);

	FSkeletalMeshModel* ImportedResource = SkeletalMesh->GetImportedModel();
	auto& SkeletalMeshLODInfos = SkeletalMesh->GetLODInfoArray();
	SkeletalMeshLODInfos.Empty();
	SkeletalMeshLODInfos.Add(FSkeletalMeshLODInfo());
	SkeletalMeshLODInfos[0].ReductionSettings.NumOfTrianglesPercentage = 1.0f;
	SkeletalMeshLODInfos[0].ReductionSettings.NumOfVertPercentage = 1.0f;
	SkeletalMeshLODInfos[0].ReductionSettings.MaxDeviationPercentage = 0.0f;
	SkeletalMeshLODInfos[0].LODHysteresis = 0.02f;

	ImportedResource->LODModels.Empty();
	ImportedResource->LODModels.Add(new FSkeletalMeshLODModel);
	SkeletalMesh->SetRefSkeleton(RefSkeleton);
	SkeletalMesh->CalculateInvRefMatrices();

	SkeletalMesh->SaveLODImportedData(0, SkeletalMeshImportData);
	FSkeletalMeshBuildSettings BuildOptions;
	BuildOptions.bRemoveDegenerates = false;
	BuildOptions.bRecomputeNormals = !Data.bHasVertexNormals;
	BuildOptions.bRecomputeTangents = true;
	BuildOptions.bUseMikkTSpace = true;
	SkeletalMesh->GetLODInfo(0)->BuildSettings = BuildOptions;
	SkeletalMesh->SetImportedBounds(FBoxSphereBounds(FBoxSphereBounds3f(FBox3f(SkeletalMeshImportData.Points))));

	auto& MeshBuilderModule = IMeshBuilderModule::GetForRunningPlatform();
	const FSkeletalMeshBuildParameters SkeletalMeshBuildParameters(SkeletalMesh, GetTargetPlatformManagerRef().GetRunningTargetPlatform(), 0, false);
	if (!MeshBuilderModule.BuildSkeletalMesh(SkeletalMeshBuildParameters))
	{
		SkeletalMesh->MarkAsGarbage();
		return nullptr;
	}

	for (auto Material : SkeletalMeshImportData.Materials)
	{
		FSkeletalMaterial SkelMat(Material.Material.Get());
		SkelMat.MaterialSlotName = FName(Material.MaterialImportName);

		SkeletalMesh->GetMaterials().Add(SkelMat);
	}

	// Assign sockets to the model
	for (auto Socket : Data.Sockets)
	{
		USkeletalMeshSocket* NewSocket = NewObject<USkeletalMeshSocket>(SkeletalMesh);
		NewSocket->SocketName = FName(Socket.SocketName);
		NewSocket->BoneName = FName(Socket.BoneName);
		NewSocket->RelativeLocation = Socket.RelativeLocation;
		NewSocket->RelativeRotation = Socket.RelativeRotation;
		NewSocket->RelativeScale = Socket.RelativeScale;

		SkeletalMesh->AddSocket(NewSocket);
	}

	// morphdata here
	
	SkeletalMesh->PostEditChange();
	
	SkeletalMesh->SetSkeleton(Skeleton);
	Skeleton->MergeAllBonesToBoneTree(SkeletalMesh);
	
	FAssetRegistryModule::AssetCreated(SkeletalMesh);
	SkeletalMesh->MarkPackageDirty();

	Skeleton->PostEditChange();
	FAssetRegistryModule::AssetCreated(Skeleton);
	Skeleton->MarkPackageDirty();

	if (!bImportAll)
	{
		SettingsImporter->bInitialized = false;
	}

	FGlobalComponentReregisterContext RecreateComponents;

	return SkeletalMesh;
}

void UPSKFactory::ProcessSkeleton(const FSkeletalMeshImportData& ImportData, const USkeleton* Skeleton, FReferenceSkeleton& OutRefSkeleton, int& OutSkeletalDepth)
{
	const auto RefBonesBinary = ImportData.RefBonesBinary;
	OutRefSkeleton.Empty();
	
	FReferenceSkeletonModifier RefSkeletonModifier(OutRefSkeleton, Skeleton);
	
	for (const auto Bone : RefBonesBinary)
	{
		const FMeshBoneInfo BoneInfo(FName(*Bone.Name), Bone.Name, Bone.ParentIndex);
		RefSkeletonModifier.Add(BoneInfo, FTransform(Bone.BonePos.Transform));
	}

    OutSkeletalDepth = 0;

    TArray<int> SkeletalDepths;
    SkeletalDepths.Empty(ImportData.RefBonesBinary.Num());
    SkeletalDepths.AddZeroed(ImportData.RefBonesBinary.Num());
    for (auto b = 0; b < OutRefSkeleton.GetNum(); b++)
    {
        const auto Parent = OutRefSkeleton.GetParentIndex(b);
        auto Depth  = 1.0f;

        SkeletalDepths[b] = 1.0f;
        if (Parent != INDEX_NONE)
        {
            Depth += SkeletalDepths[Parent];
        }
        if (OutSkeletalDepth < Depth)
        {
            OutSkeletalDepth = Depth;
        }
        SkeletalDepths[b] = Depth;
    }
}
