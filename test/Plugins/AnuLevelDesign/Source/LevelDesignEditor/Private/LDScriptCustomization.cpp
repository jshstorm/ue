// Fill out your copyright notice in the Description page of Project Settings.

#include "LDScriptCustomization.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "SEnumCombobox.h"

TSharedRef<IPropertyTypeCustomization> FLDScriptCustomization::MakeInstance()
{
	return MakeShared<FLDScriptCustomization>();
}

void FLDScriptCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//const bool bDisplayResetToDefault = false;
	//const FText DisplayNameOverride = FText::GetEmpty();
	//const FText DisplayToolTipOverride = FText::GetEmpty();

	//HeaderRow
	//.NameContent()
	//[
	//	StructPropertyHandle->CreatePropertyNameWidget(DisplayNameOverride, DisplayToolTipOverride, bDisplayResetToDefault)
	//];
}

void FLDScriptCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//
	// ULevelCondition::ConditionType 값에 따른 가변 파라미터를 디테일 패널에 출력
	//

	StructBuilder.AddProperty(StructPropertyHandle);

	//UObject* ValueAddress = nullptr;
	//StructPropertyHandle->GetValue(ValueAddress);
	//ULDScript* Value = Cast<ULDScript>(ValueAddress);
	//int32 Index = StructPropertyHandle->GetIndexInArray();

	//StructBuilder.AddCustomRow(FText::FromString(""))
	//.NameContent()
	//[
	//	SNew(STextBlock)
	//	.Text(FText::FromString(TEXT("ConditionType")))
	//]
	//.ValueContent()
	//[
	//	SNew(SEnumComboBox, GET_ENUM_PTR(EConditionType))
	//	.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
	//	.CurrentValue_Lambda([Value]() { return (int32)Value->ConditionType; })
	//	.OnEnumSelectionChanged_Lambda([Value, Index, StructPropertyHandle](int32 selectionValue, ESelectInfo::Type selectionType) {
	//		Value->ConditionType = (EConditionType)selectionValue;
	//		StructPropertyHandle->NotifyPostChange();
	//	})
	//];

	//if (Value->GetStructPtr() == nullptr) {
	//	return;
	//}

	//TSharedPtr<FStructOnScope> StructOnScope = MakeShareable(new FStructOnScope(Value->GetStructType(), Value->GetStructPtr()));
	//IDetailPropertyRow* StructRawPtr = StructBuilder.AddExternalStructure(StructOnScope.ToSharedRef());
	//StructRawPtr->Visibility(EVisibility::Hidden);

	//TSharedPtr<IPropertyHandle> StructRowHandle = StructRawPtr->GetPropertyHandle();
	//uint32 StructRowNum = 0;
	//StructRowHandle->GetNumChildren(StructRowNum);
	//for (uint32 i = 0; i < StructRowNum; ++i) {
	//	TSharedPtr<IPropertyHandle> StructRowPropertyHandle = StructRowHandle->GetChildHandle(i);
	//	StructBuilder.AddProperty(StructRowPropertyHandle.ToSharedRef());
	//}
}