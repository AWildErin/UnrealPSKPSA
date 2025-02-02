#pragma once
#include "AssetRegistry/AssetRegistryModule.h"

class FActorXUtils
{
public:
	
	template <typename T>
	static T* LocalFindOrCreate(UClass* StaticClass, UObject* FactoryParent, FString Filename, EObjectFlags Flags)
	{
		auto Package = CreatePackage(*FPaths::Combine(FPaths::GetPath(FactoryParent->GetPathName()), Filename));
		
		auto Asset = LoadObject<T>(Package, *Filename);
		if (!Asset)
		{
			Asset = NewObject<T>(Package, StaticClass, FName(Filename), Flags);
			Asset->PostEditChange();
			FAssetRegistryModule::AssetCreated(Asset);
			Asset->MarkPackageDirty();
		}

		return Asset;
	}

	template <typename T>
	static T* LocalCreate(UClass* StaticClass, UObject* FactoryParent, FString Filename, EObjectFlags Flags, bool bCreateFolder = false)
	{
		FString Path = FPaths::GetPath(FactoryParent->GetPathName());

		// Append our package name if needed, here for animsets with multiple sequences
		if (bCreateFolder)
		{
			Path = FactoryParent->GetName();
		}

		auto Package = CreatePackage(*FPaths::Combine(Path, Filename));
		
		auto Asset = NewObject<T>(Package, StaticClass, FName(Filename), Flags);
		return Asset;
	}
};
