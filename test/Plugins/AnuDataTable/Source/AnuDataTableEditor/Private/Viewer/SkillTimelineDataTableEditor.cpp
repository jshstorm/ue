// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillTimelineDataTableEditor.h"
#include "Engine/DataTable.h"
#include "Reference_Skill.h"
#include "DataTableEditorUtils.h"

const FString FSkillTimelineDataTableEditor::SupportedDataTableName = "RefSkillTimeline";

void FSkillTimelineDataTableEditor::InitDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	FAnuDataTableEditor::InitDataTableEditor(Mode, InitToolkitHost, Table);
}

void FSkillTimelineDataTableEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	FAnuDataTableEditor::FillToolbar(ToolbarBuilder);

	ToolbarBuilder.BeginSection("ExtendCommands");
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FSkillTimelineDataTableEditor::OnRenameClicked)),
			NAME_None,
			FText::FromString("Sort"),
			FText::FromString(""),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Sort"));
	}
	ToolbarBuilder.EndSection();
}

void FSkillTimelineDataTableEditor::OnRenameClicked()
{
	auto dataTable = const_cast<UDataTable*>(GetDataTable());

	TArray<FRefSkillTimelineDataBase*> ptrs;
	dataTable->GetAllRows("", ptrs);

	TMap<FName, TArray<FRefSkillTimelineDataBase>> values;
	for (auto one : ptrs) {
		auto& entry = values.FindOrAdd(one->Skill_UID);
		entry.Emplace(*one);
	}
	dataTable->EmptyTable();

	for (auto& pair : values) {
		
		int32 index = 0;
		for (auto& one : pair.Value) {
			
			if (pair.Key != NAME_None) {
				one.UID = *FString::Printf(TEXT("%s.%02d"), *pair.Key.ToString(), ++index);
			}

			dataTable->AddRow(one.UID, one);
		}
	}

	dataTable->Modify();

	FDataTableEditorUtils::BroadcastPostChange(dataTable, FDataTableEditorUtils::EDataTableChangeInfo::RowList);
}