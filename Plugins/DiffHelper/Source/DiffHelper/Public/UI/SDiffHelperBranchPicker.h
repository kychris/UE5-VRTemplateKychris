﻿// Copyright 2024 Gradess Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DiffHelperTypes.h"
#include "SSearchableComboBox.h"

class UDiffHelperRevisionPickerController;

class DIFFHELPER_API SDiffHelperBranchPicker : public SSearchableComboBox
{
public:
	SLATE_BEGIN_ARGS(SDiffHelperBranchPicker)
			:
			_Controller(nullptr),
			_Options(nullptr),
			_Hint(FText::FromString("Select branch..."))
		{
		}

		SLATE_ARGUMENT(TWeakObjectPtr<UDiffHelperRevisionPickerController>, Controller)
		SLATE_ARGUMENT(TArray<TSharedPtr<FString>>*, Options)
		SLATE_ARGUMENT(TSharedPtr<FDiffHelperBranch>, InitiallySelectedBranch)
		SLATE_ARGUMENT(FText, Hint)

	SLATE_END_ARGS()

protected:
	TWeakObjectPtr<UDiffHelperRevisionPickerController> Controller = nullptr;
	TSharedPtr<const TArray<TSharedPtr<FString>>> Options = nullptr;
	
	FText Hint;
	FDiffHelperBranch SelectedBranch;

public:
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	const FDiffHelperBranch& GetSelectedBranch() const { return SelectedBranch; }

protected:
	TSharedRef<SWidget> HandleGenerateWidget(TSharedPtr<FString> String);
	void HandleSelectionChanged(TSharedPtr<FString, ESPMode::ThreadSafe> String, ESelectInfo::Type Arg);

	FText GetSelectedItemText() const;
};
