﻿// Copyright 2024 Gradess Games. All Rights Reserved.


#include "UI/DiffHelperTabController.h"

#include "AssetToolsModule.h"
#include "Misc/ComparisonUtility.h"
#include "DiffHelper.h"
#include "DiffHelperCommands.h"
#include "DiffHelperManager.h"
#include "DiffHelperSettings.h"
#include "DiffHelperUtils.h"
#include "DiffUtils.h"
#include "EditorAssetLibrary.h"

#include "UI/DiffHelperTabModel.h"

#define LOCTEXT_NAMESPACE "DiffHelperTabController"

void UDiffHelperTabController::Init()
{
	AddToRoot();
	InitModel();

	BindMenuCommands();
	BindDiffPanelCommands();
	BindCommitPanelCommands();
}

void UDiffHelperTabController::Reset()
{
	InitModel();
	OnModelReset.Broadcast();
}

void UDiffHelperTabController::Deinit()
{
	RemoveFromRoot();
	Model = nullptr;
}

void UDiffHelperTabController::SetSourceBranch(const FDiffHelperBranch& InBranch)
{
	Model->SourceBranch = InBranch;
}

void UDiffHelperTabController::SetTargetBranch(const FDiffHelperBranch& InBranch)
{
	Model->TargetBranch = InBranch;
}

void UDiffHelperTabController::SelectDiffItem(const FDiffHelperDiffItem& InDiffItem)
{
	Model->SelectedDiffItem = InDiffItem;
}

void UDiffHelperTabController::CollectDiff()
{
	const auto Manager = FDiffHelperModule::Get().GetManager();
	auto Diff = Manager->GetDiff(Model->SourceBranch, Model->TargetBranch);
	Diff.Sort([](const FDiffHelperDiffItem& A, const FDiffHelperDiffItem& B)
	{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3
		return UE::ComparisonUtility::CompareNaturalOrder(A.Path, B.Path) < 0;
#else
		return A.Path.Compare(B.Path, ESearchCase::IgnoreCase) < 0;
#endif
	});

	Model->Diff = Diff;

	Model->DiffPanelData.OriginalDiff = UDiffHelperUtils::GenerateList(Model->Diff);
	UDiffHelperUtils::SortDiffList(Model->DiffPanelData.SortMode, Model->DiffPanelData.OriginalDiff);
	Model->DiffPanelData.FilteredDiff = Model->DiffPanelData.OriginalDiff;
	
	Model->DiffPanelData.TreeDiff = UDiffHelperUtils::GenerateTree(Model->Diff);
	UDiffHelperUtils::SortDiffTree(Model->DiffPanelData.SortMode, Model->DiffPanelData.TreeDiff);

	Model->DiffPanelData.SearchFilter = MakeShared<TTextFilter<const FDiffHelperDiffItem&>>(TTextFilter<const FDiffHelperDiffItem&>::FItemToStringArray::CreateUObject(this, &UDiffHelperTabController::PopulateFilterSearchString));
}

