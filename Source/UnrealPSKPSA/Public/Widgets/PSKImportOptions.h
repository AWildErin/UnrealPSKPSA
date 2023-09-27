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

	bool bInitialized;
};
