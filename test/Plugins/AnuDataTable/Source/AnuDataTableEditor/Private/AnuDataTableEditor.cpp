// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnuDataTableEditor.h"
#include "AnuDataTableEditorModule.h"
#include "DataTableEditorModule.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Layout/Overscroll.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDocumentation.h"
#include "Misc/FeedbackContext.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "ScopedTransaction.h"
#include "SAnuRowEditor.h"
#include "SAnuDataTableListViewRow.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Layout/SSeparator.h"
#include "SourceCodeNavigation.h"
#include "PropertyEditorModule.h"
#include "UObject/StructOnScope.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "Toolkits/AssetEditorManager.h"
#include "Engine/DataTable.h"
#include "Subsystems/AssetEditorSubsystem.h"

#define LOCTEXT_NAMESPACE "DataTableEditor"

const FName FAnuDataTableEditor::DataTableTabId("DataTableEditor_DataTable");
const FName FAnuDataTableEditor::DataTableDetailsTabId("DataTableEditor_DataTableDetails");
const FName FAnuDataTableEditor::RowEditorTabId("DataTableEditor_RowEditor");
const FName FAnuDataTableEditor::RowNameColumnId("RowName");
const FName FAnuDataTableEditor::RowNumberColumnId("RowNumber");
const FName FAnuDataTableEditor::RowDragDropColumnId("RowDragDrop");

class SDataTableModeSeparator : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SDataTableModeSeparator) {}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArg)
	{
		SBorder::Construct(
			SBorder::FArguments()
			.BorderImage(FEditorStyle::GetBrush("BlueprintEditor.PipelineSeparator"))
			.Padding(0.0f)
		);
	}

	// SWidget interface
	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const float Height = 20.0f;
		const float Thickness = 16.0f;
		return FVector2D(Thickness, Height);
	}
	// End of SWidget interface
};

void FAnuDataTableEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_Data Table Editor", "Data Table Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	CreateAndRegisterDataTableTab(InTabManager);
	CreateAndRegisterDataTableDetailsTab(InTabManager);
	CreateAndRegisterRowEditorTab(InTabManager);
}

void FAnuDataTableEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(DataTableTabId);
	InTabManager->UnregisterTabSpawner(DataTableDetailsTabId);
	InTabManager->UnregisterTabSpawner(RowEditorTabId);

	DataTableTabWidget.Reset();
	RowEditorTabWidget.Reset();
}

