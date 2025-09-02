#include "Reference.h"

#include "UObject/TextProperty.h"
#include "Engine/Texture2D.h"
#include "Engine.h"
#include "Kismet/KismetStringLibrary.h"
#include "LogAnuReference.h"
#include "AnuDyeingIconReferenceStatics.h"
#include "Internationalization/Text.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Reference_Resource.h"
#include "Sound/SoundWave.h"
#include "ReferenceBuilder.h"

#include "Internationalization/StringTableCore.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

const static FString Delimiter{ "|" };
const static int32 totalTagCount = 8;

void URefBase::GetTrimedStringArray(const FString& originText, TArray<FString>& outArray, const FString& delimeter)
{
	originText.ParseIntoArray(outArray, *delimeter);
	for (auto& it : outArray) {
		it.TrimStartAndEndInline();
	}
}

void URefBase::GetIntegerArray(const FString& originText, TArray<int32>& outArray)
{
	TArray<FString> strValues;
	GetTrimedStringArray(originText, strValues);
	GetIntegerArray(strValues, outArray);
}

void URefBase::GetIntegerArray(const TArray<FString>& inArray, TArray<int32>& outArray)
{
	for (auto& it : inArray) {
		outArray.Emplace(FCString::Atoi(*it));
	}
}

void URefBase::VisitAttributes(const FXmlNode* node, const FString& columName, TFunction<void(const FXmlAttribute*)>&& visitor)
{
	for (const FXmlAttribute& attr : node->GetAttributes()) {
		const FString& value = attr.GetTag();
		if (value.Find(columName) != INDEX_NONE) {
			visitor(&attr);
		}
	}
}

const FString& URefBase::GetShortenGenderString(EGender gender)
{
	static FString Female = TEXT("F");
	static FString Male = TEXT("M");
	static FString None = TEXT("None");

	switch (gender) {
	case EGender::Female:
		return Female;
	case EGender::Male:
		return Male;
	}
	return None;
}

TMap<FName, EPermissionType> URefBase::PermissionNameOverrides{
	{ "Stage", EPermissionType::BattleStage },
	{ "Challenge", EPermissionType::QuestChallenge },
};
EPermissionType URefBase::GetPermissionType(const FName& permissionStr)
{
	static const UEnum* EnumClass = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPermissionType"), true);
	int32 index = EnumClass->GetIndexByName(permissionStr);
	if (index != INDEX_NONE) {
		return static_cast<EPermissionType>(index);
	}
	
	auto it = PermissionNameOverrides.Find(permissionStr);
	return it ? *it : EPermissionType::None;
}

FString URefBase::GetPermissionStr(EPermissionType permission)
{
	for (auto& it : PermissionNameOverrides) {
		if (it.Value == permission) {
			return it.Key.ToString();
		}
	}

	static const UEnum* EnumClass = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPermissionType"), true);
	auto valueStr{ EnumClass->GetNameByValue(static_cast<int64>(permission)).ToString() };

	int32 lastNamespaceDelimeter = INDEX_NONE;
	valueStr.FindLastChar(':', lastNamespaceDelimeter);
	if (lastNamespaceDelimeter == INDEX_NONE) {
		return valueStr;
	}
	return valueStr.RightChop(lastNamespaceDelimeter + 1); // + 1 -> ":"
}

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
				if (numProb->ElementSize == sizeof(int64)) {
					int64 numValue = FCString::Atoi64(*attr.GetValue());
					prob->CopySingleValue(memberProp, &numValue);
				}
				else {
					int32 numValue = FCString::Atoi(*attr.GetValue());
					prob->CopySingleValue(memberProp, &numValue);
				}
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

		if (auto nameProb = CastField<FTextProperty>(prob))	{
			if (attr.GetValue().Compare(none, ESearchCase::IgnoreCase) == 0) {
				continue;
			}

			FText value{ AnuText::Get_CommonTable(attr.GetValue()) };
			if (value.ToString() == FStringTableEntry::GetPlaceholderSourceString()) {
				value = AnuText::Get_UITable(attr.GetValue());
			}
			prob->CopySingleValue(memberProp, &value);
			continue;
		}

		if (auto arrProb = CastField<FArrayProperty>(prob))	{
			TArray<FString> strValues;
			URefBase::GetTrimedStringArray(attr.GetValue(), strValues, Delimiter);
			ParseArrayProperty(arrProb, strValues, memberProp);
			continue;
		}

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
		//prob->CopySingleValue(memberProp, &attr.GetValue());
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
				if (arrNumProb->ElementSize == sizeof(int64)) {
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
	else if (CastField<FTextProperty>(arrProp->Inner)) {
		for (auto& arrValue : strValues) {
			if (FName(arrValue).IsNone()) {
				continue;
			}
			FText stringTableValue{ AnuText::Get_CommonTable(arrValue) };
			((TArray<FText>*)memberProp)->Add(stringTableValue);
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

void URefBase::ParseJson(const FJsonObject* jsonObj, URefBase* target)
{
	// auto-parse 1-dimension fields only
	for (auto& it : jsonObj->Values) {
		const FString& fieldName{ it.Key };
		const auto& jsonVal{ it.Value };

		FProperty* prop = target->GetClass()->FindPropertyByName(*fieldName);
		if (prop == nullptr) {
			continue;
		}

		void* memberProp = prop->ContainerPtrToValuePtr<void>(target);
		check(memberProp != nullptr);
		if (auto numProb = CastField<FNumericProperty>(prop)) {
			if (numProb->IsFloatingPoint()) {
				float numValue = 0.f;
				if (jsonVal->TryGetNumber(numValue) == false) {
					checkf(false, TEXT("[%s] json value type is invalid; must be floating number; check uid[%s], field[%s]"), *target->GetName(), *target->UID.ToString(), *fieldName);
					continue;
				}
				prop->CopySingleValue(memberProp, &numValue);
			}
			else if (numProb->IsInteger()) {
				if (numProb->ElementSize == sizeof(int64)) {
					int64 numValue = 0;
					if (jsonVal->TryGetNumber(numValue) == false) {
						checkf(false, TEXT("[%s] json value type is invalid; must be int64 number; check uid[%s], field[%s]"), *target->GetName(), *target->UID.ToString(), *fieldName);
						continue;
					}
					prop->CopySingleValue(memberProp, &numValue);
				}
				else {
					int32 numValue = 0;
					if (jsonVal->TryGetNumber(numValue) == false) {
						checkf(false, TEXT("[%s] json value type is invalid; must be int32 number; check uid[%s], field[%s]"), *target->GetName(), *target->UID.ToString(), *fieldName);
						continue;
					}
					prop->CopySingleValue(memberProp, &numValue);
				}
			}
			continue;
		}

		if (auto boolProp = CastField<FBoolProperty>(prop)) {
			bool boolValue = false;
			if (jsonVal->TryGetBool(boolValue) == false) {
				checkf(false, TEXT("[%s] json value type is invalid; must be boolean; check uid[%s], field[%s]"), *target->GetName(), *target->UID.ToString(), *fieldName);
				continue;
			}
			prop->CopySingleValue(memberProp, &boolValue);
			continue;
		}

		const static FString none = TEXT("None");
		if (auto textProp = CastField<FTextProperty>(prop)) {
			FString strValue;
			if (jsonVal->TryGetString(strValue) == false) {
				checkf(false, TEXT("[%s] json value type is invalid; must be string uid; check uid[%s], field[%s]"), *target->GetName(), *target->UID.ToString(), *fieldName);
				continue;
			}

			if (strValue.Compare(none, ESearchCase::IgnoreCase) == 0) {
				continue;
			}

			FText value = AnuText::Get_CommonTable(strValue);
			prop->CopySingleValue(memberProp, &value);
			continue;
		}

		if (auto strProp = CastField<FStrProperty>(prop)) {
			FString strValue;
			if (jsonVal->TryGetString(strValue) == false) {
				checkf(false, TEXT("[%s] json value type is invalid; must be string; check uid[%s], field[%s]"), *target->GetName(), *target->UID.ToString(), *fieldName);
				continue;
			}
			prop->CopySingleValue(memberProp, &strValue);
			continue;
		}

		if (auto nameProp = CastField<FNameProperty>(prop)) {
			FString strValue;
			if (jsonVal->TryGetString(strValue) == false) {
				checkf(false, TEXT("[%s] json value type is invalid; must be string; check uid[%s], field[%s]"), *target->GetName(), *target->UID.ToString(), *fieldName);
				continue;
			}
			FName value = *strValue;
			prop->CopySingleValue(memberProp, &value);
			continue;
		}

		if (auto arrProb = CastField<FArrayProperty>(prop)) {
			FString strValue;
			if (jsonVal->TryGetString(strValue) == false) {
				checkf(false, TEXT("[%s] json value type is invalid; must be string; check uid[%s], field[%s]"), *target->GetName(), *target->UID.ToString(), *fieldName);
				continue;
			}
			TArray<FString> strValues;
			URefBase::GetTrimedStringArray(strValue, strValues);
			target->ParseArrayProperty(arrProb, strValues, memberProp);
			continue;
		}

		if (auto enumProp = CastField<FEnumProperty>(prop)) {
			FString strValue;
			if (jsonVal->TryGetString(strValue) == false) {
				checkf(false, TEXT("[%s] json value type is invalid; must be string; check uid[%s], field[%s]"), *target->GetName(), *target->UID.ToString(), *fieldName);
				continue;
			}
			FName value = *strValue;
			int32 index = enumProp->GetEnum()->GetIndexByName(value);
			prop->CopySingleValue(memberProp, &index);
			continue;
		}
	}
}

void URefBase::Parse(const FJsonObject* root)
{
	URefBase::ParseJson(root, this);
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

////////////////////////////////////////////////////////////////////////////////////
void URefObject::Parse(const FXmlNode* node)
{
	Super::Parse(node);
}

void URefObject::Parse(UScriptStruct* type, FTableRowBase* row)
{
	Super::Parse(type, row);
}

void URefObject::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	ResolveTypeID(mapper);
}

void URefObject::ResolveTypeID(const TMap<FName, int32>& mapper)
{
	check(typeNames.Num() == 4);
	typeId.typeID1 = mapper.FindRef(typeNames[0]);
	typeId.typeID2 = mapper.FindRef(typeNames[1]);
	typeId.typeID3 = mapper.FindRef(typeNames[2]);
	typeId.typeID4 = mapper.FindRef(typeNames[3]);

	typeNames.RemoveAll([](const FName& element) {
		return element.IsNone();
	});
}

void URefObject::ParseStatData(const FXmlNode* node)
{
}

UObject* URefObject::GetModelObject(EGender gender/* = EGender::Female*/)
{
	if (auto dtTable = GetResourceModel(gender)) {
		return dtTable->LoadResource();
	}
	return nullptr;
}

UTexture2D* URefObject::GetIconDyeingTexture(EGender gender, bool bMasking) const
{
	constexpr TCHAR MaskFilenameAppend[] = TEXT("_masking");

	if (!IsDyeingIconTexture(gender)) {
		UE_LOG(LogAnuReference, Error, TEXT("The icon of this reference is not valid dyeing texture icon. See URefObject::GetIconTexture function to load general icon texture."));
		return GetIconTexture(gender);
	}

	if (_displayInfo._icon.Num() == 0) {
		return nullptr;
	}

	int32 iconIndex = FMath::Clamp(static_cast<int32>(gender), 0, _displayInfo._icon.Num() - 1);
	UObject* assetToLoad = nullptr;

	const FSoftObjectPath AssetPathToLoad = bMasking
		? UAnuDyeingIconReferenceStatics::GetDyeingMaskIconReference(_displayInfo._icon[iconIndex].ToSoftObjectPath())
		: _displayInfo._icon[iconIndex].ToSoftObjectPath();

	assetToLoad = AssetPathToLoad.TryLoad();
	if (assetToLoad == nullptr) {
		UE_LOG(LogAnuReference, Error, TEXT("The icon reference is contained in \"Diffuse\" directory, that means, it is dyeing icon. But could not be found any diffuse or mask files from relative path."));
		return nullptr;
	}

	return Cast<UTexture2D>(assetToLoad);
}

bool URefObject::IsDyeingIconTexture(EGender gender) const
{
	if (_displayInfo._icon.Num() == 0) {
		return false;
	}

	int32 iconIndex = FMath::Clamp(static_cast<int32>(gender), 0, _displayInfo._icon.Num() - 1);
	return UAnuDyeingIconReferenceStatics::IsDyeingIconReference(_displayInfo._icon[iconIndex].ToSoftObjectPath());
}

FAnuResourceModelMesh* URefObject::GetResourceModel(EGender gender/* = EGender::Female*/)
{
	int32 idx = static_cast<int32>(gender);
	int32 available = _models.Num() > 0 ?_models.Num() - 1 : 0;
	idx = std::min(idx, available);
	return idx < _models.Num() ? static_cast<FAnuResourceModelMesh*>(_models[idx]) : nullptr;
}

UTexture2D* FDisplayInfo::GetIcon(EGender gender) const
{
	int32 idx = FMath::Clamp(static_cast<int32>(gender), 0, _icon.Num() - 1);
	return _icon.IsValidIndex(idx) ? _icon[idx].LoadSynchronous() : nullptr;
}

const FText& FDisplayInfo::GetName(EGender gender) const
{
	int32 idx = FMath::Clamp(static_cast<int32>(gender), 0, _name.Num() - 1);
	return _name.IsValidIndex(idx) ? _name[idx] : FText::GetEmpty();
}

const FText& FDisplayInfo::GetDesc(EGender gender) const
{
	int32 idx = FMath::Clamp(static_cast<int32>(gender), 0, _desc.Num() - 1);
	return _desc.IsValidIndex(idx) ? _desc[idx] : FText::GetEmpty();
}

UTexture2D* URefObject::GetIconTexture(EGender gender) const
{
	return _displayInfo.GetIcon(gender);
}

UTexture2D* URefObject::GetIcon2Texture(EGender gender) const
{
	int32 idx = static_cast<int32>(gender);
	int32 available = _icon2ObjectPtrs.Num() > 0 ? _icon2ObjectPtrs.Num() - 1 : 0;
	idx = std::min(idx, available);
	return idx < _icon2ObjectPtrs.Num() ? _icon2ObjectPtrs[idx].LoadSynchronous() : nullptr;
}

const FName& URefObject::GetIcon(EGender gender)
{
	static FName InvalidIcon{NAME_None};
	int32 idx = static_cast<int32>(gender);
	int32 available = Icon.Num() > 0 ? Icon.Num() - 1 : 0;
	idx = std::min(idx, available);
	return idx < Icon.Num() ? Icon[idx] : InvalidIcon;
}

FName URefObject::GetIcon2(EGender gender)
{
	int32 idx = static_cast<int32>(gender);
	int32 available = Icon_2.Num() > 0 ? Icon_2.Num() - 1 : 0;
	idx = std::min(idx, available);
	return idx < Icon_2.Num() ? Icon_2[idx] : NAME_None;
}

const FText& URefObject::GetName(EGender gender) const
{
	return _displayInfo.GetName(gender);
}

const FText& URefObject::GetDescription(EGender gender) const
{
	return _displayInfo.GetDesc(gender);
}

const FText& URefObject::GetDesc2(EGender gender)
{
	checkf(Desc_2.Num() > 0, TEXT("Desc_2 column error: check uid[%s]"), *UID.ToString());
	int32 idx = static_cast<int32>(gender);
	int32 available = Desc_2.Num() > 0 ? Desc_2.Num() - 1 : 0;
	idx = std::min(idx, available);
	return idx < Desc_2.Num() ? Desc_2[idx] : FText::GetEmpty();
}

bool URefObject::EnableSpawnClientActor()
{
	return typeId.IsCharacter() && !typeId.IsPC();
}

EAttachmentParts URefObject::GetAttachPart() const
{
	return GetAttachPart(typeId);
}

const FName& URefObject::GetLeafTypeID() const
{
	return typeNames.Last();
}

////////////////////////////////////////////////////////////////////////////////////
void URefNPC::Parse(const FXmlNode* node)
{
	Super::Parse(node);


	FString attriValue{ node->GetAttribute("Recognition_Distance") };
	if (!attriValue.IsEmpty()) {
		Recognition_Dist_Squared = FCString::Atof(*node->GetAttribute("Recognition_Distance"));
		Recognition_Dist_Squared *= Recognition_Dist_Squared;
	}

	FConditionRule::Parse(_interactionConditions, node->GetAttribute("Interaction_PreCondition"));

	Menu.Remove(NAME_None);

	_moveEvtUID = *FString::Printf(TEXT("move.to.%s"), *UID.ToString());
	_menuEvtUID = *FString::Printf(TEXT("open.menu.%s"), *UID.ToString());
	_marketEvtUID = *FString::Printf(TEXT("open.market.%s"), *UID.ToString());
}

void URefNPC::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Character", "NPC",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

void URefCharacterStat::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	static TSet<EStatTypes> OFFENSE_STATS {
		EStatTypes::Melee_Attack,
		EStatTypes::Magic_Attack
	};

	static TSet<EStatTypes> DEFENSE_STATS {
		EStatTypes::HP
	};

	static TSet<EStatTypes> DEFAULT_STATS {
		EStatTypes::CriticalChance,
		EStatTypes::CriticalDamage,
		EStatTypes::Melee_Defense,
		EStatTypes::Magic_Defense
	};

	for (const FXmlAttribute& attr : node->GetAttributes())
	{
		const FString& value = attr.GetTag();
		int32 found = value.Find("Stat_");
		if (found == INDEX_NONE) {
			continue;
		}
		const FName statName = *value.RightChop(found + 5);
		EStatTypes statType = GetStatEnum(statName);
		if (statType == EStatTypes::None) {
			continue;
		}

		if (DEFAULT_STATS.Contains(statType)) {
			DefaultStats.Emplace(statType, FCString::Strtoi(*attr.GetValue(), NULL, 10));
			continue;
		}

		if (OFFENSE_STATS.Contains(statType)) {
			OffenseStats.Emplace(statType, FCString::Strtoi(*attr.GetValue(), NULL, 10));
			continue;
		}

		if (DEFENSE_STATS.Contains(statType)) {
			DefenseStats.Emplace(statType, FCString::Strtoi(*attr.GetValue(), NULL, 10));
			continue;
		}

		checkf(false, TEXT("[Reference] CharacterStat - parse not supported : [%s]"), *statName.ToString());
	}
}

////////////////////////////////////////////////////////////////////////////////////
URefGlobal* URefGlobal::StaticParse(const FXmlNode* node)
{
	URefGlobal* reference = NewObject<URefGlobal>();
	reference->Key = node->GetAttribute("Key");
	reference->Value = node->GetAttribute("Value");
	return reference;
}

////////////////////////////////////////////////////////////////////////////////////
void URefItem::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	{
		TArray<FString> outArray;
		URefBase::GetTrimedStringArray(node->GetAttribute("Effect_Value"), outArray);
		if (outArray.Num() >= 2) {
			Effect_Value = FCString::Atoi(*(outArray[1]));
		}
	}

	ParseTag(node, "Tag", _tagWithValues);

	FString attrValue = node->GetAttribute("Duplicate_Replacement");
	if (attrValue.IsEmpty() == false && FName(attrValue).IsNone() == false) {
		TArray<FString> strValues;
		URefBase::GetTrimedStringArray(attrValue, strValues);
		checkf(strValues.Num() == 2, TEXT("[item] Item[%s] has invalid Duplicate_Replacement[%s]; expected format> currency.uid | amount"), *UID.ToString(), *attrValue);
		_dupReplacementUID = *strValues[0];
		_dupReplacementAmount = FCString::Atoi(*strValues[1]);
	}

	if (_tagWithValues.Contains("tag.fashion.special.reward")) {
		_isPrizeItem = true;
	}
}

void URefBase::ParseTag(const FXmlNode* node, const FString& tagColumn, TMap<FName, int32>& output)
{
	VisitAttributes(node, tagColumn, [&output](const FXmlAttribute* attr) {
		TArray<FString> outArray;
		URefBase::GetTrimedStringArray(attr->GetValue(), outArray);

		int32 tagValue = outArray.Num() > 1 ? FCString::Atoi(*outArray[1]) : 0;
		output.Emplace(*outArray[0], tagValue);
	});
}

void URefItem::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Item",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_2"));
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

