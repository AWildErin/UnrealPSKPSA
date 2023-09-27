// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 *
 *
 */

enum class EPSKImportOptionDlgResponse : uint8
{
	Import,
	ImportAll,
	Cancel
};

class UPSKImportOptions;
class UNREALPSKPSA_API SPSKImportOption : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPSKImportOption) : _WidgetWindow() {}

	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_END_ARGS()

	SPSKImportOption() : UserDlgResponse(EPSKImportOptionDlgResponse::Cancel) {}
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	/** A property view to edit advanced options */
	TSharedPtr<class IDetailsView> PropertyView;

	UPROPERTY(Category = MapsAndSets, EditAnywhere)
	mutable UPSKImportOptions* Stun;

	bool bShouldImportAll;
	/** If we should import */
	bool ShouldImport();

	/** If the current settings should be applied to all items being imported */
	bool ShouldImportAll();

	/** Did we cancel */
	bool ShouldCancel();

	FReply OnImportAll();

	/** Called when 'Apply' button is pressed */
	FReply OnImport();

	FReply OnCancel();

private:
	EPSKImportOptionDlgResponse UserDlgResponse;
	FReply HandleImport();

	/** Window that owns us */
	TWeakPtr<SWindow> WidgetWindow;
};