void FAnuDataTableEditor::CreateAndRegisterDataTableTab(const TSharedRef<class FTabManager>& InTabManager)
{
	DataTableTabWidget = CreateContentBox();

	InTabManager->RegisterTabSpawner(DataTableTabId, FOnSpawnTab::CreateSP(this, &FAnuDataTableEditor::SpawnTab_DataTable))
		.SetDisplayName(LOCTEXT("DataTableTab", "Data Table"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FAnuDataTableEditor::CreateAndRegisterDataTableDetailsTab(const TSharedRef<class FTabManager>& InTabManager)
{
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(/*bUpdateFromSelection=*/ false, /*bLockable=*/ false, /*bAllowSearch=*/ true, /*InNameAreaSettings=*/ FDetailsViewArgs::HideNameArea, /*bHideSelectionTip=*/ true);
	PropertyView = EditModule.CreateDetailView(DetailsViewArgs);

	InTabManager->RegisterTabSpawner(DataTableDetailsTabId, FOnSpawnTab::CreateSP(this, &FAnuDataTableEditor::SpawnTab_DataTableDetails))
		.SetDisplayName(LOCTEXT("DataTableDetailsTab", "Data Table Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FAnuDataTableEditor::CreateAndRegisterRowEditorTab(const TSharedRef<class FTabManager>& InTabManager)
{
	RowEditorTabWidget = CreateRowEditorBox();

	InTabManager->RegisterTabSpawner(RowEditorTabId, FOnSpawnTab::CreateSP(this, &FAnuDataTableEditor::SpawnTab_RowEditor))
		.SetDisplayName(LOCTEXT("RowEditorTab", "Row Editor"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

FAnuDataTableEditor::FAnuDataTableEditor()
	: RowNameColumnWidth(0)
	, RowNumberColumnWidth(0)
	, HighlightedVisibleRowIndex(INDEX_NONE)
	, SortMode(EColumnSortMode::Ascending)
{
}

FAnuDataTableEditor::~FAnuDataTableEditor()
{
	GEditor->UnregisterForUndo(this);

	UDataTable* Table = GetEditableDataTable();
	if (Table)
	{
		SaveLayoutData();
	}
}

void FAnuDataTableEditor::PostUndo(bool bSuccess)
{
	HandleUndoRedo();
}

void FAnuDataTableEditor::PostRedo(bool bSuccess)
{
	HandleUndoRedo();
}

void FAnuDataTableEditor::HandleUndoRedo()
{
	const UDataTable* Table = GetDataTable();
	if (Table)
	{
		HandlePostChange();
		CallbackOnDataTableUndoRedo.ExecuteIfBound();
	}
}

void FAnuDataTableEditor::PreChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info)
{
}

void FAnuDataTableEditor::PostChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info)
{
	const UDataTable* Table = GetDataTable();
	if (Struct && Table && (Table->GetRowStruct() == Struct))
	{
		HandlePostChange();
	}
}

void FAnuDataTableEditor::SelectionChange(const UDataTable* Changed, FName RowName)
{
	const UDataTable* Table = GetDataTable();
	if (Changed == Table)
	{
		const bool bSelectionChanged = HighlightedRowName != RowName;
		SetHighlightedRow(RowName);

		if (bSelectionChanged)
		{
			CallbackOnRowHighlighted.ExecuteIfBound(HighlightedRowName);
		}
	}
}

void FAnuDataTableEditor::PreChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
}

void FAnuDataTableEditor::PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
	UDataTable* Table = GetEditableDataTable();
	if (Changed == Table)
	{
		// Don't need to notify the DataTable about changes, that's handled before this
		HandlePostChange();
	}
}

const UDataTable* FAnuDataTableEditor::GetDataTable() const
{
	return Cast<const UDataTable>(GetEditingObject());
}

void FAnuDataTableEditor::HandlePostChange()
{
	// We need to cache and restore the selection here as RefreshCachedDataTable will re-create the list view items
	const FName CachedSelection = HighlightedRowName;
	HighlightedRowName = NAME_None;
	RefreshCachedDataTable(CachedSelection, true/*bUpdateEvenIfValid*/);
}

void FAnuDataTableEditor::InitDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_DataTableEditor_Layout_v5")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(DataTableTabId, ETabState::OpenedTab)
				->AddTab(DataTableDetailsTabId, ETabState::OpenedTab)
				->SetForegroundTab(DataTableTabId)
			)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(RowEditorTabId, ETabState::OpenedTab)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, FAnuDataTableEditorModule::AnuDataTableEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Table);

	FDataTableEditorModule& DataTableEditorModule = FModuleManager::LoadModuleChecked<FDataTableEditorModule>("DataTableEditor");
	AddMenuExtender(DataTableEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	TSharedPtr<FExtender> ToolbarExtender = DataTableEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects());
	ExtendToolbar(ToolbarExtender);

	AddToolbarExtender(ToolbarExtender);

	RegenerateMenusAndToolbars();

	// Support undo/redo
	GEditor->RegisterForUndo(this);

	// @todo toolkit world centric editing
	/*// Setup our tool's layout
	if( IsWorldCentricAssetEditor() )
	{
		const FString TabInitializationPayload(TEXT(""));		// NOTE: Payload not currently used for table properties
		SpawnToolkitTab( DataTableTabId, TabInitializationPayload, EToolkitTabSpot::Details );
	}*/

	// asset editor commands here
	ToolkitCommands->MapAction(FGenericCommands::Get().Copy, FExecuteAction::CreateSP(this, &FAnuDataTableEditor::CopySelectedRow));
	ToolkitCommands->MapAction(FGenericCommands::Get().Paste, FExecuteAction::CreateSP(this, &FAnuDataTableEditor::PasteOnSelectedRow));
	ToolkitCommands->MapAction(FGenericCommands::Get().Duplicate, FExecuteAction::CreateSP(this, &FAnuDataTableEditor::DuplicateSelectedRow));
	ToolkitCommands->MapAction(FGenericCommands::Get().Rename, FExecuteAction::CreateSP(this, &FAnuDataTableEditor::RenameSelectedRowCommand));
	ToolkitCommands->MapAction(FGenericCommands::Get().Delete, FExecuteAction::CreateSP(this, &FAnuDataTableEditor::DeleteSelectedRow));
}

#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25)
bool FAnuDataTableEditor::CanEditRows() const
{
	return true;
}
#endif
FName FAnuDataTableEditor::GetToolkitFName() const
{
	return FName("DataTableEditor");
}

FString FAnuDataTableEditor::GetDocumentationLink() const
{
	return FString(TEXT("Gameplay/DataDriven"));
}

void FAnuDataTableEditor::OnAddClicked()
{
	UDataTable* Table = GetEditableDataTable();

	if (Table)
	{
		FName NewName = DataTableUtils::MakeValidName(TEXT("NewRow"));
		while (Table->GetRowMap().Contains(NewName))
		{
			NewName.SetNumber(NewName.GetNumber() + 1);
		}

		FDataTableEditorUtils::AddRow(Table, NewName);
		FDataTableEditorUtils::SelectRow(Table, NewName);

		SetDefaultSort();
	}
}

void FAnuDataTableEditor::OnRemoveClicked()
{
	DeleteSelectedRow();
}

FReply FAnuDataTableEditor::OnMoveRowClicked(FDataTableEditorUtils::ERowMoveDirection MoveDirection)
{
	UDataTable* Table = GetEditableDataTable();

	if (Table)
	{
		FDataTableEditorUtils::MoveRow(Table, HighlightedRowName, MoveDirection);
	}
	return FReply::Handled();
}

FReply FAnuDataTableEditor::OnMoveToExtentClicked(FDataTableEditorUtils::ERowMoveDirection MoveDirection)
{
	UDataTable* Table = GetEditableDataTable();

	if (Table)
	{
		// We move by the row map size, as FDataTableEditorUtils::MoveRow will automatically clamp this as appropriate
		FDataTableEditorUtils::MoveRow(Table, HighlightedRowName, MoveDirection, Table->GetRowMap().Num());
		FDataTableEditorUtils::SelectRow(Table, HighlightedRowName);

		SetDefaultSort();
	}
	return FReply::Handled();
}

void FAnuDataTableEditor::OnCopyClicked()
{
	UDataTable* Table = GetEditableDataTable();
	if (Table)
	{
		CopySelectedRow();
	}
}

void FAnuDataTableEditor::OnPasteClicked()
{
	UDataTable* Table = GetEditableDataTable();
	if (Table)
	{
		PasteOnSelectedRow();
	}
}

void FAnuDataTableEditor::OnDuplicateClicked()
{
	UDataTable* Table = GetEditableDataTable();
	if (Table)
	{
		DuplicateSelectedRow();
	}
}

void FAnuDataTableEditor::SetDefaultSort()
{
	SortMode = EColumnSortMode::Ascending;
	SortByColumn = FAnuDataTableEditor::RowNumberColumnId;
}

EColumnSortMode::Type FAnuDataTableEditor::GetColumnSortMode(const FName ColumnId) const
{
	if (SortByColumn != ColumnId)
	{
		return EColumnSortMode::None;
	}

	return SortMode;
}

void FAnuDataTableEditor::OnColumnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
{
	int32 ColumnIndex;

	SortMode = InSortMode;
	SortByColumn = ColumnId;

	for (ColumnIndex = 0; ColumnIndex < AvailableColumns.Num(); ++ColumnIndex)
	{
		if (AvailableColumns[ColumnIndex]->ColumnId == ColumnId)
		{
			break;
		}
	}

	if (AvailableColumns.IsValidIndex(ColumnIndex))
	{
		if (InSortMode == EColumnSortMode::Ascending)
		{
			VisibleRows.Sort([ColumnIndex](const FDataTableEditorRowListViewDataPtr& first, const FDataTableEditorRowListViewDataPtr& second)
				{
					int32 Result = (first->CellData[ColumnIndex].ToString()).Compare(second->CellData[ColumnIndex].ToString());

					if (!Result)
					{
						return first->RowNum < second->RowNum;

					}

					return Result < 0;
				});

		}
		else if (InSortMode == EColumnSortMode::Descending)
		{
			VisibleRows.Sort([ColumnIndex](const FDataTableEditorRowListViewDataPtr& first, const FDataTableEditorRowListViewDataPtr& second)
				{
					int32 Result = (first->CellData[ColumnIndex].ToString()).Compare(second->CellData[ColumnIndex].ToString());

					if (!Result)
					{
						return first->RowNum > second->RowNum;
					}

					return Result > 0;
				});
		}
	}

	CellsListView->RequestListRefresh();
}

void FAnuDataTableEditor::OnColumnNumberSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
{
	SortMode = InSortMode;
	SortByColumn = ColumnId;

	if (InSortMode == EColumnSortMode::Ascending)
	{
		VisibleRows.Sort([](const FDataTableEditorRowListViewDataPtr& first, const FDataTableEditorRowListViewDataPtr& second)
			{
				return first->RowNum < second->RowNum;
			});
	}
	else if (InSortMode == EColumnSortMode::Descending)
	{
		VisibleRows.Sort([](const FDataTableEditorRowListViewDataPtr& first, const FDataTableEditorRowListViewDataPtr& second)
			{
				return first->RowNum > second->RowNum;
			});
	}

	CellsListView->RequestListRefresh();
}

void FAnuDataTableEditor::OnColumnNameSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
{
	SortMode = InSortMode;
	SortByColumn = ColumnId;

	if (InSortMode == EColumnSortMode::Ascending)
	{
		VisibleRows.Sort([](const FDataTableEditorRowListViewDataPtr& first, const FDataTableEditorRowListViewDataPtr& second)
			{
				return (first->DisplayName).ToString() < (second->DisplayName).ToString();
			});
	}
	else if (InSortMode == EColumnSortMode::Descending)
	{
		VisibleRows.Sort([](const FDataTableEditorRowListViewDataPtr& first, const FDataTableEditorRowListViewDataPtr& second)
			{
				return (first->DisplayName).ToString() > (second->DisplayName).ToString();
			});
	}

	CellsListView->RequestListRefresh();
}

void FAnuDataTableEditor::OnEditDataTableStructClicked()
{
	const UDataTable* DataTable = GetDataTable();
	if (DataTable)
	{

		const UScriptStruct* ScriptStruct = DataTable->GetRowStruct();

		if (ScriptStruct)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(ScriptStruct->GetPathName());
			FSourceCodeNavigation::NavigateToStruct(ScriptStruct);
		}
	}
}

void FAnuDataTableEditor::ExtendToolbar(TSharedPtr<FExtender> Extender)
{
	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FAnuDataTableEditor::FillToolbar)
	);

}

void FAnuDataTableEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection("DataTableCommands");
	{
		ToolbarBuilder.AddToolBarButton(
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25)
			FUIAction(
				FExecuteAction::CreateSP(this, &FAnuDataTableEditor::Reimport_Execute),
				FCanExecuteAction::CreateSP(this, &FAnuDataTableEditor::CanReimport)),
			NAME_None,
			LOCTEXT("ReimportText", "Reimport"),
			LOCTEXT("ReimportTooltip", "Reimport this DataTable"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "AssetEditor.ReimportAsset"));

		ToolbarBuilder.AddSeparator();

		ToolbarBuilder.AddToolBarButton(
#endif
			FUIAction(FExecuteAction::CreateSP(this, &FAnuDataTableEditor::OnAddClicked)),
			NAME_None,
			LOCTEXT("AddIconText", "Add"),
			LOCTEXT("AddRowToolTip", "Add a new row to the Data Table"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Add"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FAnuDataTableEditor::OnCopyClicked)),
			NAME_None,
			LOCTEXT("CopyIconText", "Copy"),
			LOCTEXT("CopyToolTip", "Copy the currently selected row"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Copy"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FAnuDataTableEditor::OnPasteClicked)),
			NAME_None,
			LOCTEXT("PasteIconText", "Paste"),
			LOCTEXT("PasteToolTip", "Paste on the currently selected row"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Paste"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FAnuDataTableEditor::OnDuplicateClicked)),
			NAME_None,
			LOCTEXT("DuplicateIconText", "Duplicate"),
			LOCTEXT("DuplicateToolTip", "Duplicate the currently selected row"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Duplicate"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FAnuDataTableEditor::OnRemoveClicked)),
			NAME_None,
			LOCTEXT("RemoveRowIconText", "Remove"),
			LOCTEXT("RemoveRowToolTip", "Remove the currently selected row from the Data Table"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Remove"));
	}
	ToolbarBuilder.EndSection();

}

UDataTable* FAnuDataTableEditor::GetEditableDataTable() const
{
	return Cast<UDataTable>(GetEditingObject());
}

FText FAnuDataTableEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "DataTable Editor");
}

FString FAnuDataTableEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "DataTable ").ToString();
}

FLinearColor FAnuDataTableEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.0f, 0.0f, 0.2f, 0.5f);
}

FSlateColor FAnuDataTableEditor::GetRowTextColor(FName RowName) const
{
	if (RowName == HighlightedRowName)
	{
		return FSlateColor(FColorList::Orange);
	}
	return FSlateColor::UseForeground();
}