void UDiffHelperTabController::DiffAsset(const FString& InPath, const FDiffHelperCommit& InFirstRevision, const FDiffHelperCommit& InSecondRevision) const
{
	TObjectPtr<UObject> LeftDiffAsset = nullptr;
	TObjectPtr<UObject> RightDiffAsset = nullptr;

	FRevisionInfo LeftVersionInfo;
	LeftVersionInfo.Revision = InFirstRevision.Revision;
	LeftVersionInfo.Date = InFirstRevision.Date;
	// TODO: What is Changelist here? For another VCS? Need research.
	// LeftVersionInfo.Changelist = InFirstRevision.Changelist;

	FRevisionInfo RightVersionInfo;
	RightVersionInfo.Revision = InSecondRevision.Revision;
	RightVersionInfo.Date = InSecondRevision.Date;

	const auto Manager = FDiffHelperModule::Get().GetManager();
	const auto LeftTempFilename = Manager->GetFile(InPath, InFirstRevision.Revision);
	const auto RightTempFilename = Manager->GetFile(InPath, InSecondRevision.Revision);

	if (!LeftTempFilename.IsSet() || !RightTempFilename.IsSet())
	{
		UE_LOG(LogDiffHelper, Error, TEXT("Failed to get diff file for %s"), *InPath);
		return;
	}

	const auto LeftPackagePath = FPackagePath::FromLocalPath(LeftTempFilename.GetValue());
	const auto RightPackagePath = FPackagePath::FromLocalPath(RightTempFilename.GetValue());
	const auto OriginalPackagePath = FPackagePath::FromLocalPath(InPath);

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3
	const auto LeftDiffPackage = DiffUtils::LoadPackageForDiff(LeftPackagePath, OriginalPackagePath);
	const auto RightDiffPackage = DiffUtils::LoadPackageForDiff(RightPackagePath, OriginalPackagePath);
#else
	UPackage* LeftDiffPackage = LoadPackage(NULL, LeftPackagePath, LOAD_ForDiff | LOAD_DisableCompileOnLoad);
	UPackage* RightDiffPackage = LoadPackage(NULL, RightPackagePath, LOAD_ForDiff | LOAD_DisableCompileOnLoad);

	if (!LeftDiffPackage || !RightDiffPackage)
	{
		UE_LOG(LogDiffHelper, Error, TEXT("Failed to load diff package for %s"), *InPath);
		return;
	}
#endif
	

	FString AssetName = FPaths::GetBaseFilename(InPath);
	LeftDiffAsset = FindObject<UObject>(LeftDiffPackage, *AssetName);
	RightDiffAsset = FindObject<UObject>(RightDiffPackage, *AssetName);
	
	if (LeftDiffPackage && !LeftDiffAsset)
	{
		LeftDiffAsset = LeftDiffPackage->FindAssetInPackage();
	}

	if (RightDiffPackage && !RightDiffAsset)
	{
		RightDiffAsset = RightDiffPackage->FindAssetInPackage();
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	if (IsValid(LeftDiffAsset) && IsValid(RightDiffAsset))
	{
		AssetToolsModule.Get().DiffAssets(LeftDiffAsset, RightDiffAsset, LeftVersionInfo, RightVersionInfo);
	}
}

void UDiffHelperTabController::CallModelUpdated() const
{
	Model->OnModelUpdated_Raw.Broadcast();
}

void UDiffHelperTabController::SetSearchFilter(const FText& InText) const
{
	Model->DiffPanelData.SearchFilter->SetRawFilterText(InText);
}

void UDiffHelperTabController::SetSortingMode(const FName& InColumnId, EColumnSortMode::Type InSortMode) const
{
	auto& Data = Model->DiffPanelData;
	Data.SortMode = InSortMode;

	UDiffHelperUtils::SortDiffList(Data.SortMode, Data.FilteredDiff);
	UDiffHelperUtils::SortDiffTree(Data.SortMode, Data.TreeDiff);

	CallModelUpdated();
}

void UDiffHelperTabController::SetActiveWidgetIndex(const int32& InIndex) const
{
	Model->DiffPanelData.CurrentWidgetIndex = InIndex;

	CallModelUpdated();
}

void UDiffHelperTabController::SetSelectedCommits(const TArray<TSharedPtr<FDiffHelperCommit>>& InCommits) const
{
	Model->CommitPanelData.SelectedCommits = InCommits;
}

void UDiffHelperTabController::SelectNode(const TSharedPtr<FDiffHelperItemNode>& InNode) const
{
	Model->DiffPanelData.SelectedNode = InNode;
}

int32 UDiffHelperTabController::GetCommitIndex(const FDiffHelperCommit& InCommit) const
{
	const auto& Commits = Model->SelectedDiffItem.Commits;
	return Commits.IndexOfByPredicate([&InCommit](const FDiffHelperCommit& Commit)
	{
		return Commit.Revision == InCommit.Revision;
	});
}

void UDiffHelperTabController::InitModel()
{
	Model = NewObject<UDiffHelperTabModel>(this);
	Model->OnModelUpdated_Raw.AddWeakLambda(this, [this]() { Model->OnModelUpdated.Broadcast(); });
}

void UDiffHelperTabController::BindMenuCommands()
{
	const auto& Commands = FDiffHelperCommands::Get();

	MenuCommands = MakeShared<FUICommandList>();

	MenuCommands->MapAction(
		Commands.CreateNewDiff,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::Reset)
	);
}

