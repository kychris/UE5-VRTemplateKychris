﻿// Copyright 2024 Gradess Games. All Rights Reserved.


#include "UI/SDiffHelperPickerPanel.h"

#include "DiffHelperSettings.h"

#include "UI/SDiffHelperBranchPicker.h"
#include "SlateOptMacros.h"

#include "UI/DiffHelperRevisionPickerController.h"
#include "UI/DiffHelperRevisionPickerModel.h"
#include "UI/SDiffHelperDiffViewer.h"

#define LOCTEXT_NAMESPACE "DiffHelper"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SDiffHelperPickerPanel::~SDiffHelperPickerPanel()
{
	if (Controller.IsValid())
	{
		Controller->Deinit();
		Controller.Reset();
	}
}

void SDiffHelperPickerPanel::Construct(const FArguments& InArgs)
{
	Controller = NewObject<UDiffHelperRevisionPickerController>();
	Controller->Init();

	OnShowDiff = InArgs._OnShowDiff;

	TSharedPtr<FDiffHelperBranch> CachedSourceBranch = MakeShareable<FDiffHelperBranch>(new FDiffHelperBranch(Controller->GetModel()->SourceBranch));
	TSharedPtr<FDiffHelperBranch> CachedTargetBranch = MakeShareable<FDiffHelperBranch>(new FDiffHelperBranch(Controller->GetModel()->TargetBranch));

	const auto* Settings = GetDefault<UDiffHelperSettings>();
	
	ChildSlot
	[
		SNew(SBorder)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SBox)
			.WidthOverride(Settings->PickerPanelWidth)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SAssignNew(SourceBranchPicker, SDiffHelperBranchPicker)
					.Controller(Controller)
					.InitiallySelectedBranch(CachedSourceBranch)
					.Hint(LOCTEXT("DiffHelperSourceBranchHint", "Select source branch..."))
				]
				+ SVerticalBox::Slot()
				[
					SAssignNew(TargetBranchPicker, SDiffHelperBranchPicker)
					.Controller(Controller)
					.InitiallySelectedBranch(CachedTargetBranch)
					.Hint(LOCTEXT("DiffHelperTargetBranchHint", "Select target branch..."))
				]
				// add "Show diff" button here
				+ SVerticalBox::Slot()
				[
					SNew(SButton)
					.OnClicked(this, &SDiffHelperPickerPanel::OnShowDiffClicked)
					.IsEnabled(this, &SDiffHelperPickerPanel::CanShowDiff)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DiffHelperShowDiffButton", "Show diff"))
						.Justification(ETextJustify::Center)
					]
				]
			]
		]
	];
}

FReply SDiffHelperPickerPanel::OnShowDiffClicked() const
{
	const auto& SourceBranch = SourceBranchPicker->GetSelectedBranch();
	const auto& TargetBranch = TargetBranchPicker->GetSelectedBranch();

	Controller->SetSourceBranch(SourceBranch);
	Controller->SetTargetBranch(TargetBranch);
	Controller->OpenDiffTab();

	OnShowDiff.ExecuteIfBound();
	
	return FReply::Handled();
}

bool SDiffHelperPickerPanel::CanShowDiff() const
{
	const auto& SourceBranch = SourceBranchPicker->GetSelectedBranch();
	const auto& TargetBranch = TargetBranchPicker->GetSelectedBranch();

	return SourceBranch.IsValid() && TargetBranch.IsValid() && SourceBranch != TargetBranch;
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