FText FAnuDataTableEditor::GetCellText(FDataTableEditorRowListViewDataPtr InRowDataPointer, int32 ColumnIndex) const
{
	if (InRowDataPointer.IsValid() && ColumnIndex < InRowDataPointer->CellData.Num())
	{
		return InRowDataPointer->CellData[ColumnIndex];
	}

	return FText();
}

FText FAnuDataTableEditor::GetCellToolTipText(FDataTableEditorRowListViewDataPtr InRowDataPointer, int32 ColumnIndex) const
{
	FText TooltipText;

	if (ColumnIndex < AvailableColumns.Num())
	{
		TooltipText = AvailableColumns[ColumnIndex]->DisplayName;
	}

	if (InRowDataPointer.IsValid() && ColumnIndex < InRowDataPointer->CellData.Num())
	{
		TooltipText = FText::Format(LOCTEXT("ColumnRowNameFmt", "{0}: {1}"), TooltipText, InRowDataPointer->CellData[ColumnIndex]);
	}

	return TooltipText;
}

float FAnuDataTableEditor::GetRowNumberColumnWidth() const
{
	return RowNumberColumnWidth;
}

void FAnuDataTableEditor::RefreshRowNumberColumnWidth()
{

	TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FTextBlockStyle& CellTextStyle = FEditorStyle::GetWidgetStyle<FTextBlockStyle>("DataTableEditor.CellText");
	const float CellPadding = 10.0f;

	for (const FDataTableEditorRowListViewDataPtr& RowData : AvailableRows)
	{
		const float RowNumberWidth = FontMeasure->Measure(FString::FromInt(RowData->RowNum), CellTextStyle.Font).X + CellPadding;
		RowNumberColumnWidth = FMath::Max(RowNumberColumnWidth, RowNumberWidth);
	}

}

void FAnuDataTableEditor::OnRowNumberColumnResized(const float NewWidth)
{
	RowNumberColumnWidth = NewWidth;
}

float FAnuDataTableEditor::GetRowNameColumnWidth() const
{
	return RowNameColumnWidth;
}

void FAnuDataTableEditor::RefreshRowNameColumnWidth()
{

	TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FTextBlockStyle& CellTextStyle = FEditorStyle::GetWidgetStyle<FTextBlockStyle>("DataTableEditor.CellText");
	static const float CellPadding = 10.0f;

	for (const FDataTableEditorRowListViewDataPtr& RowData : AvailableRows)
	{
		const float RowNameWidth = FontMeasure->Measure(RowData->DisplayName, CellTextStyle.Font).X + CellPadding;
		RowNameColumnWidth = FMath::Max(RowNameColumnWidth, RowNameWidth);
	}

}

float FAnuDataTableEditor::GetColumnWidth(const int32 ColumnIndex) const
{
	if (ColumnWidths.IsValidIndex(ColumnIndex))
	{
		return ColumnWidths[ColumnIndex].CurrentWidth;
	}
	return 0.0f;
}

void FAnuDataTableEditor::OnColumnResized(const float NewWidth, const int32 ColumnIndex)
{
	if (ColumnWidths.IsValidIndex(ColumnIndex))
	{
		FColumnWidth& ColumnWidth = ColumnWidths[ColumnIndex];
		ColumnWidth.bIsAutoSized = false;
		ColumnWidth.CurrentWidth = NewWidth;

		// Update the persistent column widths in the layout data
		{
			if (!LayoutData.IsValid())
			{
				LayoutData = MakeShared<FJsonObject>();
			}

			TSharedPtr<FJsonObject> LayoutColumnWidths;
			if (!LayoutData->HasField(TEXT("ColumnWidths")))
			{
				LayoutColumnWidths = MakeShared<FJsonObject>();
				LayoutData->SetObjectField(TEXT("ColumnWidths"), LayoutColumnWidths);
			}
			else
			{
				LayoutColumnWidths = LayoutData->GetObjectField(TEXT("ColumnWidths"));
			}

			const FString& ColumnName = AvailableColumns[ColumnIndex]->ColumnId.ToString();
			LayoutColumnWidths->SetNumberField(ColumnName, NewWidth);
		}
	}
}

void FAnuDataTableEditor::OnRowNameColumnResized(const float NewWidth)
{
	RowNameColumnWidth = NewWidth;
}

void FAnuDataTableEditor::LoadLayoutData()
{
	LayoutData.Reset();

	const UDataTable* Table = GetDataTable();
	if (!Table)
	{
		return;
	}

	const FString LayoutDataFilename = FPaths::ProjectSavedDir() / TEXT("AssetData") / TEXT("DataTableEditorLayout") / Table->GetName() + TEXT(".json");

	FString JsonText;
	if (FFileHelper::LoadFileToString(JsonText, *LayoutDataFilename))
	{
		TSharedRef< TJsonReader<TCHAR> > JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonText);
		FJsonSerializer::Deserialize(JsonReader, LayoutData);
	}
}

void FAnuDataTableEditor::SaveLayoutData()
{
	const UDataTable* Table = GetDataTable();
	if (!Table || !LayoutData.IsValid())
	{
		return;
	}

	const FString LayoutDataFilename = FPaths::ProjectSavedDir() / TEXT("AssetData") / TEXT("DataTableEditorLayout") / Table->GetName() + TEXT(".json");

	FString JsonText;
	TSharedRef< TJsonWriter< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > > JsonWriter = TJsonWriterFactory< TCHAR, TPrettyJsonPrintPolicy<TCHAR> >::Create(&JsonText);
	if (FJsonSerializer::Serialize(LayoutData.ToSharedRef(), JsonWriter))
	{
		FFileHelper::SaveStringToFile(JsonText, *LayoutDataFilename);
	}
}

