// Copyright 1998-2016 Epic Games Inc and 2016-2016 glsseact. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ExtendText.h"

#include "UMG.h"
#include "UMGStyle.h"

#include "Components/TextWidgetTypes.h"
#include "Widgets/Text/SRichTextBlock.h"

#include "ExtendTextBlock.generated.h"

/**
* FExtendTextStyle is to represent various TextStyle in Extend Text.
* FExtendTextStyle will be to use in Blueprint.
*/
USTRUCT(BlueprintType)
struct EXTENDTEXT_API FExtexndTextStyle
{
	GENERATED_USTRUCT_BODY()
public:
	FExtexndTextStyle();


	FExtexndTextStyle(FSlateFontInfo InFont, FLinearColor InColor)
	{
		this->Font = InFont;
		this->Color = InColor;
	}

	//Slate Font Information
	UPROPERTY(EditAnywhere, Category = "Style", BlueprintReadWrite)
		FSlateFontInfo Font;

	//Style Color
	UPROPERTY(EditAnywhere, Category = "Style", BlueprintReadWrite)
		FLinearColor Color;

	//FontSize
	UPROPERTY(EditAnywhere, Category = "Style", BlueprintReadWrite)
		int32 FontSize = 0;

	UPROPERTY(EditAnywhere, Category = "Style", BlueprintReadWrite)
		FLinearColor ShadowColor = FLinearColor(ForceInitToZero);

	UPROPERTY(EditAnywhere, Category = "Style", BlueprintReadWrite)
		FVector2D ShadowOffset = FVector2D::ZeroVector;
};

/**
* A simple static fancy text widget.

* �� No Children
* �� Dynamic Fancy Text
*/

UCLASS(meta = (DisplayName = "Extend Text"))
class EXTENDTEXT_API UExtendTextBlock : public UTextLayoutWidget
{
	GENERATED_UCLASS_BODY()
public:
	static void CalcLineCount(const FString& origin, int32 charCountInOneLine, TArray<FString>& outStrForEachLine);
	UFUNCTION(BlueprintPure)
		static FText GetOmittedText(const FText& originText, UExtendTextBlock* textWidget, int32 omitByWidth, int32 omitByMultiLineLimit, bool& result);

public:
	UFUNCTION(BlueprintNativeEvent, Category = "User Interface")
		void OnSynchronizeProperties();
	UFUNCTION(BlueprintImplementableEvent, Category = "User Interface")
		FText OnAdjustText(const FText& text);
	/**
	* Directly sets the widget text.
	* Warning: This will wipe any binding created for the Text property!
	* @param InText The text to assign to the widget
	*/
	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "SetText (Text)"))
		void SetText(FText InText);

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "SetReplaceNewLineToWhiteSpace (bool)"))
		void SetReplaceNewLineToWhiteSpace(bool set);

	UFUNCTION(BlueprintCallable, Category = "Widget")
		void SetConvertLocalizationTag(bool set);

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "GetText (Text)"))
		FText GetText() const;

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "SetDefaultTextStyle (TextBlockStyle)"))
		void SetDefaultTextStyle(FTextBlockStyle TextBlockStyle);

	//UFUNCTION(BlueprintCallable, Category = "Appearance")
		void SetJustification(ETextJustify::Type InJustification);

	UFUNCTION(BlueprintCallable, Category = "Wrapping")
		void SetWrapTextAt(float inWrapTextAt);

	UFUNCTION(BlueprintCallable, Category = "Wrapping")
		void SetOmitText(int32 omitByWidth);

	UFUNCTION(BlueprintCallable, Category = Appearance)
		void SetUnderlineBrush(FSlateBrush InUnderlineBrush);

	UFUNCTION(BlueprintCallable, Category = Appearance)
		void SetLineHeightPercentage(float percentage);

	UFUNCTION(BlueprintCallable)
		class UMaterialInstanceDynamic* GetFontDynamicMaterial();
	UFUNCTION(BlueprintCallable)
		class UMaterialInstanceDynamic* GetOutlineFontDynamicMaterial();

	//StyleSet
	TSharedRef< ISlateStyle > BuildWidgetOption();
public:
	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	// UVisual interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	// End of UVisual interface

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
	virtual void OnCreationFromPalette() override;

	virtual FString GetLabelMetadata() const override;
#endif

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() ;
	virtual void OnBindingChanged(const FName& Property) ;
	// End of UWidget interface

public:
	/** The text to display */
	UPROPERTY(EditAnywhere, Category = Content, meta = (MultiLine = "true"))
	FText Text;

	UPROPERTY(EditAnywhere, Category = Content)
	bool ReplaceNewLineToWhiteSpace = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Content)
	bool ConvertLocalizationTag = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Content)
	int32 OmitByWidth = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Content)
	int32 OmitByMultiLineLimit = 0;

	/** A bindable delegate to allow logic to drive the text of the widget */
	UPROPERTY()
	FGetText TextDelegate;

	/** The defualt style of the text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter=SetDefaultTextStyle, Category = Appearance, meta = (DisplayName = "DefaultTextStyle"))
	FTextBlockStyle DefaultTextStyle;

	/** How the text should be aligned with the margin. */
	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
		TEnumAsByte<ETextJustify::Type> Justification;*/

	/** True if we're wrapping text automatically based on the computed horizontal space for this widget */
	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
		bool AutoWrapText;*/
	
	/** The styles of the text */
	UPROPERTY(EditAnywhere, Category = Appearance, meta = (DisplayName = "Styles"))
	TMap<FName, FExtexndTextStyle> Styles;	

	UPROPERTY(BlueprintReadOnly, Category = Content, meta = (MultiLine = "true"))
	TArray<FString> ArgumentParams;	
		
protected:
	TSharedPtr<SRichTextBlock> MyTextBlock;
	TSharedPtr< class ISlateStyle > Instance;

	PROPERTY_BINDING_IMPLEMENTATION(FText, Text);
};
