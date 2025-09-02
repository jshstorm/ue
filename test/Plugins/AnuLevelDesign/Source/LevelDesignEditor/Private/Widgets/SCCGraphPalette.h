// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved. 

#pragma once
#include "SGraphPalette.h"

class ANULEVELDESIGNEDITOR_API SCCGraphPalette : public SGraphPalette
{
public:
	SLATE_BEGIN_ARGS(SCCGraphPalette){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<class FLevelDesignEditor> InLevelDesignEditor);
	virtual void CollectAllActions(FGraphActionListBuilderBase& OutAllActions) override;
	virtual TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData);
	virtual FReply OnActionDragged(const TArray< TSharedPtr<FEdGraphSchemaAction> >& InActions, const FPointerEvent& MouseEvent);

	void Refresh();

protected:
	TWeakPtr<FLevelDesignEditor> LevelDesignEditor;
};