TSharedRef<ITableRow> FAnuDataTableEditor::MakeRowWidget(FDataTableEditorRowListViewDataPtr InRowDataPtr, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(SAnuDataTableListViewRow, OwnerTable)
		.AnuDataTableEditor(SharedThis(this))
		.RowDataPtr(InRowDataPtr)
		.IsEditable(CanEditRows());
}

TSharedRef<SWidget> FAnuDataTableEditor::MakeCellWidget(FDataTableEditorRowListViewDataPtr InRowDataPtr, const int32 InRowIndex, const FName& InColumnId)
{
	int32 ColumnIndex = 0;
	for (; ColumnIndex < AvailableColumns.Num(); ++ColumnIndex)
	{
		const FDataTableEditorColumnHeaderDataPtr& ColumnData = AvailableColumns[ColumnIndex];
		if (ColumnData->ColumnId == InColumnId)
		{
			break;
		}
	}

	// Valid column ID?
	if (AvailableColumns.IsValidIndex(ColumnIndex) && InRowDataPtr->CellData.IsValidIndex(ColumnIndex))
	{
		return SNew(SBox)
			.Padding(FMargin(4, 2, 4, 2))
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "DataTableEditor.CellText")
			.ColorAndOpacity(this, &FAnuDataTableEditor::GetRowTextColor, InRowDataPtr->RowId)
			.Text(this, &FAnuDataTableEditor::GetCellText, InRowDataPtr, ColumnIndex)
			.HighlightText(this, &FAnuDataTableEditor::GetFilterText)
			.ToolTipText(this, &FAnuDataTableEditor::GetCellToolTipText, InRowDataPtr, ColumnIndex)
			];
	}

	return SNullWidget::NullWidget;
}

void FAnuDataTableEditor::OnRowSelectionChanged(FDataTableEditorRowListViewDataPtr InNewSelection, ESelectInfo::Type InSelectInfo)
{
	const bool bSelectionChanged = !InNewSelection.IsValid() || InNewSelection->RowId != HighlightedRowName;
	const FName NewRowName = (InNewSelection.IsValid()) ? InNewSelection->RowId : NAME_None;

	SetHighlightedRow(NewRowName);

	if (bSelectionChanged)
	{
		CallbackOnRowHighlighted.ExecuteIfBound(HighlightedRowName);
	}
}

void FAnuDataTableEditor::CopySelectedRow()
{
	UDataTable* TablePtr = Cast<UDataTable>(GetEditingObject());
	uint8* RowPtr = TablePtr ? TablePtr->GetRowMap().FindRef(HighlightedRowName) : nullptr;

	if (!RowPtr || !TablePtr->RowStruct)
		return;

	FString ClipboardValue;
	TablePtr->RowStruct->ExportText(ClipboardValue, RowPtr, RowPtr, TablePtr, PPF_Copy, nullptr);

	FPlatformApplicationMisc::ClipboardCopy(*ClipboardValue);
}

void FAnuDataTableEditor::PasteOnSelectedRow()
{
	UDataTable* TablePtr = Cast<UDataTable>(GetEditingObject());
	uint8* RowPtr = TablePtr ? TablePtr->GetRowMap().FindRef(HighlightedRowName) : nullptr;

	if (!RowPtr || !TablePtr->RowStruct)
		return;

	const FScopedTransaction Transaction(LOCTEXT("PasteDataTableRow", "Paste Data Table Row"));
	TablePtr->Modify();

	FString ClipboardValue;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardValue);

	FDataTableEditorUtils::BroadcastPreChange(TablePtr, FDataTableEditorUtils::EDataTableChangeInfo::RowData);

	const TCHAR* Result = TablePtr->RowStruct->ImportText(*ClipboardValue, RowPtr, TablePtr, PPF_Copy, GWarn, GetPathNameSafe(TablePtr->RowStruct));

#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25)
	TablePtr->HandleDataTableChanged(HighlightedRowName);
	TablePtr->MarkPackageDirty();