URefTag* URefItem::GetFirstTag() const
{
	if (_tags.Num() == 0) {
		return nullptr;
	}
	return _tags.CreateConstIterator()->Key;
}

int32 URefItem::GetTotalTagValue()
{
	int32 retValue = 0;
	for (auto& it : _tagWithValues) {
		retValue += it.Value;
	}
	return retValue;
}

int32 URefItem::GetTotalTagValueByFilter(TSet<FString> filters, int32& applyCount)
{
	int32 retValue = 0;
	applyCount = 0;

	for (auto& it : _tagWithValues) {
		FString key = it.Key.ToString();
		TArray<FString> keyArrays = UKismetStringLibrary::ParseIntoArray(key, ".");
		FString filter = keyArrays.Last();

		if (filters.Contains("default") == false && filters.Contains(filter) == false)
		{
			continue;
		}
		retValue += it.Value;
		applyCount++;
	}

	int32 deltaValue = 0;
	if (_isPrizeItem == true) {
		deltaValue = _tagWithValues["tag.fashion.special.reward"];
	}

	if (filters.Contains("default") == true) {
		deltaValue *= totalTagCount;
	}
	else {
		deltaValue *= filters.Num();
	}
	

	return retValue + deltaValue;
}

bool URefItem::HasTag(const FName& tagName)
{
	for (auto& pair : _tags) {
		if (pair.Key->UID.IsEqual(tagName)) {
			return true;
		}
	}
	return false;
}

void URefItemEquip::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	// override common equip item property
	Inven_Stack_Max = 1;
}

void URefItemEquip::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Item", "Equip",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

FName URefItemEquip::GetWeaponType() const
{
	if (typeId.IsSword()) {
		return "Sword";
	}
	else if (typeId.IsBow()) {
		return "Bow";
	}
	else if (typeId.IsArmor()) {
		return "Armor";
	}
	else if (typeId.IsHelmet()) {
		return "Helmet";
	}
	else if (typeId.IsBoots()) {
		return "Boots";
	}
	else if (typeId.IsGauntlet()) {
		return "Gauntlet";
	}

	else if (typeId.IsUnit()) {
		return "Unit";
	}
	return NAME_None;
}

FString URefItemEquip::GetWeaponAnimString() const
{
	if (typeId.IsSword()) {
		return "RK";
	}
	else if (typeId.IsBow()) {
		return "Blink";
	}
	else if (typeId.IsUnit()) {
		return "Abiliter";
	}
	return TEXT("");
}

void URefItemEquip::GetStatInfoData(FName type, TArray<FStatInfoData>& output)
{
	auto itr = _attributes.Find(type);
	if (itr == nullptr) {
		return;
	}

	const FName ApplySkillType = "Skill";
	auto IsFirstBasicSkill = [](const FEquipAttributeInfo& attribute){
		if (attribute.Apply_Effect.IsNone()) {
			return false;
		}

		const FString sUID{ attribute.Apply_UID.ToString() };
		const int32 index{ FCString::Atoi(*FString::Chr(sUID[sUID.Len() - 1])) };
		return index <= 1;
	};

	for (auto& refAttribute : *itr) {
		if (type == ApplySkillType && IsFirstBasicSkill(refAttribute->Attribute) == false) {
			continue;
		}
		output.Emplace(refAttribute->StatInfoData);
	}
}

URefEquipUpgradeEffect* URefItemEquip::GetUpgradeEffect(int32 upgrade)
{
	if (_upgradeEfts.IsValidIndex(upgrade - 1) == false) {
		return nullptr;
	}
	return _upgradeEfts[upgrade - 1];
}

URefEquipAttribute* URefItemEquip::GetAttribute(FName type)
{
	auto itr = _attributes.Find(type);
	if (itr != nullptr && itr->IsValidIndex(0)) {
		return (*itr)[0];
	}
	return nullptr;
}

FEquipAttributeInfo* URefItemEquip::GetAttribute(FName type, FName effect) const
{
	auto itr = _attributes.Find(type);
	if (itr != nullptr) {
		auto find = itr->FindByPredicate([effect](URefEquipAttribute* reference) {
			return reference->GetApplyEffect() == effect;
		});
		return find != nullptr ? &(*find)->Attribute : nullptr;
	}
	return nullptr;
}

void URefItemCostume::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	// override common costume item property
	Grade = 0;
	Inven_Stack_Max = 1;

	_defaultColors.SetNum(CUSTOMIZABLE_COLOR_COUNT);
}

void URefItemCostume::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Item", "Costume",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

void URefItemEtc::Parse(const FXmlNode* node)
{
	Super::Parse(node);
}

void URefItemEtc::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Item", "Etc",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

void URefItemEmblem::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	// override common emblem item property
	Inven_Stack_Max = 1;
}

void URefBase::ParseStatInfo(const FXmlNode* node, const FString& statColumn, TMap<EStatTypes, FStatInfoData>& output)
{
	for (const FXmlAttribute& attr : node->GetAttributes()) {
		const FString& columnName = attr.GetTag();
		int32 found = columnName.Find(statColumn);
		if (found == INDEX_NONE) {
			continue;
		}

		TArray<FString> values;
		URefBase::GetTrimedStringArray(attr.GetValue(), values);
		if (values.Num() != 2) {
			continue;
		}

		FStatInfoData statInfo;
		if (URefBase::ParseStatInfo(values[0], statInfo)) {
			statInfo.Value = FCString::Strtoi(*values[1], NULL, 10);
			EStatTypes statType = URefObject::GetStatEnum(statInfo.StatString);
			output.Emplace(statType, MoveTemp(statInfo));
		}
	}
}

void URefItemEmblem::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Item", "Emblem",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

bool URefItemEmblem::IsTranscendable() const
{
	return Grade >= URefItemEmblem::TranscendableGrade;
}

URefEmblemEffect* URefItemEmblem::GetUpgradeEffect(uint8 upgradeValue) const
{
	if (_upgradeEffects.IsValidIndex(upgradeValue) == false) {
		return nullptr;
	}
	return _upgradeEffects[upgradeValue];
}

int32 URefItemEmblem::GetUpgradeExp(int32 curUpgradeValue) const
{
	int32 index = FMath::Clamp(curUpgradeValue, 0, Upgrade_Exp.Num() - 1);
	check(Upgrade_Exp.IsValidIndex(index));
	return Upgrade_Exp[index];
}

int32 URefItemEmblem::GetMaterialExp(int32 upgradeValue) const
{
	int32 index = FMath::Clamp(upgradeValue, 0, Material_Exp.Num() - 1);
	check(Material_Exp.IsValidIndex(index));
	return Material_Exp[index];
}

void URefItemUsable::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Item", "Usable",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

void URefItemUsable::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	// override common usable property
	Grade = 0;
	Price = 0;
}

void URefItemDyeing::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Item", "Usable", "Dyeing",
	};

	_dyeingType = *node->GetAttribute("TID_4");

	typeNames.Append(FixedTypes);
	typeNames.Emplace(_dyeingType);

	URefObject::ParseTypeID(node, mapper);
}

void URefItemQuest::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	// override common usable property
	Grade = 0;
	Inven_Stack_Max = 9999;
	Price = 0;
}

void URefItemQuest::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Item", "Quest",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

////////////////////////////////////////////////////////////////////////////////////
EBodyParts URefBody::GetBodyParts(URefBody* refBody)
{
	return GetBodyParts(refBody->Part);
}

EBodyParts URefBody::GetBodyParts(const FName& partsName)
{
	static TMap<FName, EBodyParts> PARTS_MAPS{
		{URefBody::FaceType, EBodyParts::Face},
		{URefBody::SkinType, EBodyParts::Skin},
		{"Height", EBodyParts::Height},
		{"Frog", EBodyParts::Frog},
		{"Npc", EBodyParts::Npc},
	};

	auto itr = PARTS_MAPS.Find(partsName);
	return itr != nullptr ? (*itr) : EBodyParts::None;
}

void URefBody::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	
	_parts = GetBodyParts(this);
}

UTexture2D* URefBody::GetIconTexture(EGender gender/* = EGender::Female*/)
{
	return _displayInfo.GetIcon(gender);
}

FAnuResourceModelMesh* URefBody::GetResourceMesh(EGender gender/* = EGender::Female*/)
{
	if (_meshes.Num() == 0) {
		return nullptr;
	}

	uint8 genderIdx = FMath::Clamp(static_cast<uint8>(gender), (uint8)0, (uint8)(_meshes.Num() - 1));
	if (_meshes.IsValidIndex(genderIdx) == false) {
		genderIdx = 0;
	}

	return _meshes[genderIdx];
}

////////////////////////////////////////////////////////////////////////////////////
void URefCharacter::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	ParseStatData(node);
}

void URefCharacter::ParseStatData(const FXmlNode* node)
{
	Super::ParseStatData(node);

	for (const FXmlAttribute& attr : node->GetAttributes())
	{
		const FString& value = attr.GetTag();
		int32 found = value.Find("Stat_");
		if (found == INDEX_NONE) {
			continue;
		}
		const FName statName = *value.RightChop(found + 5);
		EStatTypes statType = GetStatEnum(statName);
		if (statType != EStatTypes::None) {
			_stats.Emplace(statType, FStatInfoData{ statName, statType, FCString::Strtoi(*attr.GetValue(), NULL, 10) });
		}
	}
}

FName URefCharacter::GetDialogType() const
{
	return UID;
}

void URefCharacter::GetDialogs(const FName& triggerType, TArray<URefDialog*>& dialogs)
{
	auto it = _dialogbyTriggerType.Find(triggerType);
	if (it == nullptr) {
		return;
	}

	dialogs.Append(*it);
}

void URefPC::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	static TMap<FName, EGender> GenderMap {
		{"Male", EGender::Male},
		{"Female", EGender::Female},
	};
	FString gender{ node->GetAttribute("Gender") };
	auto it = GenderMap.Find(*gender);
	Gender = it ? *it : EGender::Invalid;
}

void URefPC::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Character", "PC",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

void URefMonster::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	const FString& speed{ node->GetAttribute("RotationSpeedMS") };
	if(speed.IsEmpty() == false){
		RotationSpeed = FCString::Atof(*speed) * 0.001f;
	}

	FString hpCountArray = node->GetAttribute("HPCount");

	TArray<FString> splits;
	URefBase::GetTrimedStringArray(hpCountArray, splits);
	
	if (splits.Num() > 0) {
		HPCount = FCString::Atoi(*splits[0]);
	}

	for (int32 i = 1; i < splits.Num(); ++i) {
		PhaseRange.Emplace(FCString::Atof(*splits[i]));
	}
}

void URefMonster::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Character", "Monster",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

void URefLifeObject::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	auto enumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ELifeObjectSequencerOption"));
	int64 enumFlags = 0;
	for (auto optionName : Sequencer_Option) {
		enumFlags |= enumPtr->GetValueByNameString(optionName);
	}
	_sequencerOptionFlags = (ELifeObjectSequencerOption)enumFlags;

	MaxPriority = FMath::Max(MaxPriority, Priority);
}

void URefLifeObject::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		URefLifeObject::NAME_TID_1,
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_2"));
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

int32 URefLifeObject::GetTagOffset() const
{
	static TMap<int32, int32> TagOffsets {
		{TID2_BERRY, 50},
		{TID2_TREE, 100},
		{TID2_STONE, 100},
		{TID2_FISH, 0},
		{TID2_PORTAL, 100},
		{TID2_MANAGING, 0},
		{TID2_ANIMAL_SITTING, 100},
	};
	auto it = TagOffsets.Find(typeId.typeID2);
	return it ? *it : 0;
}

void URefPortal::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	FString permissionStr = node->GetAttribute("Permission");
	Permission = PermissionUtil::GetPermissionType(TCHAR_TO_UTF8(*permissionStr));
}

void URefPortal::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		URefLifeObject::NAME_TID_1,
		"Portal",
		"None",
		"None"
	};

	typeNames.Append(FixedTypes);

	URefObject::ParseTypeID(node, mapper);
}

void URefGimmick::Parse(const FXmlNode* node)
{
	Super::Parse(node);
}

void URefGimmick::ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper)
{
	static TArray<FName> FixedTypes{
		"Gimmick",
	};

	typeNames.Append(FixedTypes);
	typeNames.Emplace(*node->GetAttribute("TID_2"));
	typeNames.Emplace(*node->GetAttribute("TID_3"));
	typeNames.Emplace(*node->GetAttribute("TID_4"));

	URefObject::ParseTypeID(node, mapper);
}

////////////////////////////////////////////////////////////////////////////////////
bool URefCurrency::IsChargeable() const
{
	return Charge_Time != 0;
}

static UEnum* GetClassLicenseEnum()
{
	static UEnum* LicenseEnumType = nullptr;
	if (LicenseEnumType == nullptr || LicenseEnumType->IsValidLowLevel() == false) {
		LicenseEnumType = FindObject<UEnum>(ANY_PACKAGE, TEXT("EClassLicense"), true);
	}
	return LicenseEnumType;
}
EClassLicense URefClass::GetLicenceType(const FString& value)
{
	return static_cast<EClassLicense>(GetClassLicenseEnum()->GetValueByNameString(value));
}

FName URefClass::GetLicenseString(EClassLicense license)
{
	return GetClassLicenseEnum()->GetNameByValue(static_cast<int64>(license));
}

FText URefClass::GetLicenseDisplayText(EClassLicense license)
{
	return GetClassLicenseEnum()->GetDisplayNameTextByValue(static_cast<int64>(license));
}

void URefClass::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	FColor color{ FColor::FromHex(node->GetAttribute("Color")) };
	_color = FLinearColor::FromSRGBColor(color);

	VisitAttributes(node, "LicenseUnlock_QuestUID", [this](const FXmlAttribute* attr) {
		TArray<FString> licenseType;
		attr->GetTag().ParseIntoArray(licenseType, TEXT("_"));

		TArray<FString> questUID;
		URefBase::GetTrimedStringArray(attr->GetValue(), questUID);
		for (auto& it : questUID) {
			FName uid{ *it };
			if (uid.IsNone() == false) {
				OpenQuest_UID.FindOrAdd(GetLicenceType(licenseType[2])).Emplace(uid);
			}
		}
	});
}

void URefClass::ForeachOpenQuest(EClassLicense type, TFunction<bool(URefQuest*)>&& visitor)
{
	if (auto it = GetRTLicense(type); it._openRefQuest.Num() > 0) {
		for (auto& quest : it._openRefQuest) {
			if (visitor(quest) == false) { return; }
		}
	}
	else {
		visitor(nullptr);
	}
}

void URefClass::ForeachMastery(EClassLicense type, TFunction<bool(URefClassLicenseMastery*)>&& visitor)
{
	for (auto& mastery : GetRTLicense(type)._masteries) {
		if (visitor(mastery) == false) { return; }
	}
}

bool URefClass::FindOpenQuest(const FName& questUID) const
{
	for (auto& it : OpenQuest_UID) {
		for (auto& uid : it.Value) {
			if (uid == questUID) {
				return true;
			}
		}
	}
	return false;
}

const FText& URefClass::GetRawName() const
{
	return Name;
}

const FText& URefClass::GetRawEngName() const
{
	return Name_Title;
}

FRTRefClassLicense& URefClass::GetRTLicense(EClassLicense type)
{
	return _rtLicense.FindOrAdd(type);
}

URefQuest* URefClass::GetLicenseUnlockQuest(EClassLicense type)
{
	auto it = _rtLicense.Find(type);
	return (it && it->_openRefQuest.Num() > 0) ? it->_openRefQuest[0] : nullptr;
}

FText URefClass::GetLicenseDesc(EClassLicense type) const
{
	int32 index = FMath::Clamp(static_cast<int32>(type) - 1, 0, LicenseDesc.Num() - 1);
	return LicenseDesc.IsValidIndex(index) ? LicenseDesc[index] : FText::GetEmpty();
}

UTexture2D* URefClass::GetIconTexture() const
{
	return _iconTexture.LoadSynchronous();
}

void URefClassLicenseMastery::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	VisitAttributes(node, "Condition_Ref", [this](const FXmlAttribute* attr) {
		FConditionRule::Parse(_conditions, attr->GetValue());
	});
}

FText URefClassLicenseMastery::GetHintDesc()
{
	FString hintKey;
	FFormatNamedArguments fmtArgs;
	if (Desc_Detail_1.IsEmpty() == false) {
		fmtArgs.Emplace(TEXT("0"), FText::FormatNamed(Desc_Detail_1, "ObjectName", _refObject->GetName()));
		hintKey += TEXT("{0}\n");
	}
	if (Desc_Detail_2.IsEmpty() == false) {
		fmtArgs.Emplace(TEXT("1"), FText::FormatNamed(Desc_Detail_2, "ObjectName", _refObject->GetName()));
		hintKey += TEXT("{1}\n");
	}
	if (Desc_Detail_2.IsEmpty() == false) {
		fmtArgs.Emplace(TEXT("2"), FText::FormatNamed(Desc_Detail_3, "ObjectName", _refObject->GetName()));
		hintKey += TEXT("{2}");
	}

	if (hintKey.IsEmpty() == true) {
		return FText::GetEmpty();
	}
	return  FText::Format(FText::FromString(hintKey), fmtArgs);
}

