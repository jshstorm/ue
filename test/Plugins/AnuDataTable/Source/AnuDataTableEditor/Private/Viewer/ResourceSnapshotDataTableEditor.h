// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnuDataTableEditor.h"

struct ANUREFERENCE_API FAnuResourceSnapshot;

struct FSnapshotDataTableParam {
	FSnapshotDataTableParam(const FName& rowName, FAnuResourceSnapshot* tableData) 
		: key(rowName), resData(tableData) { }
	FName key;
	FAnuResourceSnapshot* resData = nullptr;
};

class FResourceSnapshotDataTableEditor : public FAnuDataTableEditor
{
public:
	static const FString SupportedDataTableName;

public:
	FResourceSnapshotDataTableEditor();

	DECLARE_DELEGATE_OneParam(FExportDelegate, const TArray<FSnapshotDataTableParam>&);
	void BindExport(FExportDelegate&& func);
public:

	virtual void InitDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table) override;

protected:
	virtual void FillToolbar(FToolBarBuilder& ToolbarBuilder) override;
	virtual void OnDuplicateRow(const FName NewName) override;
	virtual void OnRenameOrDeleteRow(const FName NewName) override;

private:
	void OnExportCheckOnly();
	void OnExportNew();
	void OnExportAll();
	void OnResetCheckBox();

private:
	FAnuResourceSnapshot* GetRowStructData(const FName& rowName);
	void VisitTableRow(TFunction<void(const FName& key, FAnuResourceSnapshot*)>&& visitor);

private:
	FExportDelegate _exportEvent;
	
};