#endif

	FDataTableEditorUtils::BroadcastPostChange(TablePtr, FDataTableEditorUtils::EDataTableChangeInfo::RowData);

	if (Result == nullptr)
	{
		FNotificationInfo Info(LOCTEXT("FailedPaste", "Failed to paste row"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void FAnuDataTableEditor::DuplicateSelectedRow()
{
	UDataTable* TablePtr = Cast<UDataTable>(GetEditingObject());
	FName NewName = HighlightedRowName;

	if (NewName == NAME_None || TablePtr == nullptr)
	{
		return;
	}

	const TArray<FName> ExistingNames = TablePtr->GetRowNames();
	while (ExistingNames.Contains(NewName))
	{
		NewName.SetNumber(NewName.GetNumber() + 1);
	}

	FDataTableEditorUtils::DuplicateRow(TablePtr, HighlightedRowName, NewName);
	FDataTableEditorUtils::SelectRow(TablePtr, NewName);
	OnDuplicateRow(NewName);
}

void FAnuDataTableEditor::RenameSelectedRowCommand()
{
	UDataTable* TablePtr = Cast<UDataTable>(GetEditingObject());
	FName NewName = HighlightedRowName;

	if (NewName == NAME_None || TablePtr == nullptr)
	{
		return;
	}

	if (VisibleRows.IsValidIndex(HighlightedVisibleRowIndex))
	{
		TSharedPtr< SAnuDataTableListViewRow > RowWidget = StaticCastSharedPtr< SAnuDataTableListViewRow >(CellsListView->WidgetFromItem(VisibleRows[HighlightedVisibleRowIndex]));
		RowWidget->SetRowForRename();
	}
}

void FAnuDataTableEditor::DeleteSelectedRow()
{
	if (UDataTable* Table = GetEditableDataTable())
	{
		// We must perform this before removing the row
		const int32 RowToRemoveIndex = VisibleRows.IndexOfByPredicate([&](const FDataTableEditorRowListViewDataPtr& InRowName) -> bool
			{
				return InRowName->RowId == HighlightedRowName;
			});

		OnRenameOrDeleteRow(HighlightedRowName);

		// Remove row
		if (FDataTableEditorUtils::RemoveRow(Table, HighlightedRowName))
		{
			// Try and keep the same row index selected
			const int32 RowIndexToSelect = FMath::Clamp(RowToRemoveIndex, 0, VisibleRows.Num() - 1);
			if (VisibleRows.IsValidIndex(RowIndexToSelect))
			{
				FDataTableEditorUtils::SelectRow(Table, VisibleRows[RowIndexToSelect]->RowId);
			}
			// Refresh list. Otherwise, the removed row would still appear in the screen until the next list refresh. An
			// analog of CellsListView->RequestListRefresh() also occurs inside FDataTableEditorUtils::SelectRow
			else
			{
				CellsListView->RequestListRefresh();
			}
		}
	}
}

FText FAnuDataTableEditor::GetFilterText() const
{
	return ActiveFilterText;
}

void FAnuDataTableEditor::OnFilterTextChanged(const FText& InFilterText)
{
	ActiveFilterText = InFilterText;
	UpdateVisibleRows();
}

void FAnuDataTableEditor::OnFilterTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnCleared)
	{
		SearchBoxWidget->SetText(FText::GetEmpty());
		OnFilterTextChanged(FText::GetEmpty());
	}
}

void FAnuDataTableEditor::PostRegenerateMenusAndToolbars()
{
	const UDataTable* DataTable = GetDataTable();

	if (DataTable)
	{
		const UUserDefinedStruct* UDS = Cast<const UUserDefinedStruct>(DataTable->GetRowStruct());

		// build and attach the menu overlay
		TSharedRef<SHorizontalBox> MenuOverlayBox = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.ShadowOffset(FVector2D::UnitVector)
			.Text(LOCTEXT("DataTableEditor_RowStructType", "Row Type: "))
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SHyperlink)
				.Style(FEditorStyle::Get(), "Common.GotoNativeCodeHyperlink")
			.OnNavigate(this, &FAnuDataTableEditor::OnEditDataTableStructClicked)
			.Text(FText::FromName(DataTable->GetRowStructName()))
			.ToolTipText(LOCTEXT("DataTableRowToolTip", "Open the struct used for each row in this data table"))
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.VAlign(VAlign_Center)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.OnClicked(this, &FAnuDataTableEditor::OnFindRowInContentBrowserClicked)
			.Visibility(UDS ? EVisibility::Visible : EVisibility::Collapsed)
			.ToolTipText(LOCTEXT("FindRowInCBToolTip", "Find struct in Content Browser"))
			.ContentPadding(4.0f)
			.ForegroundColor(FSlateColor::UseForeground())
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Browse"))
			]
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SHyperlink)
				.Style(FEditorStyle::Get(), "Common.GotoNativeCodeHyperlink")
			.Visibility(!UDS ? EVisibility::Visible : EVisibility::Collapsed)
			.OnNavigate(this, &FAnuDataTableEditor::OnNavigateToDataTableRowCode)
			.Text(FText::FromName(DataTable->GetRowStructName()))
			.ToolTipText(FText::Format(LOCTEXT("GoToCode_ToolTip", "Click to open this source file in {0}"), FSourceCodeNavigation::GetSelectedSourceCodeIDE()))
			];

		SetMenuOverlay(MenuOverlayBox);
	}
}

FReply FAnuDataTableEditor::OnFindRowInContentBrowserClicked()
{
	const UDataTable* DataTable = GetDataTable();
	if (DataTable)
	{
		TArray<FAssetData> ObjectsToSync;
		ObjectsToSync.Add(FAssetData(DataTable->GetRowStruct()));
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}

	return FReply::Handled();
}

void FAnuDataTableEditor::OnNavigateToDataTableRowCode()
{
	const UDataTable* DataTable = GetDataTable();
	if (DataTable && FSourceCodeNavigation::CanNavigateToStruct(DataTable->GetRowStruct()))
	{
		FSourceCodeNavigation::NavigateToStruct(DataTable->GetRowStruct());
	}
}