////////////////////////////////////////////////////////////////////////////////////
void URefClassLevel::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	for (const FXmlAttribute& attr : node->GetAttributes())	{
		const FString& value = attr.GetTag();
		int32 found = value.Find("Stat_");
		if (found == INDEX_NONE) {
			continue;
		}

		const StatValue statValue{ FCString::Strtoi(*attr.GetValue(), NULL, 10) };
		if(statValue > 0){
			const FName& statName{ *value.RightChop(found + 5) };
			EStatTypes statType = URefObject::GetStatEnum(statName);
			_statGrowthValues.Emplace(FStatInfoData{ statName, statType, statValue });
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////

void URefStageInfo::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	
	FString currencyString = node->GetAttribute("Currency");
	TArray<FString> idxs;
	URefBase::GetTrimedStringArray(currencyString, idxs);

	checkf((idxs.Num() % 2) == 0, TEXT("StageInfo invalid: stage[%s] currency[%s]"), *UID.ToString(), *currencyString);
	for (int32 i = 0; i < idxs.Num(); i += 2) {
		Currency.Emplace(FName(idxs[i + 0]), FCString::Atoi(*idxs[i + 1]));
	}

	for (const FXmlAttribute& attr : node->GetAttributes())	{
		const FString& value = attr.GetTag();
		int32 found = value.Find("Mod_");
		if (found == INDEX_NONE) {
			continue;
		}

		const FName statName = *value.RightChop(found + 4);
		EStatTypes statType = URefObject::GetStatEnum(statName);
		Mod_Stat.Emplace(statType, FCString::Strtoi64(*attr.GetValue(), NULL, 10));
	}

	TArray<FString> entryCnt;
	URefBase::GetTrimedStringArray(node->GetAttribute("Entry_Count"), entryCnt);
	if (entryCnt.IsValidIndex(0)) {
		Entry_Max = FCString::Atoi(*entryCnt[0]);
	}
	if (entryCnt.IsValidIndex(1)) {
		Entry_Min = FCString::Atoi(*entryCnt[1]);
	}
}

bool URefStageInfo::UseMatchMaking() const
{
	const EStageType stageType{ GetStageType() };
	return stageType == EStageType::Multi || 
		stageType == EStageType::Game ||
		stageType == EStageType::Raid ||
		stageType == EStageType::Fashion;
}

EStageType URefStageInfo::GetStageType() const
{
	return _refStageGroup ? _refStageGroup->Type : EStageType::None;
}

UTexture2D* URefStageInfo::GetAssetIcon()
{
	UTexture2D* texture = Asset_Icon.Get();
	return texture ? texture : Asset_Icon.LoadSynchronous();
}

bool URefStageInfo::IsFinalStage() const
{
	for (auto& it : _redirectStage) {
		if (it->IsFinalStage()) {
			return true;
		}
	}

	return ResetSchedule.IsNone() && EnterCount == 1;
}

bool URefStageInfo::GetEnterCostInfo(int32 index, URefCurrency*& outCurrency, FName& outCurrencyUID, int32& outAmount) const
{
	check(_costCurrency.Num() == Currency.Num());
	if (_costCurrency.IsValidIndex(index) == false) {
		outCurrency = nullptr;
		outCurrencyUID = NAME_None;
		outAmount = 0;
		return false;
	}

	outCurrency = _costCurrency[index].Key;
	outAmount = _costCurrency[index].Value;
	outCurrencyUID = Currency[index].Key;
	return true;
}

void URefStageGroup::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	_typeString = node->GetAttribute("Type");

	// keyword
	VisitAttributes(node, "Keyword_", [this](const FXmlAttribute* attr) {
		FName keywordQuestUID{ *attr->GetValue() };
		if (keywordQuestUID != NAME_None) {
			Keywords.Emplace(keywordQuestUID);
		}
	});

	// Region
	static TMap<FName, EStageRegion> RegionTypes{
		{"None", EStageRegion::None},
		{"battle.region.1", EStageRegion::ParellelWorld},
		{"battle.region.2", EStageRegion::UnderColony},
		{"battle.region.3", EStageRegion::RedNova},
		{"battle.challenge.1", EStageRegion::Challange},
		{"battle.observation.1", EStageRegion::Observation},
		{"region.prototype", EStageRegion::Prototype},
	};

	if (EStageRegion* stageRegionType = RegionTypes.Find(Region)) {
		_stageRegion = *stageRegionType;
	}
}

void URefStageContest::Parse(const FXmlNode* node)
{
	static TMap<EContestType, EPermissionType> PlayerPermissions{
		{EContestType::FashionShow, EPermissionType::MMO_FashionShow},
		{EContestType::Championship, EPermissionType::MMO_Stage_Raid},
		{EContestType::Games, EPermissionType::MMO_Games},
	};

	static TMap<EContestType, EPermissionType> SpectatorPermissions{
		{EContestType::Championship, EPermissionType::MMO_Stage_Raid_Observation},
		{EContestType::FashionShow, EPermissionType::MMO_Stage_Raid_Observation},
	};

	Super::Parse(node);

	auto enumptr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EContestType"));
	_type = (EContestType)enumptr->GetValueByNameString(ContestType);
	check(_type != EContestType::None);
	auto& entry = map.FindOrAdd(_type);
	entry = this;

	if (auto permissionIter = PlayerPermissions.Find(_type)) {
		_playerPermission = *permissionIter;
	}

	if (auto permissionIter = SpectatorPermissions.Find(_type)) {
		_spectatorPermission = *permissionIter;
	}
}

void URefContestDonationGuide::Parse(const FXmlNode* node)
{
	Contest = FName(node->GetAttribute(TEXT("Contest")));
	GuideKey = FName(node->GetAttribute(TEXT("Key")));
	Value = node->GetAttribute(TEXT("Value"));
	_byContest.FindOrAdd(Contest).FindOrAdd(GuideKey) = this;
}

TMap<FName, TMap<FName, URefContestDonationGuide*>> URefContestDonationGuide::_byContest;

URefContestDonationGuide* URefContestDonationGuide::GetDonationGuide(FName contestUID, FName key)
{
	return _byContest.FindRef(contestUID).FindRef(key);
}

UTexture2D* URefChatStickerGroup::GetIconTexture() const
{
	return ImageTexture.LoadSynchronous();
}

////////////////////////////////////////////////////////////////////////////////////
void URefShopItem::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	FDynamicCost cost;
	cost.Parse(node);
	_cost.Emplace(MoveTemp(cost));

	VisitAttributes(node, "Apply_Tag", [this](const FXmlAttribute* attr) {
		auto value = attr->GetValue();
		if (value != "None") {
			ApplyTags.Emplace(value);
		}
	});

	// condition
	VisitAttributes(node, "Condition_Ref", [this](const FXmlAttribute* attr) {
		FString conditionValue{ attr->GetValue() };
		if (conditionValue.Compare("None") == 0) {
			return;
		}

		FConditionRule::Parse(Conditions, conditionValue);
	});
	VisitAttributes(node, "Condition_Text", [this](const FXmlAttribute* attr) {
		FString value{ attr->GetValue() };
		if (value.Compare("None") == 0) {
			return;
		}

		FText text{ AnuText::Get_CommonTable(value) };
		if (text.ToString() == FStringTableEntry::GetPlaceholderSourceString()) {
			text = AnuText::Get_UITable(value);
		}
		ConditionTexts.Emplace(text);
	});

	VisitAttributes(node, "Warn_Condition", [this](const FXmlAttribute* attr) {
		FString conditionValue{ attr->GetValue() };
		if (conditionValue.Compare("None") == 0) {
			return;
		}

		FConditionRule::Parse(_warnConditions, conditionValue);
	});
}

void URefShopItemInApp::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	_registeredInApp = true;
}

FText URefShopItem::GetName(EGender gender)
{
	return _target->_displayInfo.GetName(gender);
}

void URefShopItem::GetTargetDisplayInfos(EGender gender, TArray<FText>& outNames, TArray<int32>& outAmounts, TArray<UTexture2D*>& outIcons)
{
	if (_type == URefItem::StaticClass()) {
		SAFE_CAST(_target, URefItem, targetItem);
		outNames.Emplace(targetItem->GetName(gender));
		outAmounts.Emplace(Amount);
		outIcons.Emplace(targetItem->GetIconTexture(gender));
	}
	else if (_type == URefReward::StaticClass()) {
		SAFE_CAST(_target, URefReward, targetReward);
		for (auto& it : targetReward->RewardItems) {
			outNames.Emplace(it.Reward->GetName(gender));
			outAmounts.Emplace(Amount* it.Amount);
			
			if (auto icon = it.Reward->GetIconTexture(gender)) {
				outIcons.Emplace(icon);
			}
		}
	}
	else if (_type == URefCurrency::StaticClass()) {
		SAFE_CAST(_target, URefCurrency, targetCurrency);
		outNames.Emplace(targetCurrency->Name);
		outAmounts.Emplace(Amount);
		outIcons.Emplace(targetCurrency->GetIcon());
	}
}

URefTag* URefShopItem::GetFirstDisplayTag() const
{
	return _displayTags.Num() != 0 ? _displayTags[0] : nullptr;
}

int32 URefShopItem::GetGettableCount()
{
	int32 amount = Amount;
	if (URefReward* rwd = GetTargetImpl<URefReward>()) {
		amount *= (rwd->RewardType == ERewardType::Random ? 1 : rwd->_rewardCount);
	}
	return amount;
}

URefTag* URefShopItem::FindDisplayTag(const FName& tagUID) const
{
	auto it = _displayTags.FindByPredicate([tagUID](URefTag* tag) {
		return tag->UID == tagUID;
	});
	return it ? *it : nullptr;
}

FText URefShopItem::GetDescription(EGender gender)
{
	return _target->_displayInfo.GetDesc(gender);
}

int32 URefShopItem::GetTargetGuid() const
{
	return _target->GUID;
}

const FName& URefShopItem::GetTargetUID() const
{
	return _target->UID;
}

UTexture2D* URefShopItem::GetIcon(EGender gender)
{
	if (auto pc = GetTargetImpl<URefPC>()) {
		return pc->GetIcon2Texture(gender);
	}

	return _target->_displayInfo.GetIcon(gender);
}

FName URefShopItem::GetDisplayTagIcon()
{
	for (auto& tag : _displayTags) {
		if (tag->Icon.IsNone() == false) {
			return tag->Icon;
		}
	}
	return NAME_None;
}

URefItem* URefShopItem::GetDyeingIconTarget(EGender gender)
{
	if (auto targetItem = GetTargetImpl<URefItem>()) {
		return targetItem;
	}
	else if (auto targetReward = GetTargetImpl<URefReward>()) {
		int32 idx = FMath::Clamp(static_cast<int32>(gender), 0, _target->_displayInfo._icon.Num() - 1);
		auto shopItemIcon = _target->_displayInfo._icon[idx];

		for (auto& it : targetReward->RewardItems) {
			if (it.Reward->_item && it.Reward->_item->_displayInfo._icon.Find(shopItemIcon) != INDEX_NONE) {
				return it.Reward->_item;
			}
		}
	}
	return nullptr;
}

URefBase* URefShopItem::GetTarget(TSubclassOf<URefBase> clazz) const
{
	return _type == clazz ? _target : nullptr;
}

URefBase* URefShopItem::GetCost(TSubclassOf<URefBase> clazz) const
{
	for (auto& cost : _cost) {
		if (cost._type == clazz) {
			return cost._target;
		}
	}
	return nullptr;
}

URefBase* URefShopItem::GetCostTarget() const
{
	return _cost.Num() > 0 ? _cost[0]._target : nullptr;
}

FName URefShopItem::GetCostType() const
{
	return _cost.Num() > 0 ? _cost[0]._typeName : NAME_None;
}

FName URefShopItem::GetCostUID() const
{
	if (_cost.Num() <= 0) {
		return NAME_None;
	}
	auto& target = _cost[0]._target;
	return target ? target->UID : NAME_None;
}

int32 URefShopItem::GetCostGuid() const
{
	if (_cost.Num() <= 0) {
		return 0;
	}
	auto& target = _cost[0]._target;
	return target ? target->GUID : 0;
}

UTexture2D* URefShopItem::GetCostIcon(EGender gender) const
{
	auto cost = _cost.Num() > 0 ? &_cost[0] : nullptr;
	return cost && cost->UID.IsNone() == false ? cost->GetIcon(gender) : nullptr;
}

const FText& URefShopItem::GetCostName(EGender gender) const
{
	if (_cost.Num() <= 0) {
		return FText::GetEmpty();
	}
	auto& target = _cost[0]._target;
	return target ? target->_displayInfo.GetName(gender) : FText::GetEmpty();
}

void URefShopItem::GetCostItems(TMap<URefItem*, int32>& output) const
{
	for (auto& cost : _cost) {
		if (cost._type != URefItem::StaticClass()) {
			continue;
		}
		output.Emplace(static_cast<URefItem*>(cost._target), cost.GetOriginAmount());
	}
}

void URefShopItem::GetCostCurrency(TMap<URefCurrency*, int32>& output) const
{
	for (auto& cost : _cost) {
		if (cost._type != URefCurrency::StaticClass()) {
			continue;
		}
		output.Emplace(static_cast<URefCurrency*>(cost._target), cost.GetOriginAmount());
	}
}

bool URefShopItem::IsPaymentType() const
{
	if (_cost.Num() <= 0) {
		return false;
	}
	return _cost[0]._type == URefPayment::StaticClass();
}

bool URefShopItem::IsPostType() const
{
	if (_type == URefQuest::StaticClass() || _type == URefCurrency::StaticClass() || Target_Type == "RandomBox") {
		return false;
	}

	if (IsAdvertisementType()) {
		return false;
	}

	return _shopGroup->_isPostType;
}

bool URefShopItem::IsSubscriptionType() const
{
	return Target_Type == URefShopItem::SubscriptionType;
}

bool URefShopItem::IsAdvertisementType() const
{
	if (_cost.Num() <= 0) {
		return false;
	}
	return _cost[0]._type == URefAdvertisement::StaticClass();
}

//////////////////////////////////////////////////////////////////////////////////////
void URefShopGroup::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	static TSet<FName> PostType{ "InApp", "Mail", "Bridge", "Exchange" };
	_isPostType = (PostType.Find(Type) != nullptr);
}

bool URefShopGroup::IsPaymentType() const
{
	return _packages.FindByPredicate([](URefShopItem* shopItem){
		return shopItem->IsPaymentType();
	}) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////
void URefShopCost::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	_impl.Parse(node, "Target_Type", "Target_UID", "Amount");
	_impl.ParseDiscount(node);
}

bool URefQuestGroup::UseUserGroup() const
{
	return UserGroup != NAME_None;
}

void URefQuestGroup::GetRewards(TArray<FRewardInfo>& output)
{
	for (auto& it : _rewards) {
		output.Append(it->RewardItems);
	}
}

URefSchedule* URefQuestGroup::GetActiveSchedule(int32 index/* = 0*/) const
{
	index = FMath::Clamp(index, 0, _activeSchedules.Num() - 1);
	return _activeSchedules.IsValidIndex(index) ? _activeSchedules[index] : nullptr;
}

void URefQuestGroup::Parse(const FXmlNode* node)
{
	URefBase::Parse(node);

	ParseType(node);
}

void URefQuestGroup::ParseType(const FXmlNode* node)
{
	FString addType{ node->GetAttribute("Add_Type") };
	_manualAddType = (addType.Find("Manual") != INDEX_NONE);
	_initialStateIndex = (addType.Find("Progress") != INDEX_NONE) ? 1 : 0;
	_type = GetQuestGroupType(Type);
	checkf(_type != EQuestGroupType::Invalid, TEXT("[quest] QuestGroup[%s] bound with invalid Type[%s]"), *UID.ToString(), *Type.ToString());
}

URefSchedule* URefQuestGroup::GetCurAppendSchedule()
{
	if (_curAppendSchedule.IsSet() == false || (*_curAppendSchedule)->GetNextEndDate() < (*_curAppendSchedule)->GetNow()) {
		_curAppendSchedule.Reset();
		auto available = _appendSchedules.FindByPredicate([](URefQuestSchedule* qs) {
			return qs->_schedule->GetNow() < qs->_schedule->GetNextEndDate();
		});
		if (available) {
			_curAppendSchedule = (*available)->_schedule;
		}
	}
	return _curAppendSchedule.IsSet() ? _curAppendSchedule.GetValue() : nullptr;
}

void URefArbeitQuestGroup::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	static TMap<FString, EArbeitCategory> CATEGORY_TYPE{
		{ "None", EArbeitCategory::None },
		{ "Subscription", EArbeitCategory::Subscription },
		{ "Massive_Default", EArbeitCategory::Massive_Default },
		{ "Massive_Battle", EArbeitCategory::Massive_Battle },
		{ "Seasonal_Christmas", EArbeitCategory::Seasonal_Christmas },
		{ "Seasonal_Luna", EArbeitCategory::Seasonal_Luna },
		{ "Seasonal_Spring", EArbeitCategory::Seasonal_Spring },
		{ "Seasonal_Summer", EArbeitCategory::Seasonal_Summer },
		{ "Seasonal_Halloween", EArbeitCategory::Seasonal_Halloween },
	};

	CategoryType = CATEGORY_TYPE[Category];
	checkf(CategoryType != EArbeitCategory::Invalid, TEXT("[arbeit] ArbeitQuestGroup[%s] bound with invalid Category[%s]"), *UID.ToString(), *Category);
}

bool URefArbeitQuestGroup::IsScheduleAvailable() const
{
	bool available = true;
	for (auto& schedule : _activeSchedules) {
		if (schedule->IsAvailable() == false) {
			available = false;
			break;
		}
	}
	return available;
}

void URefStampTourQuestGroup::ParseType(const FXmlNode* node)
{
	_manualAddType = false;
	_initialStateIndex = 1;
	Type = "TimeEvent";
	_type = EQuestGroupType::TimeEvent;
}

void URefPassQuestGroup::ParseType(const FXmlNode* node)
{
	Super::ParseType(node);
	checkf(_type == EQuestGroupType::Attendance || _type == EQuestGroupType::PassMission, TEXT("QuestGroup_Pass[%s] bound with invalid Type[%s]; it must be Pass or PassMission"), *UID.ToString());
}

TMap<int32, TSet<EQuestProgress>> URefQuest::FeedTriggers;
void URefQuest::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	if (Class_Exp.Num() == 0) {
		Class_Exp.Emplace("None");
	}

	for (const FXmlAttribute& attr : node->GetAttributes()) {
		const FString& attrName = attr.GetTag();
		// condition
		int32 attrFound = attrName.Find("Accept_Condition_Ref");
		if (attrFound != INDEX_NONE) {
			FConditionRule::Parse(_acceptConditions, attr.GetValue());
		}
		attrFound = attrName.Find("Reward_Condition_Ref");
		if (attrFound != INDEX_NONE) {
			FConditionRule::Parse(_rewardConditions, attr.GetValue());
		}

		// feed trigger
		attrFound = attrName.Find("Feed_Trigger");
		if (attrFound != INDEX_NONE) {
			TArray<FString> strValues;
			FString feedTrigger{attr.GetValue()};
			URefBase::GetTrimedStringArray(feedTrigger, strValues);
			FName condition{ *strValues[0] };
			if (condition != NAME_None) {
				checkf(strValues.Num() > 1, TEXT("quest[%s] Feed_Trigger[%s] invalid: {quest.state} | {feed.uid}"), *feedTrigger);
				EQuestProgress prgState = URefQuest::GetProgressByStr(condition);
				auto& feeds = _feeds.FindOrAdd(prgState);
				for (int32 i = 1; i < strValues.Num(); i++) {
					feeds.Emplace(*strValues[i]);
				}
				auto& triggerStates = URefQuest::FeedTriggers.FindOrAdd(GUID);
				triggerStates.Emplace(prgState);
			}
		}

		// desc
		attrFound = attrName.Find("Desc");
		if (attrFound != INDEX_NONE) {
			TArray<FString> strValues;
			URefBase::GetTrimedStringArray(attr.GetValue(), strValues);
			for(auto& str : strValues) { _descKeys.Emplace(*str); }
		}
	}
}

int32 URefQuest::GetMaxAvailableCount() const
{
	auto it = _acceptConditions.FindByPredicate([](const FConditionRule& condition) {
		return condition.Rule == "OrderOfArrival";
	});
	return it ? it->MapValues.FindRef("OrderOfArrival") : INT_MAX;
}

