// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnuDataTableEditor.h"

class FResourceModelDataTableEditor : public FAnuDataTableEditor
{
public:
	static const FString SupportedDataTableName;
	static FResourceModelDataTableEditor* Instance;

	struct PartialTable
	{
		FName RowStructName;
		FString RowNamePrefix;
		UDataTable* DataTable;

		PartialTable(FName inRowStructName, FString inRowNamePrefix, UDataTable* inDataTable)
		 : RowStructName(inRowStructName), RowNamePrefix(inRowNamePrefix), DataTable(inDataTable){ }
	};

private:
	TArray<PartialTable> _partialTables;

public:
	/** Constructor */
	FResourceModelDataTableEditor();

	/** Destructor */
	virtual ~FResourceModelDataTableEditor();

	virtual void InitDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table) override;
	virtual void PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info) override;
	virtual FSlateColor GetRowTextColor(FName RowName) const override;

	virtual void OnAddClicked() override;
	virtual void SelectionChange(const UDataTable* Changed, FName RowName) override;

protected:
	virtual void FillToolbar(FToolBarBuilder& ToolbarBuilder) override;

private:
	void OnSyncClicked();
	void OnQueryClicked();
	void BuildMainMesh(const UDataTable* Changed);
	void BuildSubMeshes(const UDataTable* Changed);

	UDataTable* LoadPartialTable(FString tableName);
};