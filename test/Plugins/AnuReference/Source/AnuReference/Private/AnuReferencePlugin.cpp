// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AnuReferencePlugin.h"
#include "KeyGenerator.h"

#if WITH_EDITOR
#include "UnrealEd/Public/FileHelpers.h"
#endif

#include "AnuReferenceStyle.h"
#include "Reference.h"
#include "ReferenceBuilder.h"
#include "LogAnuReference.h"


#define LOCTEXT_NAMESPACE "FAnuReferenceModule"

DEFINE_LOG_CATEGORY(LogAnuReference);

void FAnuReferenceModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FAnuReferenceExportStyle::Initialize();
	FAnuReferenceExportStyle::ReloadTextures();

	FExportCommands::Register();

	pluginCommands = MakeShared<FUICommandList>();
	pluginCommands->MapAction( FExportCommands::Get().action, FExecuteAction::CreateRaw(this, &FAnuReferenceModule::Export), FCanExecuteAction());

	TSharedPtr<FExtender> extender = MakeShared<FExtender>();
	extender->AddMenuExtension("WindowLayout", EExtensionHook::After, pluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FAnuReferenceModule::AddMenuExtension));

#if WITH_EDITOR
	FLevelEditorModule& module = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	module.GetMenuExtensibilityManager()->AddExtender(extender);
#endif //WITH_EDITOR
}

void FAnuReferenceModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FAnuReferenceModule::Export()
{
#if WITH_EDITOR
	UWorld* world = GEditor->GetEditorWorldContext().World();

	TArray<AActor*> allActors;
	UGameplayStatics::GetAllActorsOfClass(world, AActor::StaticClass(), allActors);
	TArray<AActor*> lookupTargets;
	TArray<ARefInteractor*> spawnInteractor;
	for (auto actor : allActors) {
		if (auto spawner = Cast<ARefInteractor>(actor)) {
			spawnInteractor.Emplace(spawner);
			lookupTargets.Emplace(actor);
			continue;
		}

		auto lookups = actor->GetComponentsByInterface(UWorldLookupTarget::StaticClass());
		if (lookups.Num() != 0) {
			lookupTargets.Emplace(actor);
			continue;
		}
	}

	UReferenceBuilder::ExportWorldLookupTarget(world, lookupTargets);
	if (UReferenceBuilder::ExportSpawner(spawnInteractor)) {
		FEditorFileUtils::SaveCurrentLevel();
	}
#endif //WITH_EDITOR
}

void FAnuReferenceModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FExportCommands::Get().action);
}

void FAnuReferenceModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FExportCommands::Get().action);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAnuReferenceModule, AnuReference)
