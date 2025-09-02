#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"
#include "Framework/Commands/Commands.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FAnuReferenceExportStyle
{
public:
	static void Initialize();
	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	/** @return The Slate style set for the Shooter game */
	static const ISlateStyle& Get();

	static FName GetStyleSetName();

private:
	static TSharedRef<class FSlateStyleSet> Create();

private:
	static TSharedPtr<class FSlateStyleSet> instance;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FExportCommands : public TCommands<FExportCommands>
{
public:

	FExportCommands(): TCommands<FExportCommands>(TEXT("ReferenceExport"), NSLOCTEXT("Contexts", "ReferenceExport", "AnuReferenceExport Plugin"), NAME_None, FAnuReferenceExportStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> action;
};
