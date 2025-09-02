// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnuDataTableEditorModule.h"
#include "Modules/ModuleManager.h"
#include "IDataTableEditor.h"

#include "AnuDataTableEditor.h"
#include "AssetTypeActions_AnuDataTable.h"
#include "Viewer/ResourceModelDataTableEditor.h"
#include "Viewer/ResourceMeshPartialDataTableEditor.h"
#include "Viewer/SkillTimelineDataTableEditor.h"
#include "Viewer/ResourceSnapshotDataTableEditor.h"

const FName FAnuDataTableEditorModule::AnuDataTableEditorAppIdentifier(TEXT("DataTableEditorApp"));

IMPLEMENT_MODULE(FAnuDataTableEditorModule, AnuDataTableEditor);

void FAnuDataTableEditorModule::StartupModule()
{
	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type AnuAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Anu")), FText::FromString("Anu"));
	RegisterAssetTypeAction(AssetTools, MakeShared<FAssetTypeActions_AnuDataTable>(AnuAssetCategoryBit));
}

void FAnuDataTableEditorModule::ShutdownModule()
{	
	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools")) 
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index) 
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}
	CreatedAssetTypeActions.Empty();
}

TSharedRef<IDataTableEditor> FAnuDataTableEditorModule::CreateDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	if (FResourceModelDataTableEditor::SupportedDataTableName.Equals(Table->GetName()))
	{
		return CreateResourceModelDataTableEditor(Mode, InitToolkitHost, Table);
	}

	if (Table->GetPathName().Contains(FResourceMeshPartialDataTableEditor::SupportedDataTablePath))
	{
		return CreateResourceMeshPartialDataTableEditor(Mode, InitToolkitHost, Table);
	}

	if (FSkillTimelineDataTableEditor::SupportedDataTableName.Equals(Table->GetName()))
	{
		return CreateSkillTimelineDataTableEditor(Mode, InitToolkitHost, Table);
	}

	if (FResourceSnapshotDataTableEditor::SupportedDataTableName.Equals(Table->GetName()))
	{
		return CreateSnapshotDataTableEditor(Mode, InitToolkitHost, Table);
	}

	TSharedRef<FAnuDataTableEditor> NewDataTableEditor(new FAnuDataTableEditor());
	NewDataTableEditor->InitDataTableEditor(Mode, InitToolkitHost, Table);
	return NewDataTableEditor;
}

TSharedRef<IDataTableEditor> FAnuDataTableEditorModule::CreateResourceModelDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	TSharedRef<FResourceModelDataTableEditor> NewDataTableEditor(new FResourceModelDataTableEditor());
	NewDataTableEditor->InitDataTableEditor(Mode, InitToolkitHost, Table);
	return NewDataTableEditor;
}

TSharedRef<IDataTableEditor> FAnuDataTableEditorModule::CreateResourceMeshPartialDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	TSharedRef<FResourceMeshPartialDataTableEditor> NewDataTableEditor(new FResourceMeshPartialDataTableEditor());
	NewDataTableEditor->InitDataTableEditor(Mode, InitToolkitHost, Table);
	return NewDataTableEditor;
}

TSharedRef<IDataTableEditor> FAnuDataTableEditorModule::CreateSkillTimelineDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	TSharedRef<FSkillTimelineDataTableEditor> NewDataTableEditor(new FSkillTimelineDataTableEditor());
	NewDataTableEditor->InitDataTableEditor(Mode, InitToolkitHost, Table);
	return NewDataTableEditor;
}

TSharedRef<IDataTableEditor> FAnuDataTableEditorModule::CreateSnapshotDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	TSharedRef<FResourceSnapshotDataTableEditor> NewDataTableEditor(new FResourceSnapshotDataTableEditor());
	NewDataTableEditor->InitDataTableEditor(Mode, InitToolkitHost, Table);
	return NewDataTableEditor;
}