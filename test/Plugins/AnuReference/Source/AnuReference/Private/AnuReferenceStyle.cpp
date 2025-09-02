// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AnuReferenceStyle.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#if WITH_EDITOR
#include "Interfaces/IPluginManager.h"
#endif //WITH_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedPtr<FSlateStyleSet> FAnuReferenceExportStyle::instance = nullptr;

void FAnuReferenceExportStyle::Initialize()
{
	if (!instance.IsValid())
	{
		instance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*instance);
	}
}

void FAnuReferenceExportStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*instance);
	ensure(instance.IsUnique());
	instance.Reset();
}

FName FAnuReferenceExportStyle::GetStyleSetName()
{
	static FName name(TEXT("AnuReferenceExportStyle"));
	return name;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

TSharedRef<FSlateStyleSet> FAnuReferenceExportStyle::Create()
{
	TSharedRef<FSlateStyleSet> style = MakeShared<FSlateStyleSet>("AnuReferenceExportStyle");
#if WITH_EDITOR
	style->SetContentRoot(IPluginManager::Get().FindPlugin("AnuReference")->GetBaseDir() / TEXT("Resources"));
	style->Set("AnuReference.PluginAction", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
#endif //WITH_EDITOR

	return style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FAnuReferenceExportStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FAnuReferenceExportStyle::Get()
{
	return *instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LOCTEXT_NAMESPACE "FAnuReferenceModule"

void FExportCommands::RegisterCommands()
{
	UI_COMMAND(action, "Export Spawner", "Export spawn actors to reference file", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