void FAnuDataTableEditor::RefreshCachedDataTable(const FName InCachedSelection, const bool bUpdateEvenIfValid)
{
	UDataTable* Table = GetEditableDataTable();
	TArray<FDataTableEditorColumnHeaderDataPtr> PreviousColumns = AvailableColumns;

	FDataTableEditorUtils::CacheDataTableForEditing(Table, AvailableColumns, AvailableRows);

	// Update the desired width of the row names and numbers column
	// This prevents it growing or shrinking as you scroll the list view
	RefreshRowNumberColumnWidth();
	RefreshRowNameColumnWidth();

	// Setup the default auto-sized columns
	ColumnWidths.SetNum(AvailableColumns.Num());
	for (int32 ColumnIndex = 0; ColumnIndex < AvailableColumns.Num(); ++ColumnIndex)
	{
		const FDataTableEditorColumnHeaderDataPtr& ColumnData = AvailableColumns[ColumnIndex];
		FColumnWidth& ColumnWidth = ColumnWidths[ColumnIndex];
		ColumnWidth.CurrentWidth = FMath::Clamp(ColumnData->DesiredColumnWidth, 10.0f, 400.0f); // Clamp auto-sized columns to a reasonable limit
	}

	// Load the persistent column widths from the layout data
	{
		const TSharedPtr<FJsonObject>* LayoutColumnWidths = nullptr;
		if (LayoutData.IsValid() && LayoutData->TryGetObjectField(TEXT("ColumnWidths"), LayoutColumnWidths))
		{
			for (int32 ColumnIndex = 0; ColumnIndex < AvailableColumns.Num(); ++ColumnIndex)
			{
				const FDataTableEditorColumnHeaderDataPtr& ColumnData = AvailableColumns[ColumnIndex];

				double LayoutColumnWidth = 0.0f;
				if ((*LayoutColumnWidths)->TryGetNumberField(ColumnData->ColumnId.ToString(), LayoutColumnWidth))
				{
					FColumnWidth& ColumnWidth = ColumnWidths[ColumnIndex];
					ColumnWidth.bIsAutoSized = false;
					ColumnWidth.CurrentWidth = static_cast<float>(LayoutColumnWidth);
				}
			}
		}
	}

	if (PreviousColumns != AvailableColumns)
	{
		ColumnNamesHeaderRow->ClearColumns();

		if (CanEditRows())
		{
			ColumnNamesHeaderRow->AddColumn(
				SHeaderRow::Column(RowDragDropColumnId)
				[
					SNew(SBox)
					.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.ToolTip(IDocumentation::Get()->CreateToolTip(
					LOCTEXT("DataTableRowHandleTooltip", "Drag Drop Handles"),
					nullptr,
					*FDataTableEditorUtils::VariableTypesTooltipDocLink,
					TEXT("DataTableRowHandle")))
				[
					SNew(STextBlock)
					.Text(FText::GetEmpty())
				]
				]
			);
		}

		ColumnNamesHeaderRow->AddColumn(
			SHeaderRow::Column(RowNumberColumnId)
			.SortMode(this, &FAnuDataTableEditor::GetColumnSortMode, RowNumberColumnId)
			.OnSort(this, &FAnuDataTableEditor::OnColumnNumberSortModeChanged)
			.ManualWidth(this, &FAnuDataTableEditor::GetRowNumberColumnWidth)
			.OnWidthChanged(this, &FAnuDataTableEditor::OnRowNumberColumnResized)
			[
				SNew(SBox)
				.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			.ToolTip(IDocumentation::Get()->CreateToolTip(
				LOCTEXT("DataTableRowIndexTooltip", "Row Index"),
				nullptr,
				*FDataTableEditorUtils::VariableTypesTooltipDocLink,
				TEXT("DataTableRowIndex")))
			[
				SNew(STextBlock)
				.Text(FText::GetEmpty())
			]
			]

		);

		ColumnNamesHeaderRow->AddColumn(
			SHeaderRow::Column(RowNameColumnId)
			.DefaultLabel(LOCTEXT("DataTableRowName", "Row Name"))
			.ManualWidth(this, &FAnuDataTableEditor::GetRowNameColumnWidth)
			.OnWidthChanged(this, &FAnuDataTableEditor::OnRowNameColumnResized)
			.SortMode(this, &FAnuDataTableEditor::GetColumnSortMode, RowNameColumnId)
			.OnSort(this, &FAnuDataTableEditor::OnColumnNameSortModeChanged)
		);

		for (int32 ColumnIndex = 0; ColumnIndex < AvailableColumns.Num(); ++ColumnIndex)
		{
			const FDataTableEditorColumnHeaderDataPtr& ColumnData = AvailableColumns[ColumnIndex];

			ColumnNamesHeaderRow->AddColumn(
				SHeaderRow::Column(ColumnData->ColumnId)
				.DefaultLabel(ColumnData->DisplayName)
				.ManualWidth(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateSP(this, &FAnuDataTableEditor::GetColumnWidth, ColumnIndex)))
				.OnWidthChanged(this, &FAnuDataTableEditor::OnColumnResized, ColumnIndex)
				.SortMode(this, &FAnuDataTableEditor::GetColumnSortMode, ColumnData->ColumnId)
				.OnSort(this, &FAnuDataTableEditor::OnColumnSortModeChanged)
				[
					SNew(SBox)
					.Padding(FMargin(0, 4, 0, 4))
				.VAlign(VAlign_Fill)
				.ToolTip(IDocumentation::Get()->CreateToolTip(FDataTableEditorUtils::GetRowTypeInfoTooltipText(ColumnData), nullptr, *FDataTableEditorUtils::VariableTypesTooltipDocLink, FDataTableEditorUtils::GetRowTypeTooltipDocExcerptName(ColumnData)))
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
				.Text(ColumnData->DisplayName)
				]
				]
			);
		}
	}

	UpdateVisibleRows(InCachedSelection, bUpdateEvenIfValid);

	if (PropertyView.IsValid())
	{
		PropertyView->SetObject(Table);
	}
}

void FAnuDataTableEditor::UpdateVisibleRows(const FName InCachedSelection, const bool bUpdateEvenIfValid)
{
	if (ActiveFilterText.IsEmptyOrWhitespace())
	{
		VisibleRows = AvailableRows;
	}
	else
	{
		VisibleRows.Empty(AvailableRows.Num());

		const FString& ActiveFilterString = ActiveFilterText.ToString();
		for (const FDataTableEditorRowListViewDataPtr& RowData : AvailableRows)
		{
			bool bPassesFilter = false;

			if (RowData->DisplayName.ToString().Contains(ActiveFilterString))
			{
				bPassesFilter = true;
			}
			else
			{
				for (const FText& CellText : RowData->CellData)
				{
					if (CellText.ToString().Contains(ActiveFilterString))
					{
						bPassesFilter = true;
						break;
					}
				}
			}

			if (bPassesFilter)
			{
				VisibleRows.Add(RowData);
			}
		}
	}

	CellsListView->RequestListRefresh();
	RestoreCachedSelection(InCachedSelection, bUpdateEvenIfValid);
}

void FAnuDataTableEditor::RestoreCachedSelection(const FName InCachedSelection, const bool bUpdateEvenIfValid)
{
	// Validate the requested selection to see if it matches a known row
	bool bSelectedRowIsValid = false;
	if (!InCachedSelection.IsNone())
	{
		bSelectedRowIsValid = VisibleRows.ContainsByPredicate([&InCachedSelection](const FDataTableEditorRowListViewDataPtr& RowData) -> bool
			{
				return RowData->RowId == InCachedSelection;
			});
	}

	// Apply the new selection (if required)
	if (!bSelectedRowIsValid)
	{
		FName rowName = (VisibleRows.Num() > 0) ? VisibleRows[0]->RowId : NAME_None;
		SetHighlightedRow(rowName);
		CallbackOnRowHighlighted.ExecuteIfBound(HighlightedRowName);
	}
	else if (bUpdateEvenIfValid)
	{
		SetHighlightedRow(InCachedSelection);
		CallbackOnRowHighlighted.ExecuteIfBound(HighlightedRowName);
	}
}

