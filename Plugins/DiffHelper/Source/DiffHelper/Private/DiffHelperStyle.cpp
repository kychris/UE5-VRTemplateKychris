// Copyright 2024 Gradess Games. All Rights Reserved.

#include "DiffHelperStyle.h"
#include "DiffHelper.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/ToolBarStyle.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FDiffHelperStyle::StyleInstance = nullptr;

void FDiffHelperStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FDiffHelperStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FDiffHelperStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("DiffHelperStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FDiffHelperStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("DiffHelperStyle"));
	SetStyles(Style);
	
	return Style;
}

void FDiffHelperStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

void FDiffHelperStyle::ReloadStyles()
{
	if (StyleInstance.IsValid())
	{
		SetStyles(StyleInstance.ToSharedRef());
		FSlateApplication::Get().GetRenderer()->ReleaseAccessedResources(false);
		FSlateApplication::Get().GetRenderer()->LoadStyleResources(*StyleInstance);
	}
}

void FDiffHelperStyle::SetStyles(TSharedRef<FSlateStyleSet> Style)
{
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("DiffHelper")->GetBaseDir() / TEXT("Resources"));

	// TODO: I assume we dont need "DiffHelper" prefix for the property names because our style is already inside the "DiffHelper"
	Style->Set("DiffHelper.Diff", new IMAGE_BRUSH_SVG(TEXT("DiffIcon"), Icon20x20));
	Style->Set("DiffHelper.Directory", new IMAGE_BRUSH_SVG(TEXT("DirectoryIcon"), Icon20x20));
	Style->Set("DiffHelper.Directory.Small", new IMAGE_BRUSH_SVG(TEXT("DirectoryIcon"), Icon16x16));
	Style->Set("DiffHelper.ExpandAll", new IMAGE_BRUSH_SVG(TEXT("ExpandAll"), Icon20x20));
	Style->Set("DiffHelper.CollapseAll", new IMAGE_BRUSH_SVG(TEXT("CollapseAll"), Icon20x20));

	// Arrows
	Style->Set("DiffHelper.ArrowUp", new IMAGE_BRUSH_SVG(TEXT("ArrowUp"), Icon20x20));
	Style->Set("DiffHelper.ArrowDown", new IMAGE_BRUSH_SVG(TEXT("ArrowDown"), Icon20x20));
	Style->Set("DiffHelper.DoubleArrowUp", new IMAGE_BRUSH_SVG(TEXT("DoubleArrowUp"), Icon20x20));
	Style->Set("DiffHelper.DoubleArrowDown", new IMAGE_BRUSH_SVG(TEXT("DoubleArrowDown"), Icon20x20));
	Style->Set("DiffHelper.VerticalOutwardArrow", new IMAGE_BRUSH_SVG(TEXT("VerticalOutwardArrow"), Icon20x20));
	Style->Set("DiffHelper.HorizontalOutwardArrow", new IMAGE_BRUSH_SVG(TEXT("HorizontalOutwardArrow"), Icon20x20));

	// Diff panel tree item
	Style->Set("DiffHelper.Highlight", new FSlateColorBrush(FLinearColor(0.259f, 0.541f, 0.067f, 0.3f)));
}

const ISlateStyle& FDiffHelperStyle::Get()
{
	return *StyleInstance;
}
