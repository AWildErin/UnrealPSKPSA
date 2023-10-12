// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "PSKImportOptions.generated.h"

/**
 *
 */
UCLASS(config = Engine, defaultconfig, transient)
class UNREALPSKPSA_API UPSKImportOptions : public UObject
{
	GENERATED_BODY()
public:
	UPSKImportOptions();

	UPROPERTY(EditAnywhere, Category = "Import Settings", meta = (ToolTip = "Whether or not to create the materials for the PSK model"))
	bool bCreateMaterials;

	UPROPERTY(EditAnywhere, Category = "Import Settings", meta = (ToolTip = "Whether or not to load the properties exported by UModel"))
	bool bLoadProperties;

	UPROPERTY(EditAnywhere, Category = "Import Settings|Properties|Skeletal Mesh", meta = (EditCondition = "bSkeletalMesh && bLoadProperties"))
	bool bCreateSockets;

	//UPROPERTY(EditAnywhere, Category = "Import Settings|Properties|Static Mesh", meta = (EditCondition = "!bSkeletalMesh && bLoadProperties", EditConditionHides))

	// Used to influence other properties, it has no other purpose for importing.
	UPROPERTY(BlueprintReadOnly, Category = "Import Settings", meta = (HideInDetailPanel))
	bool bSkeletalMesh;

	bool bInitialized;
};