int32 URefQuest::GetInitialStateIndex() const
{
	return _group->_initialStateIndex;
}

URefReward* URefQuest::GetMainReward() const
{
	if (_manualRewards.Num() != 0) {
		return _manualRewards[0];
	}

	if (_rewards.Num() != 0) {
		return _rewards[0];
	}

	return nullptr;
}

URefQuestChallenge* URefQuest::GetChallenge(QuestChallengeRankType rankType) const
{
	auto it = _challenges.FindByPredicate([rankType](URefQuestChallenge* challenge){
		return rankType == challenge->_rankingType;
	});
	return it ? *it : nullptr;
}

URefObject* URefQuest::GetRepresentativeHost()
{
	switch (_groupType)
	{
	case EQuestGroupType::Arbeit:
		return GetHost(URefNPC::StaticClass());
	case EQuestGroupType::Challenge:
		return GetHost(URefItem::StaticClass());
	case EQuestGroupType::Blacklist:
		return GetHost(URefMonster::StaticClass());
	default:
		break;
	}
	
	if (_hostObject.Num() == 0) {
		return nullptr;
	}
	auto it{ _hostObject.CreateIterator() };
	return it->Value;
}

URefQuestChallenge* URefQuest::GetChallengeByName(const FName& rankingName) const
{
	auto it = _challenges.FindByPredicate([&rankingName](URefQuestChallenge* challenge) {
		return rankingName == challenge->Ranking_Name;
	});
	return it ? *it : nullptr;
}

bool URefQuest::IsManualRewardArbeit() const
{
	return _groupType == EQuestGroupType::Arbeit && _manualRewards.Num() > 0;
}

TArray<URefQuestSequence*> URefQuest::GetQuestSequences(int32 beginIndex, int32 count)
{
	TArray<URefQuestSequence*> retValue;

	for (int i = 0; i < count; i++) {
		retValue.Add(_sequences[beginIndex -i]);
	}
	return retValue;
}

bool URefQuest::IsResetable() const
{
	return _resetSchedule != nullptr;
}

bool URefQuest::IsRepeatable() const
{
	check(Repeat >= 0);
	return IsInfiniteRepeatable() || Repeat != 1;
}

bool URefQuest::IsAccumulatable() const
{
	if (_sequences.Num() == 1) {
		URefQuestSequence* seq = _sequences[0];
		return seq->Condition_Count.Num() > 1;
	}
	return false;
}

void URefQuest::GetRewards(TArray<FRewardInfo>& output)
{
	for (auto& it : _rewards) {
		output.Append(it->RewardItems);
	}
}

void URefQuest::GetManualRewards(TArray<FRewardInfo>& output)
{
	for (auto& it : _manualRewards) {
		output.Append(it->RewardItems);
	}
}

URefObject* URefQuest::GetHost(TSubclassOf<URefObject> clazz)
{
	auto it = _hostObject.Find(clazz);
	return it ? *it : nullptr;
}

bool URefQuest::IsInfiniteRepeatable() const
{
	return Repeat == 0;
}

bool URefQuest::CheckRemainToComplete(int32 curCompleteCnt) const
{
	if (IsInfiniteRepeatable()) {
		return true;
	}
	return curCompleteCnt < Repeat;
}

void URefQuestSchedule::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	_schedule = NewObject<URefSchedule>(this);

	TArray<FString> dateStrs;
	dateStrs.Emplace(node->GetAttribute("StartDate"));
	dateStrs.Emplace(node->GetAttribute("EndDate"));
	bool res = _schedule->PeriodCondition.Parse("Period", FString::Join(dateStrs, TEXT("|")));
	checkf(res, TEXT("QuestGroup[%s] bound with invalid QuestActiveSchedule"), *QuestGroup_UID.ToString());
	_schedule->Use_Local_Time = (FCString::Atoi(*node->GetAttribute("Use_Local_Time")) != 0);
}

void URefQuestSequence::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	if (Host_Object.Num() == 0) {
		Host_Object.Emplace(URefQuestSequence::DefaultHostObject);
	}

	URefBase::GetIntegerArray(node->GetAttribute("Condition_Count"), Condition_Count);

	VisitAttributes(node, "Condition_Ref", [this](const FXmlAttribute* attr) {
		FConditionRule::Parse(_completeConditions, attr->GetValue());
	});
}

bool URefQuestSequence::UseNotifyWidget() const
{
	static TSet<EQuestGroupType> NotifyGroups{
		EQuestGroupType::Essential, EQuestGroupType::Class, EQuestGroupType::Epic, EQuestGroupType::Stage, EQuestGroupType::Arbeit, EQuestGroupType::MassiveArbeit,
	};
	return GetConditionCount() > 1 && _conditionTarget && NotifyGroups.Find(_quest->_groupType);
}

const FConditionRule* URefQuestSequence::GetRule(const FName& ruleName) const
{
	return _completeConditions.FindByPredicate([ruleName](const FConditionRule& condition) {
		return condition.Rule == ruleName;
	});
}

int32 URefQuestSequence::GetRuleCount(const FName& ruleName) const
{
	int32 count = 0;
	for (auto& condition : _completeConditions) {
		if (condition.Rule == ruleName) {
			count++;
		}
	}
	return count;
}

void URefQuestSequence::GetTakeItems(TMap<FName, int32>& output) const
{
	auto takeItemCondition = GetRule("TakeItem");
	if (takeItemCondition == nullptr) {
		return;
	}
	output.Append(takeItemCondition->MapValues);
}

FName URefQuestSequence::GetTargetDialog() const
{
	auto dlgCondition = GetRule("RefDialog");
	return dlgCondition ? *(dlgCondition->Values[0]) : FName("None");
}

FName URefQuestSequence::GetTargetObject() const
{
	auto objCondition = GetRule("Object");
	return objCondition ? *(objCondition->Values[0]) : FName("None");
}

FName URefQuestSequence::GetTargetQuest() const
{
	auto objCondition = GetRule("RefQuest");
	return objCondition ? *(objCondition->Values[0]) : FName("None");
}

FName URefQuestSequence::GetTargetCurrency() const
{
	auto objCondition = GetRule("RefCurrency");
	return objCondition ? *(objCondition->Values[0]) : FName("None");
}

FString URefQuestSequence::GetDebugString() const
{
	return FString::Printf(TEXT("%s:%d"), *Quest_UID.ToString(), Order);
}

int32 URefQuestSequence::GetConditionCount() const
{
	return Condition_Count[0];
}

FText URefQuestSequence::GetConditionText(FText originText) const
{
	return FText::FormatNamed(MoveTemp(originText), TEXT("Condition_Count"), GetConditionCount());
}

FText URefQuestSequence::GetTargetName(EGender gender) const
{
	return _displayInfo.GetName(gender);
}
UTexture2D* URefQuestSequence::GetTargetIcon(EGender gender) const
{
	return _displayInfo.GetIcon(gender);
}

bool URefQuestSequence::GetTimeContext(int32& goalSeconds) const
{
	static FName NAME_Time{"Time"};
	if (Condition != NAME_Time) {
		return false;
	}

	auto secondsCond = GetRule("Seconds");
	checkf(secondsCond, TEXT("QuestSequence[%s] condition is Time, but not bound with Seconds checker"), *GetDebugString());
	goalSeconds = FCString::Atoi(*secondsCond->Values[0]);
	return true;
}

void URefQuestSequence::GetRewards(TArray<FRewardInfo>& output) const
{
	for (auto& rwd : _reward) {
		output.Append(rwd->RewardItems);
	}
}

void URefQuestSequence::GetManualRewards(TArray<FRewardInfo>& output) const
{
	for (auto& rwd : _manualRewards) {
		output.Append(rwd->RewardItems);
	}
}

bool URefQuestSequence::IsConditionTargetItem() const
{
	return Cast<URefItem>(_conditionTarget) != nullptr;
}

bool URefQuestSequence::IsFirstSequence() const
{
	return _quest->_sequences[0] == this;
}

bool URefQuestSequence::IsLastSequence() const
{
	return _quest->_sequences[_quest->_sequences.Num() - 1] == this;
}

void URefQuestSequenceArbeit::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	_helpable = node->GetAttribute("Helpable").ToBool();
}

bool URefQuestPass::IsPointPass(URefQuest* quest)
{
	if (quest->_groupType != EQuestGroupType::Attendance) {
		return false;
	}
	SAFE_CAST(quest, URefQuestPass, pass);
	return pass->_missionGroup != nullptr;
}

FName URefQuestEvent::GetChallengeCeremonyEvtUID(URefQuest* quest)
{
	FString uid{ FString::Printf(TEXT("ch.ceremony.%s"), *quest->UID.ToString()) };
	return *uid;
}

void FConditionRule::Parse(TArray<FConditionRule>& dst, const FString& conditionStr)
{
	if (conditionStr.IsEmpty() || conditionStr.Compare("None") == 0) {
		return;
	}

	TArray<FString> strValues;
	URefBase::GetTrimedStringArray(conditionStr, strValues);

	FName rule{ *strValues[0] };
	strValues.RemoveAt(0);
	FConditionRule::AddRule(dst, rule, strValues);
}

void FConditionRule::AddRule(TArray<FConditionRule>& paramDest, const FName& rule, const TArray<FString>& values)
{
#define MAKE_RULE_PARSER(RuleType) \
	[](TArray<FConditionRule>& d, const FName& r, const TArray<FString>& v){ FConditionRule::AddRule_##RuleType(d, r, v); }

	static TMap<FName, TFunction<void(TArray<FConditionRule>&, const FName&, const TArray<FString>&)>> CustomAdder {
		{"OrderOfArrival", MAKE_RULE_PARSER(OrderOfArrival) },
		{"TakeItemBundle", MAKE_RULE_PARSER(TakeItemBundle) },
		{"TakeItem", MAKE_RULE_PARSER(Item) },
		{"HaveItem", MAKE_RULE_PARSER(Item) },
		{"HaveCurrency", MAKE_RULE_PARSER(Item) },
		{"EquipItem", MAKE_RULE_PARSER(Equip) },
	};

	auto customAdder = CustomAdder.Find(rule);
	if (customAdder == nullptr) { // use default adder
		FConditionRule cond;
		cond.Rule = rule;
		cond.Values.Append(values);
		paramDest.Emplace(cond);
		return;
	}
	(*customAdder)(paramDest, rule, values);
}

void FConditionRule::AddRule_OrderOfArrival(TArray<FConditionRule>& dst, const FName& rule, const TArray<FString>& values)
{
	checkf(values.Num() == 1, TEXT("quest condition[OrderOfArrival] param count[%d] invalid: expected-> OrderOfArrival | count"), values.Num());
	int32 count{ FCString::Atoi(*values[0]) };

	FConditionRule cond;
	cond.Rule = rule;
	cond.MapValues.Emplace("OrderOfArrival", count);
	dst.Emplace(cond);
}

void FConditionRule::AddRule_Item(TArray<FConditionRule>& dst, const FName& rule, const TArray<FString>& values)
{
	checkf(values.Num() == 2, TEXT("HaveItem/TakeItem checker param count[%d] invalid: expected-> HaveItem/TakeItem | item.uid | amount"), values.Num());

	FName itemUID{ *values[0] };
	int32 amount{ FCString::Atoi(*values[1]) };

	auto oldRule = dst.FindByPredicate([rule](const FConditionRule& ruleParam) {
		return rule == ruleParam.Rule;
	});
	if (oldRule == nullptr) {
		FConditionRule cond;
		cond.Rule = rule;
		cond.Items.Emplace(MakeTuple(itemUID, amount));
		dst.Emplace(cond);
		return;
	}

	auto oldItemRule = oldRule->Items.FindByPredicate([itemUID](const TPair<FName, int32>& value) {
		return itemUID == value.Key;
	});
	if (oldItemRule == nullptr) {
		oldRule->Items.Emplace(itemUID, amount);
		return;
	}
	*oldItemRule = MakeTuple(itemUID, amount + oldItemRule->Value);
}

void FConditionRule::AddRule_Equip(TArray<FConditionRule>& dst, const FName& rule, const TArray<FString>& values)
{
	FName checkType = *values[0];
	auto condition = dst.FindByPredicate([rule](const FConditionRule& ruleParam) {
		return rule == ruleParam.Rule;
	});
	
	if (condition == nullptr) {
		FConditionRule cond;
		cond.Rule = rule;
		dst.Emplace(cond);
		condition = &dst[dst.Num() - 1];
	}

	if (checkType == "ClassUID") {
		checkf(values.Num() == 3, TEXT("EquipItem checker param count[%d] invalid: expected - checkType | classUID | slotNumber"), values.Num());
		FName classUID = *values[1];
		int32 slotNumber = FCString::Atoi(*values[2]);
		condition->EquipClassItems.Emplace(classUID, slotNumber);
	}
	else {
		// unsupported
	}
}

void FConditionRule::AddRule_TakeItemBundle(TArray<FConditionRule>& dst, const FName& rule, const TArray<FString>& values)
{
	int32 valueNum = values.Num();
	checkf(valueNum % 2 == 0, TEXT("TakeItemBundle format error"));
	for (int32 i = 0; i < valueNum; i += 2) {
		TArray<FString> itemValues;
		itemValues.Emplace(values[i]);
		itemValues.Emplace(values[i + 1]);
		AddRule(dst, "TakeItem", itemValues);
	}
}

bool URefQuest::IsUseHUDQuest(EQuestGroupType group)
{
	static TSet<EQuestGroupType> NoUseHUDGroup {
		EQuestGroupType::Hidden, EQuestGroupType::Keyword, EQuestGroupType::Hidden_Event, EQuestGroupType::ContentsGuide
	};
	return NoUseHUDGroup.Find(group) == nullptr;
}

EQuestProgress URefQuest::GetProgressByStr(const FName& prgStr)
{
	static TMap<FName, EQuestProgress> QuestProgressMap{
		{"Acceptable", EQuestProgress::Acceptable},
		{"Progress", EQuestProgress::Progress},
		{"Complete", EQuestProgress::Complete},
	};
	auto it = QuestProgressMap.Find(prgStr);
	checkf(it != nullptr, TEXT("QuestProgress checker binded with invalid progress[%s]"), *prgStr.ToString());
	return *it;
}

QuestChallengeRankType URefQuestChallenge::GetRankingType(const FName& rankingName)
{
	static TMap<FName, QuestChallengeRankType> Types {
		{ "ch.main", QuestChallengeRankType::Default },
		{ "ch.sub.1", QuestChallengeRankType::Effort },
		{ "ch.sub.2", QuestChallengeRankType::Lucky },
		{ "ch.sub.3", QuestChallengeRankType::Extra },
	};
	auto it = Types.Find(rankingName);
	checkf(it, TEXT("QuestChallenge: not registered ranking type[%s]. check Ranking_Name"), *rankingName.ToString());
	return *it;
}

void URefQuestChallenge::Parse(const FXmlNode* node)
{
	static TMap<FName, RankingMethod> MethodMaps{
		{"Add", RankingMethod::Add},
		{"Submit", RankingMethod::Submit_Highest},
		{"Submit_Lowest", RankingMethod::Submit_Lowest},
	};

	URefBase::Parse(node);
	_rankingType = GetRankingType(Ranking_Name);
	
	auto methodIter = MethodMaps.Find(Ranking_Type);
	checkf(methodIter, TEXT("QuestChallenge[%s] bound with invalid Ranking_Type[%s]"), *GetDebugString(), *Ranking_Type.ToString());
	_rankingMethod = *methodIter;
	_submitRanking = (_rankingMethod == RankingMethod::Submit_Highest || _rankingMethod == RankingMethod::Submit_Lowest);

	if (Upload_Actor.Num() == 1 && Upload_Actor[0].IsNone()) {
		Upload_Actor.Empty();
	}
}

FString URefQuestChallenge::GetDebugString() const
{
	return FString::Printf(TEXT("%s:%s"), *Quest_UID.ToString(), *Ranking_Name.ToString());
}

bool URefQuestChallenge::UseSubmitNotify() const
{
	if (_submitRanking == false) {
		return false;
	}

	URefQuestSequence* condition = _quest->_sequences[0];
	return condition->Condition != "Act_TakePhoto";
}

FText URefQuestChallenge::GetName(EGender gender) const
{
	if (URefObject* host = _quest->GetRepresentativeHost()) {
		return FText::FormatNamed(Name, "Host_Object", host->GetName(gender));
	}
	return Name;
}

URefObject* URefQuestChallenge::GetHost(TSubclassOf<URefObject> clazz)
{
	auto it = _hostObjects.FindByPredicate([clazz](URefObject* host) {
		return host->IsA(clazz);
	});
	return it ? *it : nullptr;
}

bool URefQuestChallenge::IsMainRanking() const
{
	return _rankingType == QuestChallengeRankType::Default;
}

static const UEnum* StatEnumType = nullptr;
void InitStatEnumTable() 
{
	if (StatEnumType == nullptr || StatEnumType->IsValidLowLevel() == false) {
		StatEnumType = FindObject<UEnum>(ANY_PACKAGE, TEXT("EStatTypes"), true);
	}
}

EStatTypes URefObject::GetStatEnum(const FName& text)
{
	InitStatEnumTable();
	int32 index = StatEnumType->GetIndexByName(text);
	return index != INDEX_NONE ? EStatTypes(index) : EStatTypes::None;
}

FName URefObject::GetStatString(EStatTypes statType)
{
	InitStatEnumTable();
	return *(StatEnumType->GetNameStringByIndex((uint8)statType));
}

FText URefObject::GetStatText(EStatTypes statType)
{
	FString rowName{ "str.stat." + URefObject::GetStatString(statType).ToString() };
	return AnuText::Get_UITable(rowName.ToLower());
}

TMap<FName, EAttachmentParts> URefObject::PartsByName = {
	{"RWeapon", EAttachmentParts::AP_RWeapon},
	{"LWeapon", EAttachmentParts::AP_LWeapon},
	{"RHand", EAttachmentParts::AP_RHand},
	{"Phone", EAttachmentParts::AP_LHand},
	//{"LRing", EAttachmentParts::AP_LRing},
	{"Handwear", EAttachmentParts::AP_LBracelet},
	{"LBracelet", EAttachmentParts::AP_LBracelet},
	{"Gloves", EAttachmentParts::AP_Gloves},
	{"Hat", EAttachmentParts::AP_Hat},
	{"Eyewear", EAttachmentParts::AP_Eyewear},
	{"Mask", EAttachmentParts::AP_Mask},
	{"Back", EAttachmentParts::AP_Back},
	{"Tail", EAttachmentParts::AP_Tail},
	{"Ear", EAttachmentParts::AP_Ear},
	{"Upper", EAttachmentParts::AP_Necklace},
	{"Necklace", EAttachmentParts::AP_Necklace},
	{"Armband", EAttachmentParts::AP_Armband},
	{"Lens", EAttachmentParts::AP_Lens},
	{"MakeUp", EAttachmentParts::AP_MakeUp},
	{"Hair", EAttachmentParts::AP_Hair},
	{URefBody::FaceType, EAttachmentParts::AP_Face},
	{"Outfit", EAttachmentParts::AP_Outfit},
	{"Shoes", EAttachmentParts::AP_Shoes},
};

EAttachmentParts URefObject::GetAttachmentPart(const FName& partName)
{
	auto it = PartsByName.Find(partName);
	return it ? *it : EAttachmentParts::AP_Invalid;
}

TMap<FName, EQuestGroupType> URefQuestGroup::GroupTypes = {
	{ "Tutorial", EQuestGroupType::Tutorial },
	{ "Arbeit", EQuestGroupType::Arbeit },
	{ "Epic", EQuestGroupType::Epic },
	{ "Keyword", EQuestGroupType::Keyword },
	{ "Hidden", EQuestGroupType::Hidden },
	{ "Fairy", EQuestGroupType::Fairy },
	{ "Activity", EQuestGroupType::Activity },
	{ "Essential", EQuestGroupType::Essential },
	{ "Hidden_Event", EQuestGroupType::Hidden_Event },
	{ "Stage", EQuestGroupType::Stage },
	{ "Blacklist", EQuestGroupType::Blacklist },
	{ "Challenge", EQuestGroupType::Challenge },
	{ "Prototype", EQuestGroupType::Prototype },
	{ "Contents_Guide", EQuestGroupType::ContentsGuide },
	{ "Class", EQuestGroupType::Class },
	{ "TimeEvent", EQuestGroupType::TimeEvent },
	{ "Pass", EQuestGroupType::Attendance },
	{ "PassMission", EQuestGroupType::PassMission},
	{ "MassiveArbeit", EQuestGroupType::MassiveArbeit },
};
EQuestGroupType URefQuestGroup::GetQuestGroupType(const FName& group)
{
	auto itr = URefQuestGroup::GroupTypes.Find(group);
	return itr ? *itr : EQuestGroupType::Invalid;
}

TSet<EQuestGroupType> URefQuestGroup::CompleteUITypes {
	EQuestGroupType::Tutorial, EQuestGroupType::Essential, EQuestGroupType::Epic, EQuestGroupType::Class,
	EQuestGroupType::Arbeit, EQuestGroupType::MassiveArbeit,
};

FName URefQuestGroup::GetQuestGroupTypeName(EQuestGroupType type)
{
	for (auto& it : URefQuestGroup::GroupTypes) {
		if (it.Value == type) {
			return it.Key;
		}
	}
	return NAME_None;
}

const uint8 URefCustomDetail::GetIntValue(EGender gender)
{
	uint8 index = FMath::Clamp(static_cast<uint8>(gender), (uint8)0, (uint8)(_numValue.Num() - 1));
	if (_numValue.IsValidIndex(index)) {
		return _numValue[index];
	}

	return 0;
}

void URefColor::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	{
		FColor color{ FColor::FromHex(node->GetAttribute("DisplayColor")) };
		_displayLinear = FLinearColor::FromSRGBColor(color);
	}
	{
		FString applyColor = node->GetAttribute("ApplyColor");
		FColor color{ FColor::FromHex(applyColor) };
		_applyLinear = FLinearColor::FromSRGBColor(color);
		_applyColorHex = FParse::HexNumber(*applyColor);
	}
}