void UDiffHelperTabController::BindDiffPanelCommands()
{
	const auto& Commands = FDiffHelperCommands::Get();
	DiffPanelCommands = MakeShared<FUICommandList>();

	DiffPanelCommands->MapAction(
		Commands.GroupByDirectory,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::ToggleGroupByDirectory),
		FCanExecuteAction::CreateLambda([]() { return true; }),
		FIsActionChecked::CreateUObject(this, &UDiffHelperTabController::IsTreeView)
	);

	DiffPanelCommands->MapAction(
		Commands.ExpandAll,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::ExpandAll),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::IsTreeView),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateUObject(this, &UDiffHelperTabController::IsTreeView)
	);

	DiffPanelCommands->MapAction(
		Commands.CollapseAll,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::CollapseAll),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::IsTreeView),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateUObject(this, &UDiffHelperTabController::IsTreeView)
	);

	DiffPanelCommands->MapAction(
		Commands.OpenLocation,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::OpenLocation),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanOpenLocation)
	);

	DiffPanelCommands->MapAction(
		Commands.OpenAsset,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::OpenAsset),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanOpenAsset)
	);

	DiffPanelCommands->MapAction(
		Commands.ShowInContentBrowser,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::ShowInContentBrowser),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanShowInContentBrowser)
	);

	DiffPanelCommands->MapAction(
		Commands.DiffAgainstTarget,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::DiffAgainstTarget),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanDiffAgainstTarget)
	);
}

void UDiffHelperTabController::BindCommitPanelCommands()
{
	const auto& Commands = FDiffHelperCommands::Get();
	CommitPanelCommands = MakeShared<FUICommandList>();

	CommitPanelCommands->MapAction(
		Commands.DiffAgainstTarget,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::DiffAgainstTarget),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanDiffAgainstTarget)
    );
		
	CommitPanelCommands->MapAction(
		Commands.DiffSelectedCommits,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::DiffSelectedCommits),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanDiffSelectedCommits)
	);

	CommitPanelCommands->MapAction(
		Commands.DiffSelectedCommitAgainstNext,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::DiffSelectedCommitAgainstNext),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanDiffSelectedCommitAgainstNext)
	);

	CommitPanelCommands->MapAction(
		Commands.DiffSelectedCommitAgainstPrevious,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::DiffSelectedCommitAgainstPrevious),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanDiffSelectedCommitAgainstPrevious)
	);

	CommitPanelCommands->MapAction(
		Commands.DiffSelectedCommitAgainstNewest,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::DiffSelectedCommitAgainstNewest),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanDiffSelectedCommitAgainstNext)
	);

	CommitPanelCommands->MapAction(
		Commands.DiffSelectedCommitAgainstOldest,
		FExecuteAction::CreateUObject(this, &UDiffHelperTabController::DiffSelectedCommitAgainstOldest),
		FCanExecuteAction::CreateUObject(this, &UDiffHelperTabController::CanDiffSelectedCommitAgainstPrevious)
	);
}

FDiffHelperSimpleDelegate& UDiffHelperTabController::OnModelUpdated() const
{
	return Model->OnModelUpdated_Raw;
}

FDiffHelperSimpleDelegate& UDiffHelperTabController::OnPreWidgetIndexChanged() const
{
	return Model->DiffPanelData.OnPreWidgetIndexChanged;
}

FDiffHelperSimpleDelegate& UDiffHelperTabController::OnTreeDiffExpansionUpdated() const
{
	return Model->DiffPanelData.OnTreeDiffExpansionUpdated;
}

void UDiffHelperTabController::ToggleGroupByDirectory()
{
	OnPreWidgetIndexChanged().Broadcast();
	
	auto& Data = Model->DiffPanelData;
	Data.CurrentWidgetIndex = Data.CurrentWidgetIndex == SDiffHelperDiffPanelConstants::ListWidgetIndex ? SDiffHelperDiffPanelConstants::TreeWidgetIndex : SDiffHelperDiffPanelConstants::ListWidgetIndex;

	CallModelUpdated();
}

void UDiffHelperTabController::ExpandAll()
{
	UDiffHelperUtils::ExpandAll(Model->DiffPanelData.TreeDiff);
	OnTreeDiffExpansionUpdated().Broadcast();
}

void UDiffHelperTabController::CollapseAll()
{
	UDiffHelperUtils::CollapseAll(Model->DiffPanelData.TreeDiff);
	OnTreeDiffExpansionUpdated().Broadcast();
}

