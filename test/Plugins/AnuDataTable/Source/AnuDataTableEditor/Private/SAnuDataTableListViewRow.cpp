// Copyright Epic Games, Inc. All Rights Reserved.

#include "SAnuDataTableListViewRow.h"

#include "AssetData.h"
#include "AnuDataTableEditor.h"
#include "EditorStyleSet.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Misc/MessageDialog.h"
#include "Framework/Commands/GenericCommands.h"
#include "DetailWidgetRow.h"

#define LOCTEXT_NAMESPACE "SDataTableListViewRowName"

void SAnuDataTableListViewRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	RowDataPtr = InArgs._RowDataPtr;
	CurrentName = MakeShared<FName>(RowDataPtr->RowId);
	AnuDataTableEditor = InArgs._AnuDataTableEditor;
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25)
	IsEditable = InArgs._IsEditable;
#endif
	SMultiColumnTableRow<FDataTableEditorRowListViewDataPtr>::Construct(
		FSuperRowType::FArguments()
		.Style(FEditorStyle::Get(), "DataTableEditor.CellListViewRow")
		.OnDrop(this, &SAnuDataTableListViewRow::OnRowDrop)
		.OnDragEnter(this, &SAnuDataTableListViewRow::OnRowDragEnter)
		.OnDragLeave(this, &SAnuDataTableListViewRow::OnRowDragLeave),
		InOwnerTableView
	);

	BorderImage = TAttribute<const FSlateBrush*>(this, &SAnuDataTableListViewRow::GetBorder);

}

FReply SAnuDataTableListViewRow::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (IsEditable && MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && RowDataPtr.IsValid() && FEditorDelegates::OnOpenReferenceViewer.IsBound() && AnuDataTableEditor.IsValid())
	{
		FDataTableEditorUtils::SelectRow(AnuDataTableEditor.Pin()->GetDataTable(), RowDataPtr->RowId);

		TSharedRef<SWidget> MenuWidget = MakeRowActionsMenu();

		FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
		FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuWidget, MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect::ContextMenu);
		return FReply::Handled();
	}

	return STableRow::OnMouseButtonUp(MyGeometry, MouseEvent);
}

void SAnuDataTableListViewRow::OnSearchForReferences()
{
	if (AnuDataTableEditor.IsValid() && RowDataPtr.IsValid())
	{
		if (FAnuDataTableEditor* DataTableEditorPtr = AnuDataTableEditor.Pin().Get())
		{
			UDataTable* SourceDataTable = const_cast<UDataTable*>(DataTableEditorPtr->GetDataTable());

			TArray<FAssetIdentifier> AssetIdentifiers;
			AssetIdentifiers.Add(FAssetIdentifier(SourceDataTable, RowDataPtr->RowId));

			FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
		}
	}
}

void SAnuDataTableListViewRow::OnInsertNewRow(ERowInsertionPosition InsertPosition)
{
	if (AnuDataTableEditor.IsValid() && RowDataPtr.IsValid())
	{
		if (FAnuDataTableEditor* DataTableEditorPtr = AnuDataTableEditor.Pin().Get())
		{
			UDataTable* SourceDataTable = const_cast<UDataTable*>(DataTableEditorPtr->GetDataTable());

			if (SourceDataTable)
			{
				FName NewName = DataTableUtils::MakeValidName(TEXT("NewRow"));
				while (SourceDataTable->GetRowMap().Contains(NewName))
				{
					NewName.SetNumber(NewName.GetNumber() + 1);
				}

				if (InsertPosition == ERowInsertionPosition::Bottom)
				{
					FDataTableEditorUtils::AddRow(SourceDataTable, NewName);
				}
				else
				{
					FDataTableEditorUtils::AddRowAboveOrBelowSelection(SourceDataTable, *CurrentName, NewName, InsertPosition);
				}
				FDataTableEditorUtils::SelectRow(SourceDataTable, NewName);

				DataTableEditorPtr->SetDefaultSort();
			}
		}
	}
}

