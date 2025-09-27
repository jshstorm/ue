#include "Reference.h"

#include "UObject/TextProperty.h"
#include "Engine/Texture2D.h"
#include "Engine.h"
#include "Kismet/KismetStringLibrary.h"
#include "LogAnuReference.h"
#include "Internationalization/Text.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Sound/SoundWave.h"
#include "ReferenceBuilder.h"

#include "Internationalization/StringTableCore.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

const static FString Delimiter{ "|" };
const static int32 totalTagCount = 8;

////////////////////////////////////////////////////////////////////////////////////
void URefBase::Parse(const FXmlNode* node)
{
	for (const FXmlAttribute& attr : node->GetAttributes())
	{
		FProperty* prob = GetClass()->FindPropertyByName(*attr.GetTag());
		if (prob == nullptr) {
			continue;
		}

		void* memberProp = prob->ContainerPtrToValuePtr<void>(this);
		check(memberProp != nullptr);

		if (auto numProb = CastField<FNumericProperty>(prob)) {
			if (numProb->IsFloatingPoint()) {
				float numValue = FCString::Atof(*attr.GetValue());
				prob->CopySingleValue(memberProp, &numValue);
			}
			else if (numProb->IsInteger()) {
				/*
				if (numProb->ElementSize == sizeof(int64)) {
					int64 numValue = FCString::Atoi64(*attr.GetValue());
					prob->CopySingleValue(memberProp, &numValue);
				}
				else {
					int32 numValue = FCString::Atoi(*attr.GetValue());
					prob->CopySingleValue(memberProp, &numValue);
				}
				*/
			}
			continue;
		}

		if (auto boolProp = CastField<FBoolProperty>(prob))	{
			const FString& sValue{ attr.GetValue().ToLower() };
			bool propValue = sValue.ToBool();
			prob->CopySingleValue(memberProp, &propValue);
			continue;
		}

		const static FString none = TEXT("None");

		if (auto assetProb = CastField<FObjectProperty>(prob))	{
			const FString* assetName = (const FString*)&attr.GetValue();
			if (assetName->Compare(TEXT("None")) != 0)
			{
				if (UObject* assetObj = LoadObject<UObject>(this, **assetName))
				{
					assetProb->SetObjectPropertyValue(memberProp, assetObj);
				}
			}
			continue;
		}

		if (auto strProp = CastField<FStrProperty>(prob)) {
			prob->CopySingleValue(memberProp, &attr.GetValue());
			continue;
		}

		if (auto nameProp = CastField<FNameProperty>(prob))	{
			FName value = *attr.GetValue();
			prob->CopySingleValue(memberProp, &value);
			continue;
		}

		if (auto enumProp = CastField<FEnumProperty>(prob)) {
			FName value = *attr.GetValue();
			int32 index = enumProp->GetEnum()->GetIndexByName(value);
			prob->CopySingleValue(memberProp, &index);
			continue;
		}

		check(false);
	}
}

void URefBase::Parse(UScriptStruct* type, FTableRowBase* row)
{
	for (TFieldIterator<FProperty> sourceProp(type); sourceProp; ++sourceProp) {
		FString name = sourceProp->GetName();
		FProperty* targetProb = GetClass()->FindPropertyByName(*name);
		if (targetProb == nullptr) {
			continue;
		}

		void* propValue = targetProb->ContainerPtrToValuePtr<void>(this);
		targetProb->CopySingleValue(propValue, sourceProp->ContainerPtrToValuePtr<void>(row)); 
	}
}

void URefBase::ParseArrayProperty(const FArrayProperty* arrProp, const TArray<FString>& strValues, void* memberProp)
{
	if (auto arrNumProb = CastField<FNumericProperty>(arrProp->Inner)) {
		for (auto& arrValue : strValues) {
			if (arrNumProb->IsFloatingPoint()) {
				float numValue = FCString::Atof(*arrValue);
				((TArray<float>*)memberProp)->Add(numValue);
			}
			else if (arrNumProb->IsInteger()) {
				if (arrNumProb->GetElementSize() == sizeof(int64)) {
					int64 numValue = FCString::Atoi64(*arrValue);
					((TArray<int64>*)memberProp)->Add(numValue);
				}
				else { // default integer -> int32
					int32 numValue = FCString::Atoi(*arrValue);
					((TArray<int32>*)memberProp)->Add(numValue);
				}
			}
		}
	}
	else if (CastField<FStrProperty>(arrProp->Inner)) {
		for (auto& arrValue : strValues) {
			((TArray<FString>*)memberProp)->Add(arrValue);
		}
	}
	else if (CastField<FNameProperty>(arrProp->Inner)) {
		for (auto& arrValue : strValues) {
			((TArray<FName>*)memberProp)->Add(*arrValue);
		}
	}
	else if (auto enumProp = CastField<FEnumProperty>(arrProp->Inner)) {
		for (auto& arrValue : strValues) {
			int32 index = enumProp->GetEnum()->GetIndexByName(*arrValue);
			((TArray<uint8>*)memberProp)->Add(index);
		}
	}
}

FName URefBase::GetEnumValueName(const FName& enumMemberName, int64 value)
{
	FProperty* property = GetClass()->FindPropertyByName(enumMemberName);
	if (property == nullptr) {
		check(false);
		return NAME_None;
	}

	auto enumProperty = CastField<FEnumProperty>(property);
	checkf(enumProperty, TEXT("reference[%s] has no enum member[%s]"), *GetName(), *enumMemberName.ToString());
	return enumProperty->GetEnum()->GetNameByValue(value);
}

FString URefBase::GetEnumValueString(const FName& enumMemberName, int64 value, bool trimNamespace)
{
	FString valueStr{ GetEnumValueName(enumMemberName, value).ToString() };
	if (trimNamespace == false) {
		return valueStr;
	}
	int32 lastNamespaceDelimeter = INDEX_NONE;
	valueStr.FindLastChar(':', lastNamespaceDelimeter);
	if (lastNamespaceDelimeter == INDEX_NONE) {
		return valueStr;
	}
	return valueStr.RightChop(lastNamespaceDelimeter + 1); // + 1 -> ":"
}
