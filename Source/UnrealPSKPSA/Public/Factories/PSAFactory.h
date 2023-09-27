// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Widgets/PSAImportOptions.h"
#include "PSAFactory.generated.h"

/**
 * Implements a factory for UnrealPSKPSA animation objects.
 */
 
UCLASS(hidecategories=Object)
class UNREALPSKPSA_API UPSAFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY()
	UPSAImportOptions* SettingsImporter;

	bool bImport;
	bool bImportAll;
	bool bCancel;

	virtual UObject* FactoryCreateFile(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, const FString& Filename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
};