FReply SAnuDataTableListViewRow::OnRowDrop(const FDragDropEvent& DragDropEvent)
{
	bIsHoveredDragTarget = false;

	TSharedPtr<FAnuDataTableRowDragDropOp> DataTableDropOp = DragDropEvent.GetOperationAs< FAnuDataTableRowDragDropOp >();
	TSharedPtr<SAnuDataTableListViewRow> RowPtr = nullptr;
	if (DataTableDropOp.IsValid() && DataTableDropOp->Row.IsValid())
	{
		RowPtr = DataTableDropOp->Row.Pin();
	}
	if (!RowPtr.IsValid())
	{
		return FReply::Unhandled();
	}

	int32 JumpCount = (RowPtr->RowDataPtr)->RowNum - RowDataPtr->RowNum;

	if (!JumpCount)
	{
		return FReply::Handled();
	}

	FDataTableEditorUtils::ERowMoveDirection Direction = JumpCount > 0 ? FDataTableEditorUtils::ERowMoveDirection::Up : FDataTableEditorUtils::ERowMoveDirection::Down;

	if (FAnuDataTableEditor* DataTableEditorPtr = AnuDataTableEditor.Pin().Get())
	{
		UDataTable* SourceDataTable = const_cast<UDataTable*>(DataTableEditorPtr->GetDataTable());

		if (SourceDataTable)
		{
			FName& RowId = (RowPtr->RowDataPtr)->RowId;

			FDataTableEditorUtils::MoveRow(SourceDataTable, RowId, Direction, FMath::Abs<int32>(JumpCount));
			
			FDataTableEditorUtils::SelectRow(SourceDataTable, RowId);

			DataTableEditorPtr->SortMode = EColumnSortMode::Ascending;
			DataTableEditorPtr->SortByColumn = FAnuDataTableEditor::RowNumberColumnId;

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SAnuDataTableListViewRow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Escape && InlineEditableText->HasKeyboardFocus())
	{
		// Clear focus
		return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Cleared);
	}

	return FReply::Unhandled();
}

void SAnuDataTableListViewRow::OnRowRenamed(const FText& Text, ETextCommit::Type CommitType)
{
	UDataTable* DataTable = Cast<UDataTable>(AnuDataTableEditor.Pin()->GetEditingObject());

	if (!GetCurrentNameAsText().EqualTo(Text) && DataTable)
	{
		if (FAnuDataTableEditor* DataTableEditorPtr = AnuDataTableEditor.Pin().Get())
		{

			const TArray<FName> RowNames = DataTable->GetRowNames();

			if (Text.IsEmptyOrWhitespace() || !FName::IsValidXName(Text.ToString(), INVALID_NAME_CHARACTERS))
			{
				// Only pop up the error dialog if the rename was caused by the user's action
				if ((CommitType == ETextCommit::OnEnter) || (CommitType == ETextCommit::OnUserMovedFocus))
				{
					// popup an error dialog here
					const FText Message = FText::Format(LOCTEXT("InvalidRowName", "'{0}' is not a valid row name"), Text);
					FMessageDialog::Open(EAppMsgType::Ok, Message);
				}
				return;
			}
			const FName NewName = DataTableUtils::MakeValidName(Text.ToString());
			if (NewName == NAME_None)
			{
				// popup an error dialog here
				const FText Message = FText::Format(LOCTEXT("InvalidRowName", "'{0}' is not a valid row name"), Text);
				FMessageDialog::Open(EAppMsgType::Ok, Message);

				return;
			}
			for (const FName& Name : RowNames)
			{
				if (Name.IsValid() && (Name == NewName))
				{
					//the name already exists
					// popup an error dialog here
					const FText Message = FText::Format(LOCTEXT("DuplicateRowName", "'{0}' is already used as a row name in this table"), Text);
					FMessageDialog::Open(EAppMsgType::Ok, Message);
					return;

				}
			}

			const FName OldName = GetCurrentName();
			DataTableEditorPtr->OnRenameOrDeleteRow(OldName);
			FDataTableEditorUtils::RenameRow(DataTable, OldName, NewName);
			FDataTableEditorUtils::SelectRow(DataTable, NewName);
			DataTableEditorPtr->SetDefaultSort();

			*CurrentName = NewName;
		}
	}
}

TSharedRef<SWidget> SAnuDataTableListViewRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	TSharedPtr<FAnuDataTableEditor> DataTableEditorPtr = AnuDataTableEditor.Pin();
	return (DataTableEditorPtr.IsValid())
		? MakeCellWidget(IndexInList, ColumnName)
		: SNullWidget::NullWidget;
}

