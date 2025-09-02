// Copyright 2019 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PropertyHandle.h"
#include "Widgets/Input/STextComboBox.h"
#include "IDetailCustomization.h"

class IDetailCategoryBuilder;
class IDetailLayoutBuilder;

class ULDNode;

class ANULEVELDESIGNEDITOR_API FLDNodeCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& LayoutBuilder) override;

private:	
	void AddConditionCategory(IDetailLayoutBuilder& LayoutBuilder, ULDNode* Target);
	void CustomizeConditionChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, int32 Index, IDetailChildrenBuilder& StructBuilder, ULDNode* Target);
};