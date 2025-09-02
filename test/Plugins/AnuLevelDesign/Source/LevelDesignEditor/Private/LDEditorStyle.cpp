// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDEditorStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FLDEditorStyle::StyleInstance = NULL;

void FLDEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FLDEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FLDEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("LDEditorStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon32x32(32.0f, 32.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon64x64(64.0f, 64.0f);

TSharedRef< FSlateStyleSet > FLDEditorStyle::Create()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("AnuLevelDesign");
	FString PluginPath = Plugin->GetBaseDir();

	TSharedRef< FSlateStyleSet > Style = MakeShared<FSlateStyleSet>("LDEditorStyle");
	Style->SetContentRoot(PluginPath / TEXT("Resources"));

	Style->Set("LDEditor.background_blue", new BOX_BRUSH(TEXT("bg_blue"), FMargin(4.f / 16.f)));
	Style->Set("LDEditor.background_purple", new BOX_BRUSH(TEXT("bg_purple"), FMargin(4.f / 16.f)));
	Style->Set("LDEditor.background_red", new BOX_BRUSH(TEXT("bg_red"), FMargin(4.f / 16.f)));
	Style->Set("LDEditor.background_green", new BOX_BRUSH(TEXT("bg_green"), FMargin(4.f / 16.f)));
	Style->Set("LDEditor.background_gray", new BOX_BRUSH(TEXT("bg_gray"), FMargin(4.f / 16.f)));

	Style->Set("LDEditor.arrow_down", new IMAGE_BRUSH(TEXT("arrow_down"), FVector2D(64.0f, 12.0f)));
	Style->Set("LDEditor.arrow_up", new IMAGE_BRUSH(TEXT("arrow_up"), FVector2D(64.0f, 12.0f)));

	Style->Set("LDEditor.nodeicon.root", new IMAGE_BRUSH(TEXT("nodeicon_root"), Icon20x20));
	Style->Set("LDEditor.nodeicon.entry", new IMAGE_BRUSH(TEXT("nodeicon_entry"), Icon20x20));
	Style->Set("LDEditor.nodeicon.success", new IMAGE_BRUSH(TEXT("nodeicon_success"), Icon20x20));
	Style->Set("LDEditor.nodeicon.failure", new IMAGE_BRUSH(TEXT("nodeicon_failure"), Icon20x20));
	Style->Set("LDEditor.nodeicon.action", new IMAGE_BRUSH(TEXT("nodeicon_action"), Icon20x20));

	// TextStyle
	{
		Style->Set("LD.NodeTitle", FTextBlockStyle()
			.SetFont(DEFAULT_FONT("Bold", 11))
			.SetColorAndOpacity(FLinearColor(230.0f / 255.0f, 230.0f / 255.0f, 230.0f / 255.0f))
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f)));

		Style->Set("LD.NodeText", FTextBlockStyle()
			.SetFont(DEFAULT_FONT("Normal", 8))
			.SetColorAndOpacity(FLinearColor(218.0f / 255.0f, 218.0f / 255.0f, 218.0f / 255.0f)));

		Style->Set("PivotTool.NormalText.Blue", FTextBlockStyle()
			.SetFont(DEFAULT_FONT("Fonts/DroidSansMono", 8))
			.SetColorAndOpacity(FLinearColor(.0f, .0f, 1.0f))
			.SetHighlightColor(FLinearColor(.0f, .0f, 1.0f)));

		Style->Set("PivotTool.ButtonText.Roboto", FTextBlockStyle()
			.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f)));

		Style->Set("PivotTool.ButtonText", FTextBlockStyle()
			.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f)));

		Style->Set("PivotTool.ButtonText.Black", FTextBlockStyle()
			.SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f))
			.SetHighlightColor(FLinearColor(0.f, 0.f, 0.f))
		);
	}

	FTextBlockStyle NormalText;
	FTextBlockStyle GraphNodeTitle = FTextBlockStyle(NormalText)
		.SetFont(DEFAULT_FONT("Bold", 11))
		.SetColorAndOpacity(FLinearColor(230.0f / 255.0f, 230.0f / 255.0f, 230.0f / 255.0f))
		.SetShadowOffset(FVector2D(2, 2))
		.SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	Style->Set("Graph.Node.NodeTitle", GraphNodeTitle);

	FTextBlockStyle TextStyle = FTextBlockStyle(NormalText)
		.SetFont(DEFAULT_FONT("Bold", 10))
		.SetColorAndOpacity(FLinearColor(218.0f / 255.0f, 218.0f / 255.0f, 218.0f / 255.0f))
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	Style->Set("Graph.Node.Text", TextStyle);

	FEditableTextBoxStyle GraphNodeTitleEditableText = FEditableTextBoxStyle()
		.SetFont(NormalText.Font);

	Style->Set("Graph.Node.NodeTitleEditableText", GraphNodeTitleEditableText);

	Style->Set("Graph.Node.NodeTitleInlineEditableText", FInlineEditableTextBlockStyle()
		.SetTextStyle(GraphNodeTitle)
		.SetEditableTextBoxStyle(GraphNodeTitleEditableText)
	);

	if(FSlateApplication::IsInitialized())
	{
		Style->Set("DefaultAppIconEditor", new FSlateBrush(*FSlateApplication::Get().GetAppIcon()));
	}

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef DEFAULT_FONT

void FLDEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FLDEditorStyle::Get()
{
	return *StyleInstance;
}

const FSlateBrush* FLDEditorStyle::GetBrush(FName PropertyName)
{
	return StyleInstance->GetBrush(PropertyName);
}

const FString FLDEditorStyle::GetImageName(FString RelativePath)
{
	return StyleInstance ? StyleInstance->RootToContentDir(RelativePath, TEXT(".png")) : "";
}