bool URefColor::IsDefaultColor() const
{
	return _applyLinear.A == 0.f;
}

void URefColorPigment::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	_impl.Parse(node, "Target_Type", "Target_UID", "Amount");
	_impl.ParseDiscount(node);
}

void URefPalette::Parse(const FXmlNode* node)
{
	Super::Parse(node);
}

FString URefPalette::GetDebugString() const
{
	return FString::Printf(TEXT("%s:%s"), *Group_UID.ToString(), *Color_UID.ToString());
}

URefBase* URefPalette::GetCost(TSubclassOf<URefBase> clazz) const
{
	if (_group == nullptr) {
		return nullptr;
	}

	const auto& cost = _group->_dyeingCost;
	return clazz == cost._type ? cost._target : nullptr;
}

UTexture2D* URefPalette::GetCostIcon(EGender gender) const
{
	if (_group == nullptr) {
		return nullptr;
	}

	const auto& cost = _group->_dyeingCost;
	return cost.GetIcon(gender);
}

void URefPaletteGroup::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	_dyeingCost.Parse("Item", node->GetAttribute("Dyeing_Cost_UID"), node->GetAttribute("Dyeing_Cost_Value"));
	_unlockCost.Parse(node, "Unlock_Cost_Type", "Unlock_Cost_UID", "Unlock_Cost_Value");
	if (Parts.Num() != 0 && Parts[0].IsNone()) {
		Parts.Empty();
	}
}

bool URefPaletteGroup::IsDefault() const
{
	return Display_Type == "Default" && _unlockCost._target == nullptr;
}

UTexture2D* URefPaletteGroup::GetIconTexture() const
{
	return _displayInfo._icon.Num() != 0 ? _displayInfo._icon[0].LoadSynchronous() : nullptr;
}

#if WITH_EDITOR
void URefCurve::SetValue(FRuntimeFloatCurve curve)
{
	curve.GetRichCurve()->BakeCurve(TimeDistance);
	for (FRichCurveKey& curveKey : curve.GetRichCurve()->Keys) {
		Values.Append(FString::SanitizeFloat(curveKey.Value));
		if (curve.GetRichCurve()->GetLastKey() != curveKey) {
			Values.Append(",");
		}
	}
}
#endif

bool URefBase::ParseStatInfo(const FString& statColumn, FStatInfoData& output)
{
	if (statColumn.Compare("None") == 0) {
		return false;
	}

	int32 underbarIndex = INDEX_NONE;
	statColumn.FindLastChar('_', underbarIndex);
	checkf(underbarIndex != INDEX_NONE, TEXT("statType[%s] parse failed; format must be> [Stat]_[ApplyType] ex) MaxHP_Add"), *statColumn);

	const FString& applyType = statColumn.RightChop(underbarIndex + 1);
	const FString& stat = statColumn.LeftChop(applyType.Len() + 1);
	EStatTypes statType = URefObject::GetStatEnum(*stat);
	if (statType == EStatTypes::None) {
		return false;
	}

	static const UEnum* ApplyEnumType = FindObject<UEnum>(ANY_PACKAGE, TEXT("EStatApplyType"), true);
	EStatApplyType statApplyType = EStatApplyType::Invalid;
	int32 index = ApplyEnumType->GetIndexByName(*applyType);
	if (index != INDEX_NONE) {
		statApplyType = (EStatApplyType)index;
	}
	if (statApplyType == EStatApplyType::Invalid) {
		check(false);
		return false;
	}

	output.StatType = statType;
	output.ApplyType = statApplyType;
	output.StatString = *stat;
	return true;
}

int32 URefBase::ParseInteger(const FString& attrValue, bool infMax)
{
	static FString INFINITY_STRING{"inf"};
	if (INFINITY_STRING.Compare(attrValue, ESearchCase::IgnoreCase) == 0) {
		return infMax ? TNumericLimits<int32>::Max() : TNumericLimits<int32>::Min();
	}
	return FCString::Atoi(*attrValue);
}

void URefEquipMolding::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	FString attrName;
	for (const FXmlAttribute& attr : node->GetAttributes()) {
		attrName = attr.GetTag();
		int32 found = attrName.Find("Material_Item_UID_");
		if(found != INDEX_NONE){
			if(attrName.FindLastChar('_', found)) {
				int32 index = FCString::Atoi(*attrName.RightChop(found + 1));
				Material_Item_UID.EmplaceAt(index - 1, attr.GetValue());
			}
			continue;
		}

		found = attrName.Find("Material_Item_Count_");
		if (found != INDEX_NONE) {
			if (attrName.FindLastChar('_', found)) {
				int32 index = FCString::Atoi(*attrName.RightChop(found + 1));
				Material_Item_Count.EmplaceAt(index - 1, FCString::Atoi(*attr.GetValue()));
			}
		}
	}
}

void URefEquipMolding::GetMaterials(TArray<URefItem*>& outItems, TArray<int32>& outCounts) const
{
	for (auto& it : _materials) {
		outItems.Emplace(it.Key);
		outCounts.Emplace(it.Value);
	}
}

void FDynamicCost::Parse(const FXmlNode* node, const std::string& typeColName, const std::string& uidColName, const std::string& valColName)
{
	Parse(node->GetAttribute(typeColName.c_str()), node->GetAttribute(uidColName.c_str()), node->GetAttribute(valColName.c_str()));
}

void FDynamicCost::Parse(const FString& type, const FString& uid, const FString& amount)
{
	static TMap<FName, TSubclassOf<URefBase>> CostTypes{
		{URefCurrency::TableName, URefCurrency::StaticClass()},
		{"Item", URefItem::StaticClass()},
		{"QuestHost", URefQuest::StaticClass()},
		{"ShopCost", URefShopCost::StaticClass()},
		{"Payment", URefPayment::StaticClass()},
		{URefShopItem::AdvertisementType, URefAdvertisement::StaticClass()},
		{"ShopGroup", URefShopGroup::StaticClass()},
		{"VIP", URefVIP::StaticClass()}
	};
	_typeName = *type;
	if (_typeName.IsNone()) {
		return;
	}

	auto it = CostTypes.Find(_typeName);
	checkf(it, TEXT("dynamic cost parse failed; invalid Cost_Type[%s]"), *_typeName.ToString());
	_type = *it;

	UID = *uid;
	Amount = FCString::Atoi(*amount);
}

void FDynamicCost::ParseDiscount(const FXmlNode* node)
{
	Discount_Schedule = *node->GetAttribute("Discount_Schedule");
	Discount_Value = FCString::Atoi(*node->GetAttribute("Discount_Value"));
}

UTexture2D* FDynamicCost::GetIcon(EGender gender) const
{
	return _target->_displayInfo.GetIcon(gender);
}

const FText& FDynamicCost::GetName(EGender gender) const
{
	return _target->_displayInfo.GetName(gender);
}

int32 FDynamicCost::GetOriginAmount() const
{
	return Amount;
}

void FDynamicCost::SetOriginAmount(int32 amount)
{
	Amount = amount;
}

int32 URefSchedule::ServerLocalTimeZone = 0;
bool URefSchedule::IsAvailable(const FPeriodCondition& condition, const FDateTime& curDate, bool useLocalTime)
{
	FDateTime fromDate;
	FDateTime toDate;

	switch (condition.PeriodType)
	{
	case FPeriodCondition::Type::None:
		return true;
	case FPeriodCondition::Type::Daily:
		fromDate = FPeriodCondition::GetLastDateWithTime(curDate, condition.PeriodValue_From);
		toDate = FPeriodCondition::GetNextDateWithTime(fromDate, condition.PeriodValue_To);
		break;
	case FPeriodCondition::Type::Weekly:
		toDate = FPeriodCondition::GetNextDateWithDayOfWeek(curDate, condition.PeriodValue_To);
		fromDate = toDate - condition.PeriodDeltaTime;
		break;
	case FPeriodCondition::Type::Weekly_Time: {
		if (condition.NumberArgs.Find(FPeriodCondition::GetDayOfWeekInTmRule(curDate.GetDayOfWeek())) == INDEX_NONE) {
			return false;
		}
		fromDate = FDateTime(curDate.GetYear(), curDate.GetMonth(), curDate.GetDay(), condition.PeriodValue_From.tm_hour, condition.PeriodValue_From.tm_min, condition.PeriodValue_From.tm_sec);
		toDate = FPeriodCondition::GetNextDateWithTime(fromDate, condition.PeriodValue_To);
	} break;
	case FPeriodCondition::Type::Monthly:
		if (curDate.GetDay() != condition.PeriodValue_From.tm_mday) {
			return false;
		}
		fromDate = FDateTime(curDate.GetYear(), curDate.GetMonth(), curDate.GetDay(), condition.PeriodValue_From.tm_hour, condition.PeriodValue_From.tm_min, condition.PeriodValue_From.tm_sec);
		toDate = FPeriodCondition::GetNextDateWithTime(fromDate, condition.PeriodValue_To);
		break;
	case FPeriodCondition::Type::Period:
		fromDate = FDateTime(condition.PeriodValue_From.tm_year, condition.PeriodValue_From.tm_mon, condition.PeriodValue_From.tm_mday, condition.PeriodValue_From.tm_hour, condition.PeriodValue_From.tm_min, condition.PeriodValue_From.tm_sec);
		toDate = FDateTime(condition.PeriodValue_To.tm_year, condition.PeriodValue_To.tm_mon, condition.PeriodValue_To.tm_mday, condition.PeriodValue_To.tm_hour, condition.PeriodValue_To.tm_min, condition.PeriodValue_To.tm_sec);
		break;
	case FPeriodCondition::Type::Period_Yearly:
		fromDate = FDateTime(curDate.GetYear(), condition.PeriodValue_From.tm_mon, condition.PeriodValue_From.tm_mday, condition.PeriodValue_From.tm_hour, condition.PeriodValue_From.tm_min, condition.PeriodValue_From.tm_sec);
		toDate = FDateTime(curDate.GetYear(), condition.PeriodValue_To.tm_mon, condition.PeriodValue_To.tm_mday, condition.PeriodValue_To.tm_hour, condition.PeriodValue_To.tm_min, condition.PeriodValue_To.tm_sec);
		if (toDate < fromDate) {
			toDate = FDateTime(curDate.GetYear() + 1, condition.PeriodValue_To.tm_mon, condition.PeriodValue_To.tm_mday, condition.PeriodValue_To.tm_hour, condition.PeriodValue_To.tm_min, condition.PeriodValue_To.tm_sec);
		}
		break;
	case FPeriodCondition::Type::Duration:
	{
		fromDate = condition.GetLastStartDate(curDate, useLocalTime);
		toDate = fromDate;
		toDate += FTimespan(0, 0, condition.PeriodValue_To.tm_sec);
	}
	break;
	}

	return fromDate <= curDate && curDate <= toDate;
}

bool URefSchedule::AnyAvailableSchedule(const TArray<URefSchedule*>& schedules)
{
	if (schedules.Num() == 0) { // not exist any schedule constraint? available!
		return true;
	}

	for (auto& it : schedules) {
		if (it->IsAvailable()) {
			return true;
		}
	}
	return false;
}

FDateTime URefSchedule::GetServerNow()
{
	FDateTime now{ FDateTime::UtcNow() };
	now += FTimespan(0, 0, ServerLocalUtcDelta);
	return now;
}

FDateTime URefSchedule::GetAnuNow(EAnuTimeZoneRule tzRule)
{
	switch (tzRule)
	{
	case EAnuTimeZoneRule::Utc0:
		return FDateTime::UtcNow();
		break;
	case EAnuTimeZoneRule::ServerLocal:
		return GetServerNow();
		break;	
	default:
		break;
	}
	return FDateTime::Now();
}

FDateTime URefSchedule::ConvertServerLocalToUtc(const FDateTime& serverLocalTime)
{
	return serverLocalTime - FTimespan(0, 0, ServerLocalUtcDelta);
}

FDateTime URefSchedule::ConvertUtcToServerLocal(const FDateTime& utcTime)
{
	return utcTime + FTimespan(0, 0, ServerLocalUtcDelta);
}

FDateTime URefSchedule::ConvertLocalToUtc(const FDateTime& clientLocalTime)
{
	return clientLocalTime - FTimespan(0, 0, ClientLocalUtcDelta);
}

FDateTime URefSchedule::ConvertUtcToLocal(const FDateTime& utcTime)
{
	return utcTime + FTimespan(0, 0, ClientLocalUtcDelta);
}

FDateTime URefSchedule::ConvertServerTimeToLocalUTCTime(const FDateTime& serverTime)
{
	FDateTime value;
	auto utc = FDateTime::UtcNow();
	auto local = FDateTime::Now();
	FTimespan diff = local - utc;
	return ConvertServerLocalToUtc(serverTime) + diff;
}

FDateTime URefSchedule::ConvertServerTimeToLocalTime(const FDateTime& serverTime)
{
	FDateTime value;
	auto utc = FDateTime::UtcNow();
	auto local = FDateTime::Now();
	FTimespan diff = local - utc;
	return serverTime + diff;
}

FPeriodCondition::Type FPeriodCondition::GetPeriodType(const FName& periodType)
{
	static TMap<FName, FPeriodCondition::Type> s_type{
		{"Daily", Type::Daily},
		{"Weekly", Type::Weekly},
		{"Weekly_Time", Type::Weekly_Time},
		{"Monthly", Type::Monthly},
		{"Period", Type::Period},
		{"Period_Yearly", Type::Period_Yearly},
		{"Duration", Type::Duration},
		{"Duration_Offset", Type::Duration},
	};

	auto found = s_type.Find(periodType);
	return found ? *found : Type::None;
}

bool FPeriodCondition::Parse(const FName& Period_Type, const FString& Period_Value)
{
	if (Period_Type.IsNone()) {
		return false;
	}
	checkf(Period_Value != "None", TEXT("Period_Value invalid: Period_Type[%s], Period_Value[%s]"), *Period_Type.ToString(), *Period_Value);
	PeriodType = GetPeriodType(Period_Type);

	TArray<FString> periodValues;
	URefBase::GetTrimedStringArray(Period_Value, periodValues);

	switch (PeriodType)
	{
	case Type::Daily: {
		if (periodValues.Num() == 1) {
			StringToTime(periodValues[0], PeriodValue_From);
			StringToTime(periodValues[0], PeriodValue_To);
		}
		else if (periodValues.Num() == 2) {
			StringToTime(periodValues[0], PeriodValue_From);
			StringToTime(periodValues[1], PeriodValue_To);
		}
		else {
			checkf(false, TEXT("Period_Type[Daily] format error; expected> hh:mm:ss | hh:mm:ss or hh:mm:ss"));
		}
	} break;
	case Type::Weekly: {
		checkf(periodValues.Num() == 4, TEXT("Period_Type[Weekly] format error; expected> [DayOfWeek: MON-SUN]|[hh:mm:ss] } | [DayOfWeek: MON-SUN]|[hh:mm:ss]"));
		StringToDayOfWeek(periodValues[0], PeriodValue_From);
		if (StringToTime(periodValues[1], PeriodValue_From) == false) {
			return false;
		}
		StringToDayOfWeek(periodValues[2], PeriodValue_To);
		if (StringToTime(periodValues[3], PeriodValue_To) == false) {
			return false;
		}

		FDateTime startDate{ GetLastDateWithDayOfWeek(FDateTime::Now(), PeriodValue_From) };
		FDateTime endDate{ GetNextDateWithDayOfWeek(startDate, PeriodValue_To) };
		PeriodDeltaTime = endDate - startDate;
	} break;
	case Type::Weekly_Time: {
		checkf(periodValues.Num() >= 3, TEXT("Period_Type[Weekly_Time] format error; expected> [DayOfWeek: MON-SUN...]|[hh:mm:ss]|[hh:mm:ss]"));
		int32 i = 0;
		for (; i < periodValues.Num() - 2; i++) {
			tm timeStruct{ 0, };
			StringToDayOfWeek(periodValues[i], timeStruct);
			NumberArgs.Emplace(timeStruct.tm_wday);
		}
		checkf(NumberArgs.Num() != 0, TEXT("Period_Type[Weekly_Time] must be some of day of week"));
		if(StringToTime(periodValues[i++], PeriodValue_From) == false) {
			return false;
		}
		if(StringToTime(periodValues[i++], PeriodValue_To) == false) {
			return false;
		}
	} break;
	case Type::Monthly: {
		checkf(periodValues.Num() == 3, TEXT("Period_Type[Monthly] format error; expected> [DayOfMonth: 1-31] | [hh:mm:ss] | [hh:mm:ss]"));
		PeriodValue_From.tm_mday = FCString::Atoi(*periodValues[0]);
		if(PeriodValue_From.tm_mday <= 0 || PeriodValue_From.tm_mday > 31) break;
		if(StringToTime(periodValues[1], PeriodValue_From) == false) {
			return false;
		}
		if(StringToTime(periodValues[2], PeriodValue_To) == false) {
			return false;
		}
	} break;
	case Type::Period: {
		checkf(periodValues.Num() == 2, TEXT("Period_Type[Period] format error; expected> [yyyy-mm-dd hh:mm:ss] | [yyyy-mm-dd hh:mm:ss]"));
		if(StringToDate(periodValues[0], PeriodValue_From) == false) {
			return false;
		}
		if(StringToDate(periodValues[1], PeriodValue_To) == false) {
			return false;
		}
	} break;
	case Type::Period_Yearly: {
		checkf(periodValues.Num() == 2, TEXT("Period_Type[Period_Yearly] format error; expected> [mm-dd hh:mm:ss] | [mm-dd hh:mm:ss]"));
		FString from{"0000-" + periodValues[0]};
		FString to{ "0000-" + periodValues[1] };
		if(StringToDate(from, PeriodValue_From) == false) {
			return false;
		}
		if(StringToDate(to, PeriodValue_To) == false) {
			return false;
		}
		PeriodValue_From.tm_year = 0;
		PeriodValue_To.tm_year = 0;
	} break;
	case Type::Duration: {
		checkf(periodValues.Num() >= 2, TEXT("Period_Type[Duration] format error; expected> [start_interval_min] | [duration_min]"));
		if (periodValues.Num() == 2) {
			PeriodValue_To.tm_min = FCString::Atoi(*periodValues[0]);
			PeriodValue_To.tm_sec = FCString::Atoi(*periodValues[1]) * 60;
		}
		else {
			if (StringToTime(periodValues[0], PeriodValue_From) == false) {
				return false;
			}
			PeriodValue_To.tm_min = FCString::Atoi(*periodValues[1]);
			PeriodValue_To.tm_sec = FCString::Atoi(*periodValues[2]) * 60;
		}
		checkf(PeriodValue_To.tm_min * 60 >= PeriodValue_To.tm_sec, TEXT("Duration Period_Value[%s] error. duration must be lower than interval"), *Period_Value);
	} break;
	}
	return true;
}

