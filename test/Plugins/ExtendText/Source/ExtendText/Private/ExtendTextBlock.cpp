// Copyright 1998-2016 Epic Games Inc and 2016-2016 glsseact. All Rights Reserved.

#include "ExtendTextBlock.h"

#define LOCTEXT_NAMESPACE "UMG"

FExtexndTextStyle::FExtexndTextStyle()
{
	if (GEngine) {
		Font = FSlateFontInfo(Cast<UObject>(GEngine->GetMediumFont()), 24);
	}

	Color.R = 1.0f;
	Color.G = 1.0f;
	Color.B = 1.0f;
	Color.A = 1.0f;
}

UExtendTextBlock::UExtendTextBlock(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//Set Default Text Style
	static ConstructorHelpers::FObjectFinder<UFont> RobotoFont(TEXT("/Engine/EngineFonts/Roboto"));
	DefaultTextStyle.Font = FSlateFontInfo(RobotoFont.Object, 24, FName("Regular"));
	DefaultTextStyle.ShadowColorAndOpacity = FLinearColor::Transparent;
	DefaultTextStyle.ShadowOffset = FVector2D(1.0f, 1.0f);
	DefaultTextStyle.ColorAndOpacity = FLinearColor::White;

	//Set Default Justification
	Justification = ETextJustify::Left;

	Instance = nullptr;
}

FText UExtendTextBlock::GetText() const
{
	if (MyTextBlock.IsValid())
	{
		return MyTextBlock->GetText();
	}

	return Text;
}

void UExtendTextBlock::SetReplaceNewLineToWhiteSpace(bool set)
{
	ReplaceNewLineToWhiteSpace = set;
}

void UExtendTextBlock::SetConvertLocalizationTag(bool set)
{
	ConvertLocalizationTag = set;
}

void UExtendTextBlock::CalcLineCount(const FString& origin, int32 charCountInOneLine, TArray<FString>& outStrForEachLine)
{
	if (charCountInOneLine == 0) {
		return;
	}

	FString tempStr{origin};
	int32 newLineIndex = INDEX_NONE;
	while (tempStr.Len() > charCountInOneLine || tempStr.FindChar('\n', newLineIndex)) {
		FString splited = tempStr.Left(charCountInOneLine);
		if (splited.FindChar('\n', newLineIndex)) {
			splited = splited.Left(newLineIndex).TrimEnd();
			outStrForEachLine.Emplace(splited);
			tempStr = tempStr.Right(tempStr.Len() - newLineIndex - 1);
			continue;
		}
		outStrForEachLine.Emplace(splited);
		tempStr = tempStr.Right(tempStr.Len() - splited.Len());
	}
	if (tempStr.IsEmpty() == false) {
		outStrForEachLine.Emplace(tempStr);
	}
}

FText UExtendTextBlock::GetOmittedText(const FText& originText, UExtendTextBlock* textWidget, int32 omitByWidth, int32 omitByMultiLineLimit, bool& result)
{
	result = false;
	if (omitByWidth == 0) {
		return originText;
	}

	constexpr int32 OmitTextCount{ 4 }; // count of "..."
	FString origin{ originText.ToString() };
	const auto& textStyle = textWidget->DefaultTextStyle;
	int32 fontSize = textStyle.Font.Size - textStyle.Font.OutlineSettings.OutlineSize;
	int32 limitTextCount = omitByWidth / fontSize - OmitTextCount;

	if (omitByMultiLineLimit != 0) {
		TArray<FString> splited;
		CalcLineCount(origin, limitTextCount, splited);
		if (splited.Num() > omitByMultiLineLimit) {
			FString lastSentence = splited[omitByMultiLineLimit - 1];
			FString omitted = lastSentence.Left(limitTextCount) + "..";
			FString resolved;
			for (int32 i = 0; i < omitByMultiLineLimit - 1; i++) {
				resolved += (splited[i] + "\n");
			}
			resolved += omitted;
			result = true;
			return FText::FromString(resolved);
		}
	}
	else { // single line
		FString lastSentence = origin;
		int32 textLength = fontSize * lastSentence.Len();
		if (textLength > (limitTextCount* fontSize)) {
			FString omitted = lastSentence.Left(limitTextCount) + "..";
			result = true;
			return FText::FromString(omitted);
		}
	}

	return originText;
}

