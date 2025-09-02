// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnuDataTableEditor.h"

class FSkillTimelineDataTableEditor : public FAnuDataTableEditor
{
public:
	static const FString SupportedDataTableName;

public:
	virtual void InitDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table) override;

protected:
	virtual void FillToolbar(FToolBarBuilder& ToolbarBuilder) override;

private:
	void OnRenameClicked();
};