bool FPeriodCondition::StringToTime(const FString & hhmmss, tm& output)
{
	TArray<FString> times;
	hhmmss.ParseIntoArray(times, TEXT(":"));
	if (times.Num() != 3) {
		return false;
	}
	
	output.tm_hour = FCString::Atoi(*times[0]);
	if (output.tm_hour < 0 || output.tm_hour > 23) {
		return false;
	}

	output.tm_min = FCString::Atoi(*times[1]);
	if (output.tm_min < 0 || output.tm_min > 59) {
		return false;
	}

	output.tm_sec = FCString::Atoi(*times[2]);
	if (output.tm_sec < 0 || output.tm_sec > 59) {
		return false;
	}

	return true;
}

void FPeriodCondition::StringToDayOfWeek(const FString & MONtoSUN, tm& output)
{
	constexpr uint8 NUMBER_OF_DAYS_IN_WEEK = 7;
	TArray<FString> s_dayOfWeekStrs{
		"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
	};

	for (uint8 i = 0; i < NUMBER_OF_DAYS_IN_WEEK; ++i) {
		if (s_dayOfWeekStrs[i] == MONtoSUN) {
			output.tm_wday = i;
			return;
		}
	}
	checkf(false, TEXT("cannot parse [%s] to day of week; expected> SUN, MON, TUE, ..."), *MONtoSUN);
}

bool FPeriodCondition::StringToDate(const FString & yyyymmddhhmmss, tm& output)
{
	TArray<FString> values;
	yyyymmddhhmmss.ParseIntoArray(values, TEXT("+"));

	if (values.Num() != 2) {
		return false;
	}

	TArray<FString> dates;
	values[0].ParseIntoArray(dates, TEXT("-"));
	
	if (dates.Num() != 3) {
		return false;
	}

	output.tm_year = FCString::Atoi(*dates[0]);
	output.tm_mon = FCString::Atoi(*dates[1]);
	output.tm_mday = FCString::Atoi(*dates[2]);

	return StringToTime(values[1], output);
}

FDateTime FPeriodCondition::GetLastDateWithTime(const FDateTime& srcDate, const tm& time)
{
	FDateTime resultDate{ srcDate.GetYear(), srcDate.GetMonth(), srcDate.GetDay(), time.tm_hour, time.tm_min, time.tm_sec };
	if (resultDate > srcDate) {
		constexpr int64 SECONDS_IN_DAY = 60 * 60 * 24;
		resultDate -= FTimespan(0, 0, SECONDS_IN_DAY);
	}
	return resultDate;
}

FDateTime FPeriodCondition::GetLastDateWithDayOfWeek(const FDateTime& srcDate, const tm& time)
{
	FDateTime resultDate{ srcDate.GetYear(), srcDate.GetMonth(), srcDate.GetDay(), time.tm_hour, time.tm_min, time.tm_sec };
	int32 deltaDays = GetDayOfWeekInTmRule(resultDate.GetDayOfWeek()) - time.tm_wday;
	constexpr int32 DAYS_IN_A_WEEK{ 7 };
	if (deltaDays == 0) { // hit day, then compare time
		FTimespan temp{ time.tm_hour, time.tm_min, time.tm_sec };
		if (srcDate.GetTimeOfDay().GetTotalSeconds() < temp.GetTotalSeconds()) {
			deltaDays += DAYS_IN_A_WEEK;
		}
	}
	else if (deltaDays < 0) {
		deltaDays += DAYS_IN_A_WEEK;
	}
	check(deltaDays >= 0);
	resultDate -= FTimespan(deltaDays, 0, 0, 0);
	return resultDate;
}

FDateTime FPeriodCondition::GetLastDateWithDayOfMonth(const FDateTime& srcDate, const tm& time)
{
	auto GetResultMonth = [](const FDateTime& src, uint8 day, const tm& t) {
		constexpr uint8 MONTHS_IN_A_YEAR{ 12 };
		uint8 lastMonth = src.GetMonth() - 1;
		uint32 lastYear = src.GetYear();
		if (lastMonth == 0) {
			lastMonth = MONTHS_IN_A_YEAR;
			--lastYear;
		}
		return FDateTime(lastYear, lastMonth, day, t.tm_hour, t.tm_min, t.tm_sec);
	};

	int32 deltaDays = time.tm_mday - srcDate.GetDay();
	if (deltaDays > 0) {
		return GetResultMonth(srcDate, time.tm_mday, time);
	}
	else if (deltaDays < 0) {
		return srcDate + FTimespan(deltaDays, 0, 0, 0);
	}
	else { // hit day, then compare time
		FTimespan temp{ time.tm_hour, time.tm_min, time.tm_sec };
		if (srcDate.GetTimeOfDay().GetTotalSeconds() < temp.GetTotalSeconds()) {
			return GetResultMonth(srcDate, time.tm_mday, time);
		}
	}
	return FDateTime{ srcDate.GetYear(), srcDate.GetMonth(), srcDate.GetDay(), time.tm_hour, time.tm_min, time.tm_sec };
}

FDateTime FPeriodCondition::GetNextDateWithTime(const FDateTime& srcDate, const tm& time)
{
	FDateTime resultDate{ srcDate.GetYear(), srcDate.GetMonth(), srcDate.GetDay(), time.tm_hour, time.tm_min, time.tm_sec };
	if (resultDate < srcDate) {
		resultDate += FTimespan(1, 0, 0, 0); // add one-day
	}
	return resultDate;
}

uint8 FPeriodCondition::GetDayOfWeekInTmRule(EDayOfWeek dayOfWeek)
{
	constexpr uint8 DAYS_IN_A_WEEK = 7;
	uint8 dayOfWeekNum = static_cast<uint8>(dayOfWeek);
	dayOfWeekNum = (++dayOfWeekNum) % DAYS_IN_A_WEEK;
	return dayOfWeekNum;
}

FDateTime FPeriodCondition::GetNextDateWithDayOfWeek(const FDateTime& srcDate, const tm& dayOfWeekAndTime)
{
	int32 deltaDays = dayOfWeekAndTime.tm_wday - FPeriodCondition::GetDayOfWeekInTmRule(srcDate.GetDayOfWeek());
	constexpr int32 DAYS_IN_A_WEEK{ 7 };
	FDateTime resultDate{ srcDate.GetYear(), srcDate.GetMonth(), srcDate.GetDay(), dayOfWeekAndTime.tm_hour, dayOfWeekAndTime.tm_min, dayOfWeekAndTime.tm_sec };
	if (deltaDays == 0) { // hit day, then compare time
		if (resultDate < srcDate) {
			deltaDays += DAYS_IN_A_WEEK;
		}
	}
	else if (deltaDays < 0) {
		deltaDays += DAYS_IN_A_WEEK;
	}
	check(deltaDays >= 0);
	resultDate += FTimespan(deltaDays, 0, 0, 0);
	return resultDate;
}

FDateTime FPeriodCondition::GetNextDateWithDayOfMonth(const FDateTime& srcDate, const tm& dayOfMonthAndTime)
{
	auto GetNextMonthDate = [](const FDateTime& src, uint8 day) {
		constexpr uint8 MONTHS_IN_A_YEAR{ 12 };
		uint8 nextMonth = src.GetMonth() + 1;
		uint32 nextYear = src.GetYear();
		if (nextMonth > MONTHS_IN_A_YEAR) {
			nextMonth = 1;
			++nextYear;
		}
		return FDateTime(nextYear, nextMonth, day);
	};

	FDateTime resultDate{ srcDate.GetYear(), srcDate.GetMonth(), srcDate.GetDay(), dayOfMonthAndTime.tm_hour, dayOfMonthAndTime.tm_min, dayOfMonthAndTime.tm_sec };
	int32 deltaDays = dayOfMonthAndTime.tm_mday - srcDate.GetDay();
	if (deltaDays > 0) {
		resultDate += FTimespan(deltaDays, 0, 0, 0);
	}
	else if (deltaDays < 0) {
		FDateTime nextMonthDate = GetNextMonthDate(srcDate, dayOfMonthAndTime.tm_mday);
		resultDate = FDateTime(nextMonthDate.GetYear(), nextMonthDate.GetMonth(), nextMonthDate.GetDay(), dayOfMonthAndTime.tm_hour, dayOfMonthAndTime.tm_min, dayOfMonthAndTime.tm_sec);
	}
	else { // hit day, then compare time			
		if (resultDate < srcDate) {
			FDateTime nextMonthDate = GetNextMonthDate(srcDate, dayOfMonthAndTime.tm_mday);
			resultDate = FDateTime(nextMonthDate.GetYear(), nextMonthDate.GetMonth(), nextMonthDate.GetDay(), dayOfMonthAndTime.tm_hour, dayOfMonthAndTime.tm_min, dayOfMonthAndTime.tm_sec);
		}
	}
	return resultDate;
}

FDateTime FPeriodCondition::GetLastStartDate(bool useLocalTime) const
{
	return GetLastStartDate(useLocalTime ? FDateTime::Now() : FDateTime::UtcNow(), useLocalTime);
}

FDateTime FPeriodCondition::GetLastStartDate(const FDateTime& targetTime, bool useLocalTime) const
{
	switch (PeriodType)
	{
	case FPeriodCondition::Type::Daily:
		return FPeriodCondition::GetLastDateWithTime(targetTime, PeriodValue_From);
	case FPeriodCondition::Type::Weekly:
		return FPeriodCondition::GetLastDateWithDayOfWeek(targetTime, PeriodValue_From);
	case FPeriodCondition::Type::Weekly_Time: {
		tm timeStruct{ PeriodValue_From };
		timeStruct.tm_wday = GetDayOfWeekInTmRule(targetTime.GetDayOfWeek());
		while (NumberArgs.Find(timeStruct.tm_wday) == INDEX_NONE) {
			int32 prevDayOfWeek = timeStruct.tm_wday - 1;
			prevDayOfWeek = prevDayOfWeek < 0 ? 6 : prevDayOfWeek;
			timeStruct.tm_wday = prevDayOfWeek;
		}
		return FPeriodCondition::GetLastDateWithDayOfWeek(targetTime, timeStruct);
	}
	case FPeriodCondition::Type::Monthly:
		return FPeriodCondition::GetLastDateWithDayOfMonth(targetTime, PeriodValue_From);
	case FPeriodCondition::Type::Period:
		return FDateTime{ PeriodValue_From.tm_year, PeriodValue_From.tm_mon, PeriodValue_From.tm_mday, PeriodValue_From.tm_hour, PeriodValue_From.tm_min, PeriodValue_From.tm_sec };
	case FPeriodCondition::Type::Period_Yearly: {
		FDateTime last{ targetTime.GetYear(), PeriodValue_From.tm_mon, PeriodValue_From.tm_mday, PeriodValue_From.tm_hour, PeriodValue_From.tm_min, PeriodValue_From.tm_sec };
		if (last > targetTime) {
			last = FDateTime{ targetTime.GetYear() - 1, PeriodValue_From.tm_mon, PeriodValue_From.tm_mday, PeriodValue_From.tm_hour, PeriodValue_From.tm_min, PeriodValue_From.tm_sec };
		}
		return last;
	} break;
	case FPeriodCondition::Type::Duration: {
		FDateTime dt{ targetTime.GetYear(), targetTime.GetMonth(), targetTime.GetDay(), PeriodValue_From.tm_hour, PeriodValue_From.tm_min, PeriodValue_From.tm_sec };

		int64 curTotalMinutes = targetTime.GetTimeOfDay().GetTotalMinutes();
		dt += FTimespan(0, curTotalMinutes / PeriodValue_To.tm_min * PeriodValue_To.tm_min, 0);
		if (dt > targetTime) {
			dt -= FTimespan(0, PeriodValue_To.tm_min, 0);
		}
		return dt;
	} break;
	default:
		check(false);
		break;
	}
	return targetTime;
}

FDateTime FPeriodCondition::GetNextStartDate(const FDateTime& localDate, bool useLocalTime) const
{
	switch (PeriodType)
	{
	case FPeriodCondition::Type::Daily:
		return FPeriodCondition::GetNextDateWithTime(localDate, PeriodValue_From);
	case FPeriodCondition::Type::Weekly:
		return FPeriodCondition::GetNextDateWithDayOfWeek(localDate, PeriodValue_From);
	case FPeriodCondition::Type::Weekly_Time: {
		tm timeStruct{ PeriodValue_From };
		timeStruct.tm_wday = GetDayOfWeekInTmRule(localDate.GetDayOfWeek());
		while (NumberArgs.Find(timeStruct.tm_wday) == INDEX_NONE) {
			timeStruct.tm_wday = (timeStruct.tm_wday + 1) % 7;
		}
		return FPeriodCondition::GetNextDateWithDayOfWeek(localDate, timeStruct);
	}
	case FPeriodCondition::Type::Monthly:
		return FPeriodCondition::GetNextDateWithDayOfMonth(localDate, PeriodValue_From);
	case FPeriodCondition::Type::Period:
		return FDateTime(PeriodValue_From.tm_year, PeriodValue_From.tm_mon, PeriodValue_From.tm_mday, PeriodValue_From.tm_hour, PeriodValue_From.tm_min, PeriodValue_From.tm_sec);
	case FPeriodCondition::Type::Period_Yearly: {
		FDateTime next{ localDate.GetYear(), PeriodValue_From.tm_mon, PeriodValue_From.tm_mday, PeriodValue_From.tm_hour, PeriodValue_From.tm_min, PeriodValue_From.tm_sec };
		if (next < localDate) {
			next = FDateTime(localDate.GetYear() + 1, PeriodValue_From.tm_mon, PeriodValue_From.tm_mday, PeriodValue_From.tm_hour, PeriodValue_From.tm_min, PeriodValue_From.tm_sec);
		}
		return next;
	} break;
	case FPeriodCondition::Type::Duration: {
		// get last start date
		FDateTime dt{ localDate.GetYear(), localDate.GetMonth(), localDate.GetDay(), PeriodValue_From.tm_hour, PeriodValue_From.tm_min, PeriodValue_From.tm_sec };
		int64 curTotalMinutes = localDate.GetTimeOfDay().GetTotalMinutes();
		dt += FTimespan(0, curTotalMinutes / PeriodValue_To.tm_min * PeriodValue_To.tm_min, 0);
		if (dt > localDate) {
			dt -= FTimespan(0, PeriodValue_To.tm_min, 0);
		}

		dt += FTimespan(0, PeriodValue_To.tm_min, 0);
		return dt;
	} break;
	default:
		check(false);
		break;
	}
	return localDate;
}

FDateTime FPeriodCondition::GetNextStartDate(bool useLocalTime) const
{
	return GetNextStartDate(useLocalTime ? FDateTime::Now() : FDateTime::UtcNow(), useLocalTime);
}

FDateTime FPeriodCondition::GetNextEndDate(bool useLocalTime) const
{
	return GetNextEndDate(useLocalTime ? FDateTime::Now() : FDateTime::UtcNow(), useLocalTime);
}

FDateTime FPeriodCondition::GetNextEndDate(const FDateTime& now, bool useLocalTime) const
{
	switch (PeriodType)
	{
	case FPeriodCondition::Type::Daily:
		return FPeriodCondition::GetNextDateWithTime(now, PeriodValue_To);
	case FPeriodCondition::Type::Weekly:
		return FPeriodCondition::GetNextDateWithDayOfWeek(now, PeriodValue_To);
	case FPeriodCondition::Type::Weekly_Time: {
		tm timeStruct{ PeriodValue_To };
		timeStruct.tm_wday = GetDayOfWeekInTmRule(now.GetDayOfWeek());
		while (NumberArgs.Find(timeStruct.tm_wday) == INDEX_NONE) {
			timeStruct.tm_wday = (timeStruct.tm_wday + 1) % 7;
		}
		return FPeriodCondition::GetNextDateWithDayOfWeek(now, timeStruct);
	}
	case FPeriodCondition::Type::Monthly:
		return FPeriodCondition::GetNextDateWithDayOfMonth(now, PeriodValue_To);
	case FPeriodCondition::Type::Period:
		return FDateTime(PeriodValue_To.tm_year, PeriodValue_To.tm_mon, PeriodValue_To.tm_mday, PeriodValue_To.tm_hour, PeriodValue_To.tm_min, PeriodValue_To.tm_sec);
	case FPeriodCondition::Type::Period_Yearly: {
		FDateTime next{ now.GetYear(), PeriodValue_To.tm_mon, PeriodValue_To.tm_mday, PeriodValue_To.tm_hour, PeriodValue_To.tm_min, PeriodValue_To.tm_sec };
		if (next < now) {
			next = FDateTime{ now.GetYear() + 1, PeriodValue_To.tm_mon, PeriodValue_To.tm_mday, PeriodValue_To.tm_hour, PeriodValue_To.tm_min, PeriodValue_To.tm_sec };
		}
		return next;
	} break;
	case FPeriodCondition::Type::Duration: {
		FDateTime dt{ URefSchedule::IsAvailable(*this, useLocalTime ? URefSchedule::GetServerNow() : FDateTime::UtcNow(), useLocalTime) ? GetLastStartDate(now, useLocalTime) : GetNextStartDate(now, useLocalTime) };
		dt += FTimespan(0, 0, PeriodValue_To.tm_sec);
		return dt;
	} break;
	default:
		check(false);
		break;
	}
	return now;
}