void UExtendTextBlock::SetText(FText InText)
{
	Text = InText;

	if (MyTextBlock.IsValid())
	{
		if (ReplaceNewLineToWhiteSpace) {
			Text = FText::FromString(Text.ToString().Replace(TEXT("\n"), TEXT(" ")));
			Text = FText::FromString(Text.ToString().Replace(TEXT("\r"), TEXT("")));
		}

		Text = OnAdjustText(Text);

		// Keep the original text.
		//Text = GetOmittedText(Text, this, OmitByWidth, OmitByMultiLineLimit);
		bool omitted = false;
		MyTextBlock->SetText(GetOmittedText(Text, this, OmitByWidth, OmitByMultiLineLimit, omitted));
	}
}

void UExtendTextBlock::SetJustification(ETextJustify::Type InJustification)
{
	Justification = InJustification;
	if ( MyTextBlock.IsValid() )
	{
		MyTextBlock->SetJustification(Justification);
	}
}

void UExtendTextBlock::SetWrapTextAt(float inWrapTextAt)
{
	if (inWrapTextAt > 0.f) {
		AutoWrapText = 0;
	}
	else if(FMath::IsNearlyZero(inWrapTextAt)) {
		AutoWrapText = 1;
	}
	WrapTextAt = inWrapTextAt;

	if (MyTextBlock.IsValid()) {
		MyTextBlock->SetAutoWrapText(AutoWrapText > 0);
		MyTextBlock->SetWrapTextAt(WrapTextAt);
	}
}

void UExtendTextBlock::SetOmitText(int32 omitByWidth)
{
	if (omitByWidth == OmitByWidth) {
		return;
	}
	OmitByWidth = omitByWidth;
	SetText(Text); // refresh
}

void UExtendTextBlock::SetUnderlineBrush(FSlateBrush InUnderlineBrush)
{
	DefaultTextStyle.SetUnderlineBrush(InUnderlineBrush);
	SetDefaultTextStyle(DefaultTextStyle);
}

void UExtendTextBlock::SetLineHeightPercentage(float percentage)
{
	LineHeightPercentage = percentage;
	if (MyTextBlock.IsValid()) {
		MyTextBlock->SetLineHeightPercentage(percentage);
	}
}

TSharedRef<SWidget> UExtendTextBlock::RebuildWidget()
{
	OnSynchronizeProperties();
	Instance = BuildWidgetOption();

	bool omitted = false;
	bool IsAutoWraptext = AutoWrapText > 0 ? true : false;
	MyTextBlock =
		SNew(SRichTextBlock)
		.Text(GetOmittedText(Text, this, OmitByWidth, OmitByMultiLineLimit, omitted))
		.TextStyle(&DefaultTextStyle)
		.DecoratorStyleSet(Instance.Get())
		.Justification(Justification)
		.AutoWrapText(IsAutoWraptext)
		.WrappingPolicy(WrappingPolicy)
		.WrapTextAt(WrapTextAt)
		.LineHeightPercentage(LineHeightPercentage);
	
	FText::GetFormatPatternParameters(Text, ArgumentParams);

	return MyTextBlock.ToSharedRef();
}

void UExtendTextBlock::OnBindingChanged(const FName& Property)
{
	Super::OnBindingChanged(Property);

	if (MyTextBlock.IsValid())
	{
		static const FName TextProperty(TEXT("TextDelegate"));

		if (Property == TextProperty)
		{
			bool omitted = false;
			TAttribute<FText> TextBinding = PROPERTY_BINDING(FText, Text);
			MyTextBlock->SetText(GetOmittedText(TextBinding.Get(), this, OmitByWidth, OmitByMultiLineLimit, omitted));
		}
	}
}

