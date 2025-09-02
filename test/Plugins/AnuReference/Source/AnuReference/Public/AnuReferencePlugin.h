// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#if WITH_EDITOR
#include "Engine/World.h"
#include "Editor.h"
#include "LevelEditor.h"
#endif //WITH_EDITOR
#include "Kismet/GamePlayStatics.h"


class FAnuReferenceModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void Export();
	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedPtr<class FUICommandList> pluginCommands;
};