TSharedRef<SVerticalBox> FAnuDataTableEditor::CreateContentBox()
{
	TSharedRef<SScrollBar> HorizontalScrollBar = SNew(SScrollBar)
		.Orientation(Orient_Horizontal)
		.Thickness(FVector2D(12.0f, 12.0f));

	TSharedRef<SScrollBar> VerticalScrollBar = SNew(SScrollBar)
		.Orientation(Orient_Vertical)
		.Thickness(FVector2D(12.0f, 12.0f));

	ColumnNamesHeaderRow = SNew(SHeaderRow);

	CellsListView = SNew(SListView<FDataTableEditorRowListViewDataPtr>)
		.ListItemsSource(&VisibleRows)
		.HeaderRow(ColumnNamesHeaderRow)
		.OnGenerateRow(this, &FAnuDataTableEditor::MakeRowWidget)
		.OnSelectionChanged(this, &FAnuDataTableEditor::OnRowSelectionChanged)
		.ExternalScrollbar(VerticalScrollBar)
		.ConsumeMouseWheel(EConsumeMouseWheel::Always)
		.SelectionMode(ESelectionMode::Single)
		.AllowOverscroll(EAllowOverscroll::No);

	LoadLayoutData();
	RefreshCachedDataTable();

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		[
			SAssignNew(SearchBoxWidget, SSearchBox)
			.InitialText(this, &FAnuDataTableEditor::GetFilterText)
		.OnTextChanged(this, &FAnuDataTableEditor::OnFilterTextChanged)
		.OnTextCommitted(this, &FAnuDataTableEditor::OnFilterTextCommitted)
		]
		]
	+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		[
			SNew(SScrollBox)
			.Orientation(Orient_Horizontal)
		.ExternalScrollbar(HorizontalScrollBar)
		+ SScrollBox::Slot()
		[
			CellsListView.ToSharedRef()
		]
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			VerticalScrollBar
		]
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		[
			HorizontalScrollBar
		]
		];
}

TSharedRef<SWidget> FAnuDataTableEditor::CreateRowEditorBox()
{
	UDataTable* Table = Cast<UDataTable>(GetEditingObject());

	// Support undo/redo
	if (Table)
	{
		Table->SetFlags(RF_Transactional);
	}

	auto RowEditor = SNew(SAnuRowEditor, Table);
	RowEditor->RowSelectedCallback.BindSP(this, &FAnuDataTableEditor::SetHighlightedRow);
	CallbackOnRowHighlighted.BindSP(RowEditor, &SAnuRowEditor::SelectRow);
	CallbackOnDataTableUndoRedo.BindSP(RowEditor, &SAnuRowEditor::HandleUndoRedo);
	return RowEditor;
}

TSharedRef<SAnuRowEditor> FAnuDataTableEditor::CreateRowEditor(UDataTable* Table)
{
	return SNew(SAnuRowEditor, Table);
}

TSharedRef<SDockTab> FAnuDataTableEditor::SpawnTab_RowEditor(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == RowEditorTabId);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("DataTableEditor.Tabs.Properties"))
		.Label(LOCTEXT("RowEditorTitle", "Row Editor"))
		.TabColorScale(GetTabColorScale())
		[
			SNew(SBorder)
			.Padding(2)
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Fill)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			RowEditorTabWidget.ToSharedRef()
		]
		];
}


TSharedRef<SDockTab> FAnuDataTableEditor::SpawnTab_DataTable(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == DataTableTabId);

	UDataTable* Table = Cast<UDataTable>(GetEditingObject());

	// Support undo/redo
	if (Table)
	{
		Table->SetFlags(RF_Transactional);
	}

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("DataTableEditor.Tabs.Properties"))
		.Label(LOCTEXT("DataTableTitle", "Data Table"))
		.TabColorScale(GetTabColorScale())
		[
			SNew(SBorder)
			.Padding(2)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			DataTableTabWidget.ToSharedRef()
		]
		];
}

TSharedRef<SDockTab> FAnuDataTableEditor::SpawnTab_DataTableDetails(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == DataTableDetailsTabId);

	PropertyView->SetObject(GetEditableDataTable());

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("DataTableEditor.Tabs.Properties"))
		.Label(LOCTEXT("DataTableDetails", "Data Table Details"))
		.TabColorScale(GetTabColorScale())
		[
			SNew(SBorder)
			.Padding(2)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			PropertyView.ToSharedRef()
		]
		];
}

void FAnuDataTableEditor::SetHighlightedRow(FName Name)
{
	if (Name == HighlightedRowName)
	{
		return;
	}

	if (Name.IsNone())
	{
		HighlightedRowName = NAME_None;
		CellsListView->ClearSelection();
		HighlightedVisibleRowIndex = INDEX_NONE;
	}
	else
	{
		HighlightedRowName = Name;

		FDataTableEditorRowListViewDataPtr* NewSelectionPtr = NULL;
		for (HighlightedVisibleRowIndex = 0; HighlightedVisibleRowIndex < VisibleRows.Num(); ++HighlightedVisibleRowIndex)
		{
			if (VisibleRows[HighlightedVisibleRowIndex]->RowId == Name)
			{
				NewSelectionPtr = &(VisibleRows[HighlightedVisibleRowIndex]);

				break;
			}
		}


		// Synchronize the list views
		if (NewSelectionPtr)
		{
			CellsListView->SetSelection(*NewSelectionPtr);

			CellsListView->RequestScrollIntoView(*NewSelectionPtr);
		}
		else
		{
			CellsListView->ClearSelection();
		}
	}
}

#undef LOCTEXT_NAMESPACE