void UDiffHelperTabController::OpenLocation()
{
	const auto& Item = Model->DiffPanelData.SelectedNode;
	if (!ensure(Item.IsValid()) || !ensure(Item->IsValid()))
	{
		return;
	}

	const auto Path = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::Combine(FPaths::ProjectDir(), Item->Path));

	if (!FPaths::DirectoryExists(Path) && !FPaths::FileExists(Path))
	{
		UDiffHelperUtils::AddErrorNotification(FText::Format(LOCTEXT("PathNotFound", "Path {0} does not exist on disk"), FText::FromString(Path)));
		return;
	}
	
	FPlatformProcess::ExploreFolder(*Path);
}

void UDiffHelperTabController::OpenAsset()
{
	const auto& AssetData = Model->SelectedDiffItem.AssetData;
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetData.GetAsset());
}

void UDiffHelperTabController::ShowInContentBrowser()
{
	const auto& AssetData = Model->SelectedDiffItem.AssetData;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3
	GEditor->SyncBrowserToObject(AssetData.GetAsset(), false);
#else
	TArray<FAssetData> Assets = { AssetData };
	GEditor->SyncBrowserToObjects(Assets);
#endif
}

bool UDiffHelperTabController::CanOpenAsset()
{
	return Model->SelectedDiffItem.AssetData.IsValid();
}

bool UDiffHelperTabController::CanShowInContentBrowser()
{
	return Model->SelectedDiffItem.AssetData.IsValid();
}

bool UDiffHelperTabController::IsTreeView()
{
	return Model->DiffPanelData.CurrentWidgetIndex == SDiffHelperDiffPanelConstants::TreeWidgetIndex;
}

bool UDiffHelperTabController::CanOpenLocation()
{
	return Model->DiffPanelData.SelectedNode.IsValid() && Model->DiffPanelData.SelectedNode->IsValid();
}

void UDiffHelperTabController::ExecuteDiff(const TArray<TSharedPtr<FDiffHelperCommit>>& InCommits, const FString& InPath) const
{
	if (UDiffHelperUtils::IsDiffAvailable(InCommits, InPath))
	{
		if (UDiffHelperUtils::IsUnrealAsset(InPath))
		{
			DiffAsset(InPath, *InCommits[0], *InCommits[1]);
		}
		else
		{
			UDiffHelperUtils::DiffFileExternal(InPath, *InCommits[0], *InCommits[1]);
		}
	}
	else
	{
		UDiffHelperUtils::ShowDiffUnavailableDialog(InCommits, InPath);
	}
}

void UDiffHelperTabController::DiffAgainstTarget()
{
	const auto& DiffItem = Model->SelectedDiffItem;
	const auto CommitsToDiff = TArray<TSharedPtr<FDiffHelperCommit>>({MakeShared<FDiffHelperCommit>(DiffItem.LastTargetCommit), MakeShared<FDiffHelperCommit>(DiffItem.Commits[0])});

	ExecuteDiff(CommitsToDiff, DiffItem.Path);
}

void UDiffHelperTabController::DiffSelectedCommits()
{
	const auto& SelectedCommits = Model->CommitPanelData.SelectedCommits;
	const auto& DiffItem = Model->SelectedDiffItem;

	ExecuteDiff(SelectedCommits, DiffItem.Path);
}

void UDiffHelperTabController::DiffSelectedCommitAgainstNext()
{
	const auto& SelectedCommits = Model->CommitPanelData.SelectedCommits;
	const auto& DiffItem = Model->SelectedDiffItem;

	const auto Index = GetCommitIndex(*SelectedCommits[0]);
	const auto CommitsToDiff = TArray<TSharedPtr<FDiffHelperCommit>>({SelectedCommits[0], MakeShared<FDiffHelperCommit>(DiffItem.Commits[Index - 1])});

	ExecuteDiff(CommitsToDiff, DiffItem.Path);
}

void UDiffHelperTabController::DiffSelectedCommitAgainstPrevious()
{
	const auto& SelectedCommits = Model->CommitPanelData.SelectedCommits;
	const auto& DiffItem = Model->SelectedDiffItem;

	const auto Index = GetCommitIndex(*SelectedCommits[0]);
	const auto CommitsToDiff = TArray<TSharedPtr<FDiffHelperCommit>>({MakeShared<FDiffHelperCommit>(DiffItem.Commits[Index + 1]), SelectedCommits[0]});

	ExecuteDiff(CommitsToDiff, DiffItem.Path);
}

