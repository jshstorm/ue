// Copyright Epic Games, Inc. All Rights Reserved.

#include "ResourceSnapshotDataTableEditor.h"
#include "DataTableEditorUtils.h"

#include "Reference_Resource.h"
#include "Engine/DataTable.h"

#include "UnrealEd/Public/ObjectTools.h"

const FString FResourceSnapshotDataTableEditor::SupportedDataTableName = "DT_ResourceSnapshot";
FResourceSnapshotDataTableEditor::FResourceSnapshotDataTableEditor()
	:FAnuDataTableEditor()
{

}

void FResourceSnapshotDataTableEditor::BindExport(FExportDelegate&& func)
{
	_exportEvent = MoveTemp(func);
}

void FResourceSnapshotDataTableEditor::InitDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	FAnuDataTableEditor::InitDataTableEditor(Mode, InitToolkitHost, Table);
}

void FResourceSnapshotDataTableEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	FAnuDataTableEditor::FillToolbar(ToolbarBuilder);

	ToolbarBuilder.BeginSection("ExtendCommands");
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FResourceSnapshotDataTableEditor::OnResetCheckBox)),
			NAME_None,
			FText::FromString("Reset Check box"),
			FText::FromString(""),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Reset"));

		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FResourceSnapshotDataTableEditor::OnExportCheckOnly)),
			NAME_None,
			FText::FromString("Export Check Only"),
			FText::FromString(""),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Export"));

		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FResourceSnapshotDataTableEditor::OnExportNew)),
			NAME_None,
			FText::FromString("Export New"),
			FText::FromString(""),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Export"));

		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FResourceSnapshotDataTableEditor::OnExportAll)),
			NAME_None,
			FText::FromString("Export All"),
			FText::FromString(""),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Export"));

	}
	ToolbarBuilder.EndSection();
}

void FResourceSnapshotDataTableEditor::OnDuplicateRow(const FName NewName)
{
	if (FAnuResourceSnapshot* resData = GetRowStructData(NewName)) {
		resData->ResultTexture.Empty();
	}
}

void FResourceSnapshotDataTableEditor::OnRenameOrDeleteRow(const FName NewName)
{
	if (NewName.IsNone()) {
		return;
	}

	if (FAnuResourceSnapshot* resData = GetRowStructData(NewName)) {
		resData->DeleteCachingTextures();
	}
}

void FResourceSnapshotDataTableEditor::OnResetCheckBox()
{
	VisitTableRow([](const FName& key, FAnuResourceSnapshot* snapshotData) {
		snapshotData->ExportTexture = false;
	});
}

void FResourceSnapshotDataTableEditor::OnExportCheckOnly()
{
	TArray<FSnapshotDataTableParam> output;
	VisitTableRow([&output](const FName& key, FAnuResourceSnapshot* snapshotData) {
		if (snapshotData->ExportTexture) {
			output.Emplace(FSnapshotDataTableParam{ key, snapshotData });
		}
	});

	_exportEvent.ExecuteIfBound(output);
}

void FResourceSnapshotDataTableEditor::OnExportNew()
{
	TArray<FSnapshotDataTableParam> output;
	VisitTableRow([&output](const FName& key, FAnuResourceSnapshot* snapshotData) {
		if (snapshotData->ResultTexture.Num() == 0) {
			output.Emplace(FSnapshotDataTableParam{ key, snapshotData });
			return;
		}

		for (auto& texture : snapshotData->ResultTexture) {
			if (texture.IsNull()) {
				output.Emplace(FSnapshotDataTableParam{ key, snapshotData });
				return;
			}
		}
	});

	_exportEvent.ExecuteIfBound(output);
}

void FResourceSnapshotDataTableEditor::OnExportAll()
{
	TArray<FSnapshotDataTableParam> output;
	VisitTableRow([&output](const FName& key, FAnuResourceSnapshot* snapshotData) {
		output.Emplace(FSnapshotDataTableParam{ key, snapshotData });
	});

	_exportEvent.ExecuteIfBound(output);
}

FAnuResourceSnapshot* FResourceSnapshotDataTableEditor::GetRowStructData(const FName& rowName)
{
	auto dataTable = const_cast<UDataTable*>(GetDataTable());
	if (FAnuResourceSnapshot* resData = dataTable->FindRow<FAnuResourceSnapshot>(rowName, TEXT(""))) {
		return resData;
	}
	return nullptr;
}

void FResourceSnapshotDataTableEditor::VisitTableRow(TFunction<void(const FName& key, FAnuResourceSnapshot*)>&& visitor)
{
	auto dataTable = const_cast<UDataTable*>(GetDataTable());
	dataTable->ForeachRow<FAnuResourceSnapshot>(TEXT(""), [visitor](const FName& key, const FAnuResourceSnapshot& value) {
		visitor(key, const_cast<FAnuResourceSnapshot*>(&value));
	});
}
