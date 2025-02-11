﻿// Copyright 2024 Gradess Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/TextFilter.h"
#include "DiffHelperTypes.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDiffHelperSimpleDynamicDelegate);
DECLARE_MULTICAST_DELEGATE(FDiffHelperSimpleDelegate);
DECLARE_DELEGATE(FDiffHelperEvent)

DECLARE_LOG_CATEGORY_EXTERN(LogDiffHelper, Log, All);

// TODO: Make a nested namespace for constants
namespace DiffHelperConstants
{
	const FName DiffHelperRevisionPickerId(TEXT("DiffHelperRevisionPicker"));
	const FName DiffHelperDiffViewerId(TEXT("DiffHelperTab"));
}

namespace SDiffHelperDiffPanelConstants
{
	const FName PathColumnId(TEXT("Path"));

	const int32 ListWidgetIndex = 0;
	const int32 TreeWidgetIndex = 1;
}

namespace SDiffHelperCommitPanelConstants
{
	const FName HashColumnId(TEXT("CommitHash"));
	const FName MessageColumnId(TEXT("CommitMessage"));
	const FName AuthorColumnId(TEXT("CommitAuthor"));
	const FName DateColumnId(TEXT("CommitDate"));
}


UENUM()
enum class EDiffHelperFileStatus : uint8
{
	None,
	Added,
	Modified,
	Deleted,
	Renamed,
	Copied,
	Unmerged
};

USTRUCT(BlueprintType)
struct FDiffHelperBranch
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FString Revision;

	operator FString() const { return Name; }

	FORCEINLINE bool IsValid() const { return !Name.IsEmpty(); }
	FORCEINLINE bool operator==(const FDiffHelperBranch& Other) const { return Name == Other.Name && Revision == Other.Revision; }
	FORCEINLINE bool operator!=(const FDiffHelperBranch& Other) const { return !(*this == Other); }
};

USTRUCT(BlueprintType)
struct FDiffHelperFileData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FString Path;

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	EDiffHelperFileStatus Status = EDiffHelperFileStatus::None;
};

USTRUCT(BlueprintType)
struct FDiffHelperCommit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FString Revision;

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FString Author;

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FDateTime Date;

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	TArray<FDiffHelperFileData> Files;

	FORCEINLINE bool IsValid() const { return !Revision.IsEmpty(); }
};

USTRUCT(BlueprintType)
struct FDiffHelperDiffItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FString Path;

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	EDiffHelperFileStatus Status = EDiffHelperFileStatus::None;
	
	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FAssetData AssetData;

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	FDiffHelperCommit LastTargetCommit;

	UPROPERTY(BlueprintReadOnly, Category = "Diff Helper")
	TArray<FDiffHelperCommit> Commits;

	FORCEINLINE bool IsValid() const { return !Path.IsEmpty(); }
};

USTRUCT()
struct FDiffHelperItemNode
{
	GENERATED_BODY()

	UPROPERTY()
	FString Path;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	bool bExpanded = false;

	TSharedPtr<FDiffHelperDiffItem> DiffItem;
	TArray<TSharedPtr<FDiffHelperItemNode>> Children;

	FORCEINLINE bool IsValid() const { return !Path.IsEmpty(); }
};

USTRUCT()
struct FDiffHelperDiffPanelData
{
	GENERATED_BODY()

	FDiffHelperSimpleDelegate OnPreWidgetIndexChanged;
	FDiffHelperSimpleDelegate OnTreeDiffExpansionUpdated;

	int32 CurrentWidgetIndex = 0;
	
	TSharedPtr<TTextFilter<const FDiffHelperDiffItem&>> SearchFilter = nullptr;
	
	TArray<TSharedPtr<FDiffHelperItemNode>> OriginalDiff;
	TArray<TSharedPtr<FDiffHelperItemNode>> FilteredDiff;
	TArray<TSharedPtr<FDiffHelperItemNode>> TreeDiff;
	TSharedPtr<FDiffHelperItemNode> SelectedNode;

	EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
};

USTRUCT()
struct FDiffHelperCommitPanelData
{
	GENERATED_BODY()

	TArray<TSharedPtr<FDiffHelperCommit>> SelectedCommits;
};

USTRUCT()
struct FDiffHelperDiffTabData
{
	GENERATED_BODY()

	UPROPERTY()
	FString SourceBranchName;

	UPROPERTY()
	FString TargetBranchName;

	bool operator==(const FDiffHelperDiffTabData& InOther) const
	{
		return SourceBranchName == InOther.SourceBranchName && TargetBranchName == InOther.TargetBranchName;
	}

	friend inline uint32 GetTypeHash(const FDiffHelperDiffTabData& Key)
	{
		return HashCombineFast(GetTypeHash(Key.SourceBranchName), GetTypeHash(Key.TargetBranchName));
	}
};