void UDiffHelperTabController::DiffSelectedCommitAgainstNewest()
{
	const auto& SelectedCommits = Model->CommitPanelData.SelectedCommits;
	const auto& DiffItem = Model->SelectedDiffItem;

	const auto CommitsToDiff = TArray<TSharedPtr<FDiffHelperCommit>>({SelectedCommits[0], MakeShared<FDiffHelperCommit>(DiffItem.Commits[0])});

	ExecuteDiff(CommitsToDiff, DiffItem.Path);
}

void UDiffHelperTabController::DiffSelectedCommitAgainstOldest()
{
	const auto& SelectedCommits = Model->CommitPanelData.SelectedCommits;
	const auto& DiffItem = Model->SelectedDiffItem;

	const auto CommitsToDiff = TArray<TSharedPtr<FDiffHelperCommit>>({MakeShared<FDiffHelperCommit>(DiffItem.Commits.Last()), SelectedCommits[0]});

	ExecuteDiff(CommitsToDiff, DiffItem.Path);
}

bool UDiffHelperTabController::CanDiffAgainstTarget()
{
	const auto& DiffItem = Model->SelectedDiffItem;
	if (!DiffItem.IsValid()) { return false; }

	const auto bLastTargetCommitValid = DiffItem.LastTargetCommit.IsValid();
	const auto bHasChanges = DiffItem.Commits.Num() > 0;
	const auto bValidForDiff = UDiffHelperUtils::IsValidForDiff(DiffItem.Path);
	return bLastTargetCommitValid && bHasChanges && bValidForDiff; 
}

bool UDiffHelperTabController::CanDiffSelectedCommits()
{
	const auto& DiffItem = Model->SelectedDiffItem;
	if (!DiffItem.IsValid()) { return false; }
	
	const auto& Data = Model->CommitPanelData;

	const auto bOnlyTwoSelected = Data.SelectedCommits.Num() == 2;
	const auto bValidForDiff = UDiffHelperUtils::IsValidForDiff(DiffItem.Path);
	return bOnlyTwoSelected && bValidForDiff;
}

bool UDiffHelperTabController::CanDiffSelectedCommitAgainstNext()
{
	const auto& DiffItem = Model->SelectedDiffItem;
	if (!DiffItem.IsValid()) { return false; }
	
	const auto& Data = Model->CommitPanelData;
	if (Data.SelectedCommits.Num() != 1) { return false; }

	const auto Index = GetCommitIndex(*Data.SelectedCommits[0]);
	const auto bValidForDiff = UDiffHelperUtils::IsValidForDiff(DiffItem.Path);
	return Index > 0 && bValidForDiff;
}

bool UDiffHelperTabController::CanDiffSelectedCommitAgainstPrevious()
{
	const auto& DiffItem = Model->SelectedDiffItem;
	if (!DiffItem.IsValid()) { return false; }
	
	const auto& Data = Model->CommitPanelData;
	if (Data.SelectedCommits.Num() != 1) { return false; }

	const auto Index = GetCommitIndex(*Data.SelectedCommits[0]);
	const auto bValidForDiff = UDiffHelperUtils::IsValidForDiff(DiffItem.Path);
	return Index < (Model->SelectedDiffItem.Commits.Num() - 1) && bValidForDiff;
}

void UDiffHelperTabController::UpdateItemsData()
{
	// TODO: Estimate performance of this method.
	auto& Data = Model->DiffPanelData;
	Data.FilteredDiff = Data.OriginalDiff;
	
	UDiffHelperUtils::FilterListItems(Data.SearchFilter, Data.FilteredDiff);

	const auto OldTreeDiff = Data.TreeDiff;
	Data.TreeDiff = UDiffHelperUtils::ConvertListToTree(Data.FilteredDiff);
	UDiffHelperUtils::CopyExpandedState(OldTreeDiff, Data.TreeDiff);

	UDiffHelperUtils::SortDiffList(Data.SortMode, Data.FilteredDiff);
	UDiffHelperUtils::SortDiffTree(Data.SortMode, Data.TreeDiff);

	// TODO: We need to add more specific events for model update. Calling global update is not good approach.
	CallModelUpdated();
}

void UDiffHelperTabController::PopulateFilterSearchString(const FDiffHelperDiffItem& InItem, TArray<FString>& OutStrings) const
{
	OutStrings.Add(InItem.Path);
}

#undef LOCTEXT_NAMESPACE