int32 FPeriodCondition::GetDurationInSeconds(bool useLocalTime) const
{
	constexpr int32 SecondsInDay = 60 * 60 * 24;
	constexpr int32 SecondsInWeek = SecondsInDay * 7;

	FDateTime now{ useLocalTime ? FDateTime::Now() : FDateTime::UtcNow() };
	switch (PeriodType)
	{
	case FPeriodCondition::Type::Daily:
		return SecondsInDay;
	case FPeriodCondition::Type::Weekly:
		return SecondsInWeek;
	case FPeriodCondition::Type::Monthly:
		return FDateTime::DaysInMonth(now.GetYear(), now.GetMonth()) * SecondsInDay;
	break;
	case FPeriodCondition::Type::Weekly_Time:
	case FPeriodCondition::Type::Period:
		return static_cast<int32>((GetNextEndDate(useLocalTime) - GetNextStartDate(useLocalTime)).GetTotalSeconds());
	case FPeriodCondition::Type::Period_Yearly: {
		FDateTime start{ URefSchedule::IsAvailable(*this, useLocalTime ? URefSchedule::GetServerNow() : FDateTime::UtcNow(), useLocalTime) ? GetLastStartDate(useLocalTime) : GetNextStartDate(useLocalTime) };
		FDateTime end{ GetNextEndDate(useLocalTime) };
		return static_cast<int32>((end - start).GetTotalSeconds());
	} break;
	case FPeriodCondition::Type::Duration:
		return PeriodValue_To.tm_sec;
	break;
	default:
		break;
	}
	return 0;
}

FDateTime URefSchedule::GetNow() const
{
	FDateTime now{ FDateTime::UtcNow() };
	if (Use_Local_Time) {
		now += FTimespan(0, 0, ServerLocalUtcDelta);
	}
	return now;
}

FDateTime URefSchedule::GetLastStartDate()
{
	return PeriodCondition.GetLastStartDate(GetNow(), Use_Local_Time);
}

FDateTime URefSchedule::GetLastStartDateWith(const FDateTime& targetTime)
{
	return PeriodCondition.GetLastStartDate(targetTime, Use_Local_Time);
}

FDateTime URefSchedule::GetNextStartDate()
{
	return GetNextStartDateWith(GetNow());
}

FDateTime URefSchedule::GetNextStartDateWith(const FDateTime& targetTime)
{
	if (PeriodCondition.PeriodType == FPeriodCondition::Type::None) {
		return targetTime;
	}
	return	PeriodCondition.GetNextStartDate(targetTime, Use_Local_Time);
}

FDateTime URefSchedule::GetNextEndDate()
{
	return GetNextEndDateWith(GetNow());
}

FDateTime URefSchedule::GetNextEndDateWith(const FDateTime& targetTime)
{
	if (PeriodCondition.PeriodType == FPeriodCondition::Type::None) {
		return FDateTime();
	}
	return	PeriodCondition.GetNextEndDate(targetTime, Use_Local_Time);
}

FDateTime URefSchedule::GetNextStartDateInUtc()
{
	auto nextStartDate = GetNextStartDate();
	if (Use_Local_Time) {
		nextStartDate = URefSchedule::ConvertServerLocalToUtc(nextStartDate);
	}
	return nextStartDate;
}

FDateTime URefSchedule::GetNextEndDateInUtc()
{
	auto nextEndDate = GetNextEndDate();
	if (Use_Local_Time) {
		nextEndDate = URefSchedule::ConvertServerLocalToUtc(nextEndDate);
	}
	return nextEndDate;
}

bool URefSchedule::IsToday(const FDateTime& now)
{
	if (IsAvailable()) {
		return true;
	}
	else {
		FDateTime lastStartDate{ GetLastStartDateWith(now) };
		if (lastStartDate.GetDayOfYear() == now.GetDayOfYear()) {
			return true;
		}

		FDateTime nextEndDate{ GetNextEndDateWith(now) };
		if (nextEndDate.GetDayOfYear() == now.GetDayOfYear()) {
			return true;
		}
	}
	return false;
}

bool URefSchedule::IsExpired()
{
	if (IsAvailable()) {
		return false;
	}

	const auto& nextEndDate = GetNextEndDate();
	return nextEndDate < GetNow();
}

float URefSchedule::GetDurationSeconds() const
{
	return static_cast<float>(PeriodCondition.GetDurationInSeconds(Use_Local_Time));
}

bool URefSchedule::IsAvailable() const
{
	return URefSchedule::IsAvailable(PeriodCondition, GetNow(), Use_Local_Time);
}

void URefSchedule::Parse(const FXmlNode* node)
{
	URefBase::Parse(node);
	bool res = PeriodCondition.Parse(Period_Type, Period_Value);
	checkf(res, TEXT("[schedule] Schedule[%s] has invalid Period_Type[%s] or Period_Value[%s]"), *Period_Type.ToString(), *Period_Value);
}

FString URefDialog::RootFieldName = "Type";
ESkipType URefDialog::GetSkipType(const FName& skipTypeStr)
{
	static TMap<FName, ESkipType> SkipTypes{
		{"Selectable", ESkipType::Selectable},
		{"Auto", ESkipType::Auto},
		{"Never", ESkipType::Never},
		{"Auto_Quest", ESkipType::Auto_Quest},
	};
	auto it = SkipTypes.Find(skipTypeStr);
	checkf(it, TEXT("invalid dialog skip type[%s]"), *skipTypeStr.ToString());
	return *it;
}

void URefDialog::Parse(const FJsonObject* root)
{
	URefBase::Parse(root);

	FString strValue;
	{ // optional properties
		for (auto& it : root->Values) {
			if (it.Key.Contains("Trigger_Value") == false) {
				continue;
			}
			FConditionRule::Parse(_triggerConditions, it.Value->AsString());
		}

		if (root->TryGetStringField("Skip_Type", strValue)) {
			SkipType = URefDialog::GetSkipType(*strValue);
		}
	}
	
	// dialog sub
	const TArray<TSharedPtr<FJsonValue>>* dlgSubs;
	if (root->TryGetArrayField("DialogSub", dlgSubs) == false) {
		checkf(false, TEXT("[dialog] uid[%s] json must have DialogSub"), *UID.ToString());
		return;
	}

	for (auto& dlgSub : *dlgSubs) {
		const TSharedPtr<FJsonObject>* dlgSubObj = nullptr;
		if (dlgSub->TryGetObject(dlgSubObj) == false) {
			checkf(false, TEXT("[dialog] uid[%s] json has invalid DialogSub; it must be array of {object}s"), *UID.ToString());
			return;
		}
		URefDialogSub* dlgSubRef = NewObject<URefDialogSub>(this);
		dlgSubRef->Dlg_UID = UID.ToString();
		dlgSubRef->ID = _speeches.Num() + 1;
		dlgSubRef->StringTableID = dlgSubRef->ID;
		const FJsonObject* dlgSubJsonObj = dlgSubObj->Get();
		URefBase::ParseJson(dlgSubJsonObj, dlgSubRef);
		_speeches.Emplace(dlgSubRef);

		// dialog events
		const TArray<TSharedPtr<FJsonValue>>* arrayValues;
		if (dlgSubJsonObj->TryGetArrayField("Event", arrayValues)) {
			for (auto& dlgEvt : *arrayValues) {
				const TSharedPtr<FJsonObject>* dlgEvtObj = nullptr;
				if (dlgEvt->TryGetObject(dlgEvtObj) == false) {
					checkf(false, TEXT("[dialog] uid[%s] json has invalid Event; it must be array of {object}s; check DialogSub[%d]"), *UID.ToString(), dlgSubRef->ID);
					continue;
				}

				FString str;
				if (dlgEvtObj->Get()->TryGetStringField("Type", str) == false) {
					continue;
				}

				URefDialogEvent* dlgEvtRef = NewObject<URefDialogEvent>(this);
				URefBase::ParseJson(dlgEvtObj->Get(), dlgEvtRef);
				URefBase::GetTrimedStringArray(dlgEvtRef->Type_Value, dlgEvtRef->_typeValues);
				dlgSubRef->_events.Emplace(dlgEvtRef);
			}
		}

		// play conditions
		if (dlgSubJsonObj->TryGetArrayField("Condition", arrayValues)) {
			for (auto& playCondVal : *arrayValues) {
				FConditionRule::Parse(dlgSubRef->_playCondition, playCondVal->AsString());
			}
		}
	}

	PostParse();
}

void URefDialog::PostParse()
{
	CacheFlowControlID();
	PostParse_Event();
}

void URefDialog::PostParse_Event()
{
#define MAKE_POST_HANDLER(DialogEventType) \
	[](URefDialog* d, URefDialogSub* ds, URefDialogEvent* e){ URefDialog::PostParseEvent_##DialogEventType(d, ds, e); }

	static TMap<FName, TFunction<void(URefDialog*, URefDialogSub*, URefDialogEvent*)>> EvtPostHandlers{
		{URefDialogEvent::RewardSelectionEvent, MAKE_POST_HANDLER(RewardSelection)},
		{URefDialogEvent::ViewerJoinEvent, MAKE_POST_HANDLER(ViewerJoin)},
		{"Viewer_Join_Random", MAKE_POST_HANDLER(ViewerJoinRandom)},
		{"ChoiceNextDialog", MAKE_POST_HANDLER(ChoiceNextDialog)},
	};

	for (auto& speech : _speeches) {
		for (auto& evt : speech->_events) {
			if (auto postHandler = EvtPostHandlers.Find(evt->Type)) {
				(*postHandler)(this, speech, evt);
			}
		}
	}
}

void URefDialog::PostParseEvent_RewardSelection(URefDialog* dlg, URefDialogSub* speech, URefDialogEvent* evt)
{
	int32 dummyIdx = INDEX_NONE;
	TArray<int32> selectRwdIndice;
	URefBase::GetIntegerArray(evt->Type_Value, selectRwdIndice);
	checkf(speech->Choice_ID.Num() == selectRwdIndice.Num(), TEXT("[dlg] RewardSelection dialog event in Dialog[%s] bound with invalid Choice_Key; Type_Value count[%d] != Choice_Key count[%d]"), *dlg->UID.ToString(), selectRwdIndice.Num(), speech->Choice_ID.Num());
	for (int32 i = 0; i < selectRwdIndice.Num(); i++) {
		int32 rwdIndex = selectRwdIndice[i];
		int32 choiceID = speech->Choice_ID[i];
		URefDialogSub* choiceSpeech = dlg->GetSpeechByID(choiceID, dummyIdx);
		check(choiceSpeech);
		auto& eventArgs = dlg->EventArgs.FindOrAdd("RewardSelection");
		eventArgs.Emplace(*FString::FromInt(rwdIndex), choiceSpeech->Character_UID.ToString());
	}
}

void URefDialog::PostParseEvent_ViewerJoin(URefDialog* dlg, URefDialogSub* speech, URefDialogEvent* evt)
{
	auto& eventArgs = dlg->EventArgs.FindOrAdd("ViewerJoin");
	FName joinerUID{ *evt->Type_Value };
	int32 prevCount = 0;
	if (auto it = eventArgs.Find(joinerUID)) {
		prevCount = FCString::Atoi(**it);
	}
	eventArgs.Emplace(joinerUID, FString::FromInt(++prevCount));
}

void URefDialog::PostParseEvent_ViewerJoinRandom(URefDialog* dlg, URefDialogSub* speech, URefDialogEvent* evt)
{
	auto& eventArgs = dlg->EventArgs.FindOrAdd("ViewerRandomJoin");
	FName tids{ *evt->Type_Value };
	int32 prevCount = 0;
	if (auto it = eventArgs.Find(tids)) {
		prevCount = FCString::Atoi(**it);
	}
	eventArgs.Emplace(tids, FString::FromInt(++prevCount));
}

void URefDialog::PostParseEvent_ChoiceNextDialog(URefDialog* dlg, URefDialogSub* speech, URefDialogEvent* evt)
{
	checkf(evt->_typeValues.Num() == 4, TEXT("DialogEvent[%s].Event_Type[ChoiceNextDialog]: _typeValues count[%d] invalid: it must be-> dlg.uid.1 | count.1 | dlg.uid.2 | count.2"), *evt->UID.ToString(), evt->_typeValues.Num());
	speech->_choiceDlgs.Emplace(*evt->_typeValues[0]);
	speech->_choiceDlgs.Emplace(*evt->_typeValues[2]);

	checkf(speech->_choiceDlgs.Num() == 2, TEXT("[dlg] Dialog[%s] bound with invalid ChoiceNextDialog event; expected> dlg.uid.1 | count.1 | dlg.uid.2 | count.2"), *speech->GetDebugString());
}

void URefDialog::CacheFlowControlID()
{
	for (auto& speech : _speeches) {
		for (auto& choiceKey : speech->Choice_Key) {
			if (auto choiceTargetIt = _speeches.FindByPredicate([&choiceKey](URefDialogSub* dlgSub) { return dlgSub->Key == choiceKey; })) {
				speech->Choice_ID.Emplace((*choiceTargetIt)->ID);
			}
		}

		if (speech->Go_Key.IsNone() == false) {
			const FName& goKey{ speech->Go_Key };
			if (auto targetIt = _speeches.FindByPredicate([&goKey](URefDialogSub* dlgSub) {	return dlgSub->Key == goKey; })) {
				speech->Go_ID = (*targetIt)->ID;
			}
		}
	}
}

URefDialogSub* URefDialog::GetSpeechByID(int32 id, int32& optionIndex) const
{
	optionIndex = INDEX_NONE;
	for (int32 i = 0; i < _speeches.Num(); i++) {
		if (_speeches[i]->ID == id) {
			optionIndex = i;
			return _speeches[i];
		}
	}
	return nullptr;
}

bool URefDialogSub::IsChoicable() const
{
	return Choice_ID.Num() != 0 && _choiceDlgs.Num() == 0;
}

FString URefDialogSub::GetTextUID() const
{
	return FString::Printf(TEXT("str.%s.%d"), *Dlg_UID, StringTableID);
}

FName URefDialogSub::GetCamera(EGender gender) const
{
	int32 index = static_cast<int32>(gender);
	if (Camera.IsValidIndex(index)) {
		return Camera[index];
	}
	return Camera.Num() ? Camera[0] : NAME_None;
}

TAutoConsoleVariable<bool> CVar_AnuVoiceDebug(TEXT("Anu.Voice.Debug"), false, TEXT("voice debug mode; write log voice asset name when play dialog sub"));
void URefDialog::PrintVoiceLog(FString&& log)
{
	if (CVar_AnuVoiceDebug.GetValueOnGameThread()) {
		UE_LOG(LogTemp, Warning, TEXT("%s"), *log);
	}
}

FString URefDialog::VoiceAssetPath = "/Game/Anu/Sounds/Voice/Dlg_Voice";
FString URefDialog::BuildVoiceAssetPath(const FString& voiceLangCode)
{
	if (voiceLangCode == "ko") {
		return VoiceAssetPath + "_KO";
	}

	return VoiceAssetPath;
}

bool URefDialogSub::UseVoice(EGender gender, const FString& voiceLangCode)
{
	if (_cachedUseVoice.IsSet() == false) {
		_cachedUseVoice = FPackageName::DoesPackageExist(GetVoiceAssetPath(gender, voiceLangCode));
	}

	URefDialog::PrintVoiceLog(FString::Printf(TEXT("[voice] exist[%d], asset[%s]"), _cachedUseVoice.GetValue(), *GetVoiceAssetPath(gender, voiceLangCode)));

	return _cachedUseVoice.GetValue();
}

FString URefDialogSub::GetVoiceAssetName(EGender gender, const FString& voiceLangCode) const
{
	FString charStr = _isPCSpeech ? URefBase::GetShortenGenderString(gender) : _character->Name_Key;
	FString assetName{ FString::Printf(TEXT("voice_%s_%d_%s_%s"), *_dlg->UID.ToString(), ID, *charStr, *voiceLangCode) };
	return assetName.Replace(TEXT("."), TEXT("_"));
}

FString URefDialogSub::GetVoiceAssetPath(EGender gender, const FString& voiceLangCode) const
{
	FString assetName{GetVoiceAssetName(gender, voiceLangCode)};
	return FString::Printf(TEXT("%s/%s"), *URefDialog::BuildVoiceAssetPath(voiceLangCode), *assetName);
}

FString URefDialogSub::GetVoiceAssetObjectPath(EGender gender, const FString& voiceLangCode) const
{
	FString assetName{ GetVoiceAssetName(gender, voiceLangCode) };
	return FString::Printf(TEXT("%s/%s.%s"), *URefDialog::BuildVoiceAssetPath(voiceLangCode), *assetName, *assetName);
}

USoundWave* URefDialogSub::GetVoiceSound(UObject* outer, EGender gender, const FString& voiceLangCode) const
{	
	return LoadObject<USoundWave>(outer, *GetVoiceAssetObjectPath(gender, voiceLangCode));
}

FString URefDialogSub::GetDebugString() const
{
	return FString::Printf(TEXT("%s:%d"), *_dlg->UID.ToString(), ID);
}

TMap<FName, TSet<URefQuest*>> URefTitle::KEYWORDS_BY_CATEGORY;
void URefTitle::Parse(const FXmlNode* node)
{
	KEYWORDS_BY_CATEGORY.Empty();

	URefBase::Parse(node);

	// apply value	
	check(Value_Index.Num() == Value.Num());	
	for (int32 i = 0; i < Value_Index.Num(); ++i) {
		const FString& valueIndex = Value_Index[i];
		const FString& value = Value[i];
		if (valueIndex.Compare("None") == 0) {
			continue;
		}

		FStatInfoData statInfoData;
		if (URefBase::ParseStatInfo(valueIndex, statInfoData) == false) {			
			continue;
		}

		statInfoData.Value = FCString::Strtoi64(*value, NULL, 10);		

		auto& infoByApplyType = Stat.FindOrAdd(URefObject::GetStatEnum(statInfoData.StatString));
		infoByApplyType.Emplace(statInfoData.ApplyType, statInfoData);
	}

	// keyword
	VisitAttributes(node, "Keyword_", [this](const FXmlAttribute* attr) {
		FName keywordQuestUID{ *attr->GetValue() };
		if (keywordQuestUID != NAME_None) {
			Keywords.Emplace(keywordQuestUID);
		}
	});
}

void URefTitle::GetKeywordCategory(TArray<FName>& category)
{
	KEYWORDS_BY_CATEGORY.GetKeys(category);
}

void URefTitle::GetKeywordByCategory(FName category, TSet<URefQuest*>& keywords)
{
	auto it = KEYWORDS_BY_CATEGORY.Find(category);
	if (it == nullptr) {
		return;
	}
	
	keywords.Append(it->Array());
}

void URefTitle::GetStatInfoDatas(TArray<FStatInfoData>& output)
{
	for (auto& typeIter : Stat) {
		for (auto& it : typeIter.Value) {
			output.Emplace(it.Value);
		}
	}
}

FName URefStreaming::NAME_Streaming = "Streaming";
void URefStreaming::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	for (const FXmlAttribute& attr : node->GetAttributes()) {
		const FString& attrName = attr.GetTag();
		static FString OpenConditionString{ "OpenCondition_" };
		int32 index = attrName.Find(OpenConditionString);
		if (index == INDEX_NONE) {
			continue;
		}

		FString conditionValue{ attr.GetValue() };
		if (conditionValue.Compare("None") == 0) {
			continue;
		}

		FConditionRule::Parse(_openCondition, conditionValue);
	}
}