void UExtendTextBlock::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	TAttribute<FText> TextBinding = PROPERTY_BINDING(FText, Text);

	bool omitted = false;
	MyTextBlock->SetText(GetOmittedText(TextBinding.Get(), this, OmitByWidth, OmitByMultiLineLimit, omitted));
	MyTextBlock->SetTextStyle(DefaultTextStyle);
}

void UExtendTextBlock::SetDefaultTextStyle(FTextBlockStyle TextBlockStyle)
{
	DefaultTextStyle = TextBlockStyle;

	if (MyTextBlock.IsValid()) {
		MyTextBlock->SetTextStyle(DefaultTextStyle);
		Instance = BuildWidgetOption();
		MyTextBlock->SetDecoratorStyleSet(Instance.Get());
	}
}

void UExtendTextBlock::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyTextBlock.Reset();
}

TSharedRef< ISlateStyle > UExtendTextBlock::BuildWidgetOption()
{
	TSharedRef< class FSlateStyleSet > StyleSet = MakeShareable(new FSlateStyleSet("ExtendTextBlockStyleSet"));
	StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	for (auto& Style : Styles)
	{
		Style.Value.Font.Size = DefaultTextStyle.Font.Size + Style.Value.FontSize;
		StyleSet->Set(Style.Key,
			FTextBlockStyle(DefaultTextStyle)
			.SetColorAndOpacity(Style.Value.Color)
			.SetFont(Style.Value.Font)
			.SetShadowOffset(Style.Value.ShadowOffset)
			.SetShadowColorAndOpacity(Style.Value.ShadowColor));
	}

	return StyleSet;
}

void UExtendTextBlock::OnSynchronizeProperties_Implementation()
{
	// Override this event in Blueprint
}

#if WITH_EDITOR
const FText UExtendTextBlock::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

void UExtendTextBlock::OnCreationFromPalette()
{
	Text = LOCTEXT("TextBlockDefaultValue", "Text Block");
}

FString UExtendTextBlock::GetLabelMetadata() const
{
	const int32 MaxSampleLength = 15;

	FString TextStr = Text.ToString();
	TextStr = TextStr.Len() <= MaxSampleLength ? TextStr : TextStr.Left(MaxSampleLength - 2) + TEXT("..");
	return TEXT(" \"") + TextStr + TEXT("\"");
}

#endif

UMaterialInstanceDynamic* UExtendTextBlock::GetFontDynamicMaterial()
{
	UMaterialInterface* Material = nullptr;
	UObject* Resource = DefaultTextStyle.Font.FontMaterial;
	Material = Cast<UMaterialInterface>(Resource);
	if (Material) {
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Material);
		if (!DynamicMaterial) {
			DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
			DefaultTextStyle.Font.FontMaterial = DynamicMaterial;
			if (MyTextBlock.IsValid()) {
				MyTextBlock->SetTextStyle(DefaultTextStyle);
			}
		}
		return DynamicMaterial;
	}
	return nullptr;
}

UMaterialInstanceDynamic* UExtendTextBlock::GetOutlineFontDynamicMaterial()
{
	UMaterialInterface* Material = nullptr;
	UObject* Resource = DefaultTextStyle.Font.OutlineSettings.OutlineMaterial;
	Material = Cast<UMaterialInterface>(Resource);
	if (Material) {
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Material);
		if (!DynamicMaterial) {
			DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
			DefaultTextStyle.Font.OutlineSettings.OutlineMaterial = DynamicMaterial;
			if (MyTextBlock.IsValid()) {
				MyTextBlock->SetTextStyle(DefaultTextStyle);
			}
		}
		return DynamicMaterial;
	}
	return nullptr;
}