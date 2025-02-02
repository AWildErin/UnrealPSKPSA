// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/SPSKImportOption.h"
#include "SPrimaryButton.h"
#include "Widgets/PSKImportOptions.h"
#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPSKImportOption::Construct(const FArguments& InArgs)
{
	WidgetWindow = InArgs._WidgetWindow;
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	TSharedRef<IDetailsView> Details = EditModule.CreateDetailView(DetailsViewArgs);
	EditModule.CreatePropertyTable();
	UObject* Container = NewObject<UPSKImportOptions>();
	Stun = Cast<UPSKImportOptions>(Container);
	Details->SetObject(Container);
	Details->SetEnabled(true);

	this->ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush(TEXT("Brushes.Panel")))
		.Padding(10)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.Padding(FMargin(3))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		]
		]

	// Data row struct
	// Curve interpolation
	// Details panel
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SBox)
			.WidthOverride(400)
		[
			Details
		]
		]
	+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		    .HAlign(HAlign_Right)
		    .Padding(2)
		[
			SNew(SPrimaryButton)
			.Text(FText::FromString(TEXT("Apply")))
		    .OnClicked(this, &SPSKImportOption::OnImport)
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Apply to All")))
		.OnClicked(this, &SPSKImportOption::OnImportAll)
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Cancel")))
		.OnClicked(this, &SPSKImportOption::OnCancel)
		]
		]
		]
	// Apply/Apply to All/Cancel
		];


}
bool SPSKImportOption::ShouldImport()
{
	return UserDlgResponse == EPSKImportOptionDlgResponse::Import;
}
bool SPSKImportOption::ShouldImportAll()
{
	return UserDlgResponse == EPSKImportOptionDlgResponse::ImportAll;
}
bool SPSKImportOption::ShouldCancel()
{
	return UserDlgResponse == EPSKImportOptionDlgResponse::Cancel;
}
FReply SPSKImportOption::OnImportAll()
{
	UserDlgResponse = EPSKImportOptionDlgResponse::ImportAll;
	return HandleImport();
}
FReply SPSKImportOption::OnImport()
{
	UserDlgResponse = EPSKImportOptionDlgResponse::Import;
	return HandleImport();
}
FReply SPSKImportOption::OnCancel()
{
	UserDlgResponse = EPSKImportOptionDlgResponse::Cancel;
	return HandleImport();
}
FReply SPSKImportOption::HandleImport()
{
	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