FName URefStreaming::GetConditionQuestUID() const
{
	return _listenQuests.Num() != 0 ? _listenQuests[0]->UID : NAME_None;
}

bool URefStreaming::UsePredict() const
{
	return Fail_Message.IsEmpty() == false;
}

bool URefStreaming::CheckShowScore() const
{
	return false;
}

FName URefInspecting::TypeName = "Inspecting";
void URefInspecting::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	Type = URefInspecting::TypeName;
}

FName URefTakePhoto::TypeName = "TakePhoto";
void URefTakePhoto::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	Type = URefTakePhoto::TypeName;
}

bool URefTakePhoto::CheckShowScore() const
{
	return (FCString::Atoi(*Type_Value) != 0);
}

FName URefInterview::TypeName = "Interview";
void URefInterview::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	Type = URefInterview::TypeName;
}

URefDialog* URefItemTrade::GetDialog(int32 tradeCount) const
{
	int32 index = FMath::Clamp(tradeCount, 0, _dlgs.Num() - 1);
	return _dlgs[index];
}

URefReward* URefItemTrade::GetReward(int32 tradeCount) const
{
	int32 index = FMath::Clamp(tradeCount, 0, _rewards.Num() - 1);
	return _rewards[index];
}

void URefRankingReward::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	FString value = node->GetAttribute(TEXT("Range_Value"));
	TArray<FString> strs;
	GetTrimedStringArray(value, strs);

	if (strs[0] == TEXT("inf")) {
		_rangeValues.Emplace(TNumericLimits<int32>::Lowest());
	}
	else {
		_rangeValues.Emplace(FCString::Atoi(*strs[0]));
	}

	if (strs[1] == TEXT("inf")) {
		_rangeValues.Emplace(TNumericLimits<int32>::Max());
	}
	else {
		_rangeValues.Emplace(FCString::Atoi(*strs[1]));
	}
}

TMap<FName, ERewardObjectType> URefReward::s_rewardObjTypes{
	{"Object", ERewardObjectType::Object},
	{"Currency", ERewardObjectType::Currency},
	{"Favor", ERewardObjectType::Favor},
	{"Friend", ERewardObjectType::Friend},
	{"Follow", ERewardObjectType::Follow},
	{"Popularity", ERewardObjectType::Popularity},
	{"UserInteraction", ERewardObjectType::UserInteraction},
	{"Title", ERewardObjectType::Title},
	{"ClassLicense", ERewardObjectType::ClassLicense},
	{"Permission", ERewardObjectType::Permission},
	{"Stat", ERewardObjectType::Stat},
	{"Palette", ERewardObjectType::Palette},
	{"Stress", ERewardObjectType::Stress},
	{"ChatSticker", ERewardObjectType::ChatSticker},
	{"SocialMotion", ERewardObjectType::SocialMotion},
	{"Tag", ERewardObjectType::Tag},
	{"QuestPoint", ERewardObjectType::QuestPoint},
};
FName URefReward::GetTypeStr(ERewardObjectType rwdObjType)
{
	for (auto& it : s_rewardObjTypes) {
		if (it.Value == rwdObjType) {
			return it.Key;
		}
	}
	return NAME_None;
}

ERewardObjectType URefReward::GetType(const FName& typeStr)
{
	auto it = s_rewardObjTypes.Find(typeStr);
	checkf(it, TEXT("not registered reward type[%s]"), *typeStr.ToString());
	return *it;
}

void URefReward::GetRewardInfo(int32 index, FRewardInfo& info)
{
	int32 arrayIndex = index;
	if (RewardType == ERewardType::Selectable) {
		for (int32 i = 0; i < RewardItems.Num(); i++) {
			URefRewardSelectable* selectable = Cast<URefRewardSelectable>(RewardItems[i].Reward);
			checkf(selectable, TEXT("reward[%s] contains non-selectable reward, but type is Selectable"), *UID.ToString());
			if (index == selectable->Select_Index) {
				info = RewardItems[i];
				return;
			}
		}
	}

	if (RewardItems.IsValidIndex(arrayIndex)) {
		info = RewardItems[arrayIndex];
	}
}

void URefReward::GetRewardByGroup(const FName& selectGroup, TArray<URefRewardBase*>& rewards)
{
	check(RewardType == ERewardType::Selectable);
	for (auto& it : RewardItems) {
		URefRewardSelectable* selectable = Cast<URefRewardSelectable>(it.Reward);
		checkf(selectable, TEXT("reward[%s] contains non-selectable reward, but type is Selectable"), *UID.ToString());
		if (selectable->Select_Group == selectGroup) {
			rewards.Emplace(selectable);
		}
	}
}

int32 URefReward::GetCurrencyAmount(URefCurrency* currency)
{
	int32 totalAmount = 0;
	Visit(ERewardObjectType::Currency, [&totalAmount, currency](URefRewardBase* rwd, int32 amount) {
		if (rwd->_currency == currency) {
			totalAmount += amount;
		}
	});
	return totalAmount;
}

URefRewardBase* URefReward::GetReward(ERewardObjectType type)
{
	for (auto& it : RewardItems) {
		if (it.Reward->_rewardObjType == type) {
			return it.Reward;
		}
	}
	return nullptr;
}

URefRewardBase* URefReward::GetRepresentative(ERewardObjectType type, FName tagUID) const
{
	if (tagUID.IsNone() == false) {		
		for (auto& it : RewardItems) {
			auto rwd = it.Reward;
			if (rwd->HasDisplayTag(tagUID) && type == rwd->_rewardObjType) {
				return rwd;
			}
		}
	}
	return RewardItems[0].Reward;
}

UTexture2D* URefReward::GetIconTexture(EGender gender, FName tagUID) const
{
	if (tagUID.IsNone() == false) {
		for (auto& it : RewardItems) {
			auto rwd = it.Reward;
			if (rwd->HasDisplayTag(tagUID)) {
				return rwd->GetIconTexture(gender);
			}
		}
	}
	return _displayInfo.GetIcon(gender);
}

void URefReward::GetRewardsByTag(FName displayTag, TArray<FRewardInfo>& output) const
{
	for (auto& it : RewardItems) {
		if (it.Reward->HasDisplayTag(displayTag)) {
			output.Emplace(it);
		}
	}
}

void URefReward::Visit(ERewardObjectType type, TFunction<void(URefRewardBase*, int32)>&& visitor)
{
	for (auto& it : RewardItems) {
		if (it.Reward->_rewardObjType == type) {
			visitor(it.Reward, it.Amount);
		}
	}
}

URefBase* URefRewardBase::GetRewardItemRef() const 
{
	if (_item) {
		return _item;
	}
	else if (_currency) {
		return _currency;
	}
	else if (_character) {
		return _character;
	}
	else if (_chatSticker) {
		return _chatSticker;
	}
	return nullptr;
}

bool URefRewardBase::HasDisplayTag(FName tagUID)
{
	return _displayTags.FindByPredicate([&tagUID](URefTag* tag) {
		return tagUID == tag->UID;
	}) != nullptr;
}

const FText& URefRewardBase::GetName(EGender gender) const
{
	if (_item) {
		return _item->GetName(gender);
	}
	else if (_character) {
		return _character->GetName(gender);
	}
	return _rewardObjectName;
}

const FText& URefRewardBase::GetDescription(EGender gender) const
{
	if (_item) {
		return _item->GetDescription(gender);
	}
	else if (_currency) {
		return _currency->Desc;
	}
	else if (_character) {
		return _character->GetDescription(gender);
	}
	else if (_title) {
		return _title->Desc;
	}
	return FText::GetEmpty();
}

UTexture2D* URefRewardBase::GetIconTexture(EGender gender) const
{
	if (_item) {
		return _item->GetIconTexture(gender);
	}
	else if (_currency) {
		return _currency->GetIcon();
	}
	return nullptr;
}

float URefRewardRandom::GetProbability() const
{
	return static_cast<float>(Prob) / static_cast<float>(_reward->_sumOfProbability);
}

void URefStageReward::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	ParseCondition(node);
}

const FConditionRule* URefStageReward::GetRule(const FName& ruleName) const
{
	return _condition.FindByPredicate([ruleName](const FConditionRule& condition) {
		return condition.Rule == ruleName;
	});
}

void URefStageReward::ParseCondition(const FXmlNode* node)
{
	TMap<int32, TPair<FString, FString>> condition;
	auto ParseConditionKey = [&condition](const FString& attrName)->TPair<FString, FString>& {
		TArray<FString> names;
		attrName.ParseIntoArray(names, TEXT("_"));
		return condition.FindOrAdd(FCString::Atoi(*(names[names.Num() - 1])));
	};

	for (const FXmlAttribute& attr : node->GetAttributes()) {
		const FString& attrName = attr.GetTag();
		if (attrName.Find("Condition_Value_") != INDEX_NONE) {
			ParseConditionKey(attrName).Value = attr.GetValue();
		}
		else if (attrName.Find("Condition_") != INDEX_NONE) {
			ParseConditionKey(attrName).Key = attr.GetValue();
		}
	}

	const FString delimiter{ TEXT(" | ") };
	for (auto& it : condition) {
		auto& pair = it.Value;
		FString conditionValue = pair.Key + delimiter + pair.Value;
		FConditionRule::Parse(_condition, conditionValue);
	}
}

void URefUserInteraction::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	Requester_Limit_Count = URefBase::ParseInteger(node->GetAttribute("Requester_Limit_Count"));
	Accepter_Limit_Count = URefBase::ParseInteger(node->GetAttribute("Accepter_Limit_Count"));
}

////////////////////////////////////////////////////
void URefSkillPatronageTier::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	bool b = TryParse(Target, _target);
	check(b);
	_uicolor = FLinearColor(FColor::FromHex(UI_Color.ToString()));
}

ESkillPatronageTarget URefSkillPatronageTier::GetTarget()
{
	return _target;
}

FLinearColor URefSkillPatronageTier::GetUIColor()
{
	return _uicolor;
}

////////////////////////////////////////////////////
bool URefFashionContentsScore::IsValidTag(URefTag* tag) const
{
	if (_tagGroup == nullptr) {
		return _tag == tag;
	}
	
	auto groupIt = _tagGroup->_tags.FindByPredicate([tag](auto it) {
		if (it != tag) { return false; }
		if (URefSchedule* schedule = it->_schedule; schedule && schedule->IsAvailable()) { return true; }
		return false;
	});
	return groupIt != nullptr;
}

void UMultiDonation::Init(const int32& index, TArray<FString>& output)
{
	Key = *FString::Printf(TEXT("multi.donation.%d"), index);
	Index = index;
	Score = FCString::Strtoi(*output[0], NULL, 10);
	Amount = FCString::Strtoi(*output[1], NULL, 10);
}

void URefFashionContentsGroup::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	VisitAttributes(node, "Donation", [this](const FXmlAttribute* attr) {
		TArray<FString> output;
		GetTrimedStringArray(attr->GetValue(), output);

		if (output.Num() == 2) {
			UMultiDonation* donation = NewObject<UMultiDonation>(this);
			donation->Init(Donations.Num(), output);
			Donations.Emplace(donation);
		}
	});
}

bool URefFashionContentsGroup::IsTutorial() const 
{
	return Type =="Show_Tutorial";
}

URefSchedule* URefFashionContentsGroup::GetSeasonSchedule() const
{
	return _refSchedule.Num() > 1 ? _refSchedule[1] : _refSchedule[0];
}

////////////////////////////////////////////////////
void URefFashionContentsStage::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	URefFashionContentsStage::ParseScore(node, "Add_Score_UID_", Add_Score_UID);
	URefFashionContentsStage::ParseScore(node, "Minus_Score_UID_", Minus_Score_UID);
}

void URefFashionContentsStage::ParseScore(const FXmlNode* node, const FString& columName, TArray<FName>& output)
{
	for (const FXmlAttribute& attr : node->GetAttributes()) {
		const FString& attrName = attr.GetTag();
		if (attrName.Find(columName) != INDEX_NONE) {
			FName value{ *attr.GetValue() };
			if (value.IsNone() == false) {
				output.Emplace(value);
			}
		}
	}
}

bool URefFashionContentsStage::IsTutorial() const
{
	return _refGroup->IsTutorial();
}

void URefFashionContentsNPC::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	URefFashionContentsStage::ParseScore(node, "Add_Score_UID_", Add_Score_UID);
	URefFashionContentsStage::ParseScore(node, "Minus_Score_UID_", Minus_Score_UID);

	URefFashionContentsStage::ParseScore(node, "ConditionValue", ConditionValue);
}

std::pair<uint16, float> URefFashionContentsNPC::GetScore(URefTag* tag) const
{
	constexpr uint16 NormalScore{ 3 };
	constexpr uint16 LikeScore{ 10 };
	constexpr uint16 HateScore{ 0 };

	for (auto& it : _likeTag) {
		if (it->IsValidTag(tag)) {
			float score = 0;
			for (auto& refScore : _addScores) {
				score += refScore->Add_Score;
			}
			return std::make_pair(LikeScore, score * 0.01f);
		}
	}

	for (auto& it : _hateTag) {
		if (it->IsValidTag(tag)) {
			float score = 0;
			for (auto& refScore : _minusScores) {
				score += refScore->Minus_Score;
			}
			return std::make_pair(HateScore, score * -0.01f);
		}
	}
	return std::make_pair(NormalScore, static_cast<float>(ScoreValue) * 0.1f);
}

void FEquipAttributeInfo::Parse(const FXmlNode* node)
{
	Apply_Type = *(node->GetAttribute("Apply_Type"));
	Apply_UID = *(node->GetAttribute("Apply_UID"));
	Apply_Effect = *(node->GetAttribute("Apply_Effect"));
	{
		const FString& ValueType{ node->GetAttribute("Value_Type") };
		const StatApplyType& applyType{ SkillUtil::GetApplyTypeByString(TCHAR_TO_UTF8(*ValueType)) };
		Value_Type = static_cast<EStatApplyType>(applyType);
	}

	FString value{ node->GetAttribute("Value") };
	if (value.IsEmpty() == false) {
		Value = FCString::Atoi(*value);
	}
}

const FName& URefEquipAttribute::GetApplyEffect() const
{
	return Attribute.Apply_Effect;
}

void URefEquipAttribute::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	Attribute.Parse(node);
	Attribute.Value = MaxValue;
}

void URefEquipSetEffect::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	Attribute.Parse(node);
}

void URefEquipUpgradeEffect::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	for (const FXmlAttribute& attr : node->GetAttributes()) {
		const FString& value = attr.GetTag();
		int32 found = value.Find("Stat_");
		if (found == INDEX_NONE) {
			continue;
		}

		FStatInfoData statInfo;
		if (URefBase::ParseStatInfo(value.RightChop(found + 5), statInfo) == false) {
			continue;
		}

		statInfo.Value = FCString::Strtoi(*(attr.GetValue()), NULL, 10);
		if(statInfo.Value > 0){
			_stats.Emplace(MoveTemp(statInfo));
		}
	}
}

int64 URefEquipUpgradeEffect::GetStatValue(EStatTypes statType) const
{
	auto it = _stats.FindByPredicate([statType](const FStatInfoData& statInfo) {
		return statInfo.StatType == statType;
	});
	return it ? it->Value : 0;
}

void URefEquipCarveEffect::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	VisitAttributes(node, "Target_TID", [this](const FXmlAttribute* attr) {
		TArray<FString> strValues;
		URefBase::GetTrimedStringArray(attr->GetValue(), strValues);
		if (strValues.Num() != 0 && FName(strValues[0]).IsNone()) {
			return;
		}
		Target_TID.Emplace(MoveTemp(strValues));
	});
}

void URefStreamingNPC::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	for (const FXmlAttribute& attr : node->GetAttributes()) {
		const FString& attrName = attr.GetTag();
		static FString OpenConditionString{ "Condition" };
		int32 index = attrName.Find(OpenConditionString);
		if (index == INDEX_NONE) {
			continue;
		}

		FString conditionValue{ attr.GetValue() };
		if (conditionValue.Compare("None") == 0) {
			continue;
		}

		FConditionRule::Parse(_condition, conditionValue);
	}
}

FEquipCollectionStatReward FEquipCollectionStatReward::Parse(const FString& value)
{
	TArray<FString> output;
	URefBase::GetTrimedStringArray(value, output);

	int32 count = FCString::Atoi(*output[0]);
	return { static_cast<uint8>(count), URefObject::GetStatEnum(*output[1]), FCString::Atoi(*output[2]) };
}

void URefEquipCollection::GetCollectionsByItem(URefItem* item, TSet<URefEquipCollection*>& output)
{
	if (auto it = URefEquipCollection::ByEquipGuids.Find(item ? item->GUID : 0)) {
		output.Append(*it);
	}
}

void URefEquipCollectionGroup::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	VisitAttributes(node, "Reward_Stat_", [this](const FXmlAttribute* attr){
		auto value = attr->GetValue();
		if (value != "None") {
			auto reward = FEquipCollectionStatReward::Parse(value);
			Reward_Stat.Emplace(MoveTemp(reward));
		}
	});

	Reward_Stat.Sort([](const FEquipCollectionStatReward& lhs, const FEquipCollectionStatReward& rhs){
		return lhs.Count < rhs.Count;
	});
}

void URefStat::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	Type = URefObject::GetStatEnum(UID);
}

bool URefStat::HasTarget(const FName& target) const
{
	for (const FName& one : SpecificTargets) {
		if (one.Compare(target) == 0) {
			return true;
		}
	}
	return false;
}

void URefEmblemEffect::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	ParseTag(node, "Tag", _tagWithValues);
	ParseStatInfo(node, "Emblem_Effect", _stats);
}

void URefRanking::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	auto key = TPairInitializer(RankType, PeriodicType);
	_values.FindOrAdd(key) = this;
}

FString URefEmblemEffect::GetDebugString() const
{
	return FString::Printf(TEXT("%s:%d"), *Emblem_UID.ToString(), Upgrade_Value);
}

void URefArbeitReward::Parse(const FXmlNode* node)
{
	Super::Parse(node);

	VisitAttributes(node, "Schedule_Reward", [this](const FXmlAttribute* attr) {
		ScheduleRewards.Emplace(attr->GetValue());
	});
}

void URefSkillTree::Parse(const FXmlNode* node)
{
	Super::Parse(node);
}

void URefSkillTreeStep::Parse(const FXmlNode* node)
{
	Super::Parse(node);
}

void URefSkillTreeSlot::Parse(const FXmlNode* node)
{
	Super::Parse(node);
}

bool URefSkillTreeSlot::IsMaxLevel(int32 level)
{
	return _levels.Num() <= level;
}

URefSkillTreeLevel* URefSkillTreeSlot::GetLevelReference(int32 level)
{
	return _levels.Num() > level ? _levels[level] : nullptr;
}

void URefSkillTreeLevel::Parse(const FXmlNode* node)
{
	Super::Parse(node);
}