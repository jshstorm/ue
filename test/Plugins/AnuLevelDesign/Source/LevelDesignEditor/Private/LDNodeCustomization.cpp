// Fill out your copyright notice in the Description page of Project Settings.

#include "LDNodeCustomization.h"
//#include "Common/AnuCommonDefine.h"
//#include "LevelDesign/LevelDesignTask.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "Toolkits/AssetEditorManager.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Dom/JsonObject.h"

#include "Modules/ModuleManager.h"
#include "IStructureDetailsView.h"
#include "PropertyCustomizationHelpers.h"
#include "SEnumCombobox.h"

#include "LDNode.h"

TSharedRef<IDetailCustomization> FLDNodeCustomization::MakeInstance()
{
	return MakeShared<FLDNodeCustomization>();
}

void FLDNodeCustomization::CustomizeDetails(IDetailLayoutBuilder& LayoutBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	LayoutBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	AddConditionCategory(LayoutBuilder, Cast<ULDNode>(ObjectsBeingCustomized[0]));
}

void FLDNodeCustomization::AddConditionCategory(IDetailLayoutBuilder& LayoutBuilder, ULDNode* Target)
{
	auto& CategoryBuilder = LayoutBuilder.EditCategory(TEXT("# Conditions"), FText::GetEmpty(), ECategoryPriority::TypeSpecific);

	TSharedRef<IPropertyHandle> ConditionScriptsHandle = LayoutBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULDNode, ConditionScripts));
	TSharedRef<FDetailArrayBuilder> ConditionScriptsBuilder = MakeShared<FDetailArrayBuilder>(ConditionScriptsHandle);
	ConditionScriptsBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FLDNodeCustomization::CustomizeConditionChildren, Target));
	CategoryBuilder.AddCustomBuilder(ConditionScriptsBuilder);
}

void FLDNodeCustomization::CustomizeConditionChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, int32 Index, IDetailChildrenBuilder& StructBuilder, ULDNode* Target)
{	
	StructBuilder.AddProperty(StructPropertyHandle);
}