TSharedRef<SWidget> SAnuDataTableListViewRow::MakeCellWidget(const int32 InRowIndex, const FName& InColumnId)
{
	const FName RowDragDropColumnId("RowDragDrop");

	int32 ColumnIndex = 0;

	FAnuDataTableEditor* DataTableEdit = AnuDataTableEditor.Pin().Get();
	TArray<FDataTableEditorColumnHeaderDataPtr>& AvailableColumns = DataTableEdit->AvailableColumns;

	if (InColumnId.IsEqual(RowDragDropColumnId))
	{
		return SNew(SAnuDataTableRowHandle)
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(5.0f, 1.0f)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("VerticalBoxDragIndicatorShort"))
				]
			]
			.ParentRow(SharedThis(this));
	}

	const FName RowNumberColumnId("RowNumber");

	if (InColumnId.IsEqual(RowNumberColumnId))
	{
		return SNew(SBox)
			.Padding(FMargin(4, 2, 4, 2))
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "DataTableEditor.CellText")
				.Text(FText::FromString(FString::FromInt(RowDataPtr->RowNum)))
				.ColorAndOpacity(DataTableEdit, &FAnuDataTableEditor::GetRowTextColor, RowDataPtr->RowId)
				.HighlightText(DataTableEdit, &FAnuDataTableEditor::GetFilterText)
			];
	}

	const FName RowNameColumnId("RowName");

	if (InColumnId.IsEqual(RowNameColumnId))
	{
		return SNew(SBox)
			.Padding(FMargin(4, 2, 4, 2))
			[
				SAssignNew(InlineEditableText, SInlineEditableTextBlock)
				.Text(RowDataPtr->DisplayName)
				.OnTextCommitted(this, &SAnuDataTableListViewRow::OnRowRenamed)
				.HighlightText(DataTableEdit, &FAnuDataTableEditor::GetFilterText)
				.ColorAndOpacity(DataTableEdit, &FAnuDataTableEditor::GetRowTextColor, RowDataPtr->RowId)
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25)
				.IsReadOnly(!IsEditable)
#endif
			];
	}

	for (; ColumnIndex < AvailableColumns.Num(); ++ColumnIndex)
	{
		const FDataTableEditorColumnHeaderDataPtr& ColumnData = AvailableColumns[ColumnIndex];
		if (ColumnData->ColumnId == InColumnId)
		{
			break;
		}
	}
	 
	// Valid column ID?
	if (AvailableColumns.IsValidIndex(ColumnIndex) && RowDataPtr->CellData.IsValidIndex(ColumnIndex))
	{
		return SNew(SBox)
			.Padding(FMargin(4, 2, 4, 2))
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "DataTableEditor.CellText")
				.ColorAndOpacity(DataTableEdit, &FAnuDataTableEditor::GetRowTextColor, RowDataPtr->RowId)
				.Text(DataTableEdit, &FAnuDataTableEditor::GetCellText, RowDataPtr, ColumnIndex)
				.HighlightText(DataTableEdit, &FAnuDataTableEditor::GetFilterText)
				.ToolTipText(DataTableEdit, &FAnuDataTableEditor::GetCellToolTipText, RowDataPtr, ColumnIndex)
			];
	}

	return SNullWidget::NullWidget;
}

FName SAnuDataTableListViewRow::GetCurrentName() const
{
	return CurrentName.IsValid() ? *CurrentName : NAME_None;

}

uint32 SAnuDataTableListViewRow::GetCurrentIndex() const
{
	return RowDataPtr.IsValid() ? RowDataPtr->RowNum : -1;
}

const TArray<FText>& SAnuDataTableListViewRow::GetCellValues() const
{
	check(RowDataPtr)
	return RowDataPtr->CellData;
}

FReply SAnuDataTableListViewRow::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InlineEditableText->IsHovered())
	{
		InlineEditableText->EnterEditingMode();
	}

	return FReply::Handled();
}

void SAnuDataTableListViewRow::SetRowForRename()
{
	InlineEditableText->EnterEditingMode();
}

const FDataTableEditorRowListViewDataPtr& SAnuDataTableListViewRow::GetRowDataPtr() const
{
	return RowDataPtr;
}

FText SAnuDataTableListViewRow::GetCurrentNameAsText() const
{
	return FText::FromName(GetCurrentName());
}

void SAnuDataTableRowHandle::Construct(const FArguments& InArgs)
{
	ParentRow = InArgs._ParentRow;

	ChildSlot
		[
			InArgs._Content.Widget
		];
}

FReply SAnuDataTableRowHandle::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		TSharedPtr<FDragDropOperation> DragDropOp = CreateDragDropOperation(ParentRow.Pin());
		if (DragDropOp.IsValid())
		{
			return FReply::Handled().BeginDragDrop(DragDropOp.ToSharedRef());
		}
	}

	return FReply::Unhandled();

}

TSharedPtr<FAnuDataTableRowDragDropOp> SAnuDataTableRowHandle::CreateDragDropOperation(TSharedPtr<SAnuDataTableListViewRow> InRow)
{
	TSharedPtr<FAnuDataTableRowDragDropOp> Operation = MakeShared<FAnuDataTableRowDragDropOp>(InRow);

	return Operation;
}

void SAnuDataTableListViewRow::SetIsDragDrop(bool bInIsDragDrop)
{
	bIsDragDropObject = bInIsDragDrop;
}


void SAnuDataTableListViewRow::OnRowDragEnter(const FDragDropEvent& DragDropEvent)
{
	bIsHoveredDragTarget = true;
}

void SAnuDataTableListViewRow::OnRowDragLeave(const FDragDropEvent& DragDropEvent)
{
	bIsHoveredDragTarget = false;
}


const FSlateBrush* SAnuDataTableListViewRow::GetBorder() const
{
	if (bIsDragDropObject)
	{
		return FEditorStyle::GetBrush("DataTableEditor.DragDropObject");
	}
	else if (bIsHoveredDragTarget)
	{
		return FEditorStyle::GetBrush("DataTableEditor.DragDropHoveredTarget");
	}
	else
	{
		return STableRow::GetBorder();
	}
}

void SAnuDataTableListViewRow::OnMoveToExtentClicked(FDataTableEditorUtils::ERowMoveDirection MoveDirection)
{
	if (AnuDataTableEditor.IsValid())
	{
		TSharedPtr<FAnuDataTableEditor> DataTableEditorPtr = AnuDataTableEditor.Pin();
		DataTableEditorPtr->OnMoveToExtentClicked(MoveDirection);
	}
}


TSharedRef<SWidget> SAnuDataTableListViewRow::MakeRowActionsMenu()
{
	FMenuBuilder MenuBuilder(true, AnuDataTableEditor.Pin()->GetToolkitCommands());

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTableRowMenuActions_InsertNewRow", "Insert New Row"),
		LOCTEXT("DataTableRowMenuActions_InsertNewRowTooltip", "Insert a new row"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Add"), 
		FUIAction(FExecuteAction::CreateSP(this, &SAnuDataTableListViewRow::OnInsertNewRow, ERowInsertionPosition::Bottom))
	);
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTableRowMenuActions_InsertNewRowAbove", "Insert New Row Above"),
		LOCTEXT("DataTableRowMenuActions_InsertNewRowAboveTooltip", "Insert a new Row above the current selection"),
		FSlateIcon(), 
		FUIAction(FExecuteAction::CreateSP(this, &SAnuDataTableListViewRow::OnInsertNewRow, ERowInsertionPosition::Above))
	);
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTableRowMenuActions_InsertNewRowBelow", "Insert New Row Below"),
		LOCTEXT("DataTableRowMenuActions_InsertNewRowBelowTooltip", "Insert a new Row below the current selection"),
		FSlateIcon(), 
		FUIAction(FExecuteAction::CreateSP(this, &SAnuDataTableListViewRow::OnInsertNewRow, ERowInsertionPosition::Below))
	);

	MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
	MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
	MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
	MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename);
	MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTableRowMenuActions_MoveToTopAction", "Move Row to Top"),
		LOCTEXT("DataTableRowMenuActions_MoveToTopActionTooltip", "Move selected Row to the top"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Symbols.DoubleUpArrow"), 
		FUIAction(FExecuteAction::CreateSP(this, &SAnuDataTableListViewRow::OnMoveToExtentClicked, FDataTableEditorUtils::ERowMoveDirection::Up))
	);
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTableRowMenuActions_MoveToBottom", "Move Row To Bottom"),
		LOCTEXT("DataTableRowMenuActions_MoveToBottomTooltip", "Move selected Row to the bottom"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Symbols.DoubleDownArrow"), 
		FUIAction(FExecuteAction::CreateSP(this, &SAnuDataTableListViewRow::OnMoveToExtentClicked, FDataTableEditorUtils::ERowMoveDirection::Down))
	);
	
	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("FDataTableRowUtils", "FDataTableRowUtils_SearchForReferences", "Find Row References"),
		NSLOCTEXT("FDataTableRowUtils", "FDataTableRowUtils_SearchForReferencesTooltip", "Find assets that reference this Row"),
		FSlateIcon(), 
		FUIAction(FExecuteAction::CreateSP(this, &SAnuDataTableListViewRow::OnSearchForReferences))
	);

	return MenuBuilder.MakeWidget();
}

FAnuDataTableRowDragDropOp::FAnuDataTableRowDragDropOp(TSharedPtr<SAnuDataTableListViewRow> InRow)
{
	Row = InRow;

	TSharedPtr<SAnuDataTableListViewRow> RowPtr = nullptr;
	if (Row.IsValid())
	{
		RowPtr = Row.Pin();
		RowPtr->SetIsDragDrop(true);

		DecoratorWidget = SNew(SBorder)
			.Padding(8.f)
			.BorderImage(FEditorStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::Format(NSLOCTEXT("DataTableDragDrop", "PlaceRowHere", "Place Row {0} Here"), FText::AsNumber(InRow->GetCurrentIndex())))
				]
			];

		Construct();
	}
}

void FAnuDataTableRowDragDropOp::OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent)
{
	FDecoratedDragDropOp::OnDrop(bDropWasHandled, MouseEvent);

	TSharedPtr<SAnuDataTableListViewRow> RowPtr = nullptr;
	if (Row.IsValid())
	{
		RowPtr = Row.Pin();
		RowPtr->SetIsDragDrop(false);
	}
}

#undef LOCTEXT_NAMESPACE
