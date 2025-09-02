// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.


#include "Reference_Skill.h"
#include "Engine/DataTable.h"
#include "Reference_Resource.h"
#include "Reference_Interactor.h"

////////////////////////////////////////////////////////////////////////////////////
FRefSkillObjectDataBase::FRefSkillObjectDataBase() : Super()
{
//#if !UE_BUILD_SHIPPING
//	if (Bone_Effect.DataTable == nullptr) {
//		Bone_Effect.DataTable = LoadObject<UDataTable>(nullptr, *FAnuResourceParticle::TablePath);
//		Hit_Effect.DataTable = Bone_Effect.DataTable;
//	}
//#endif
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
TMap<SkillEffect, URefSkillEffect*> URefSkillEffect::effects;
TMap<FName, URefSkillEffect*> URefSkillEffect::effectsByName;
void URefSkillEffect::Parse(const FXmlNode* node)
{
	Super::Parse(node);
	EffectName = *node->GetAttribute("Effect");
	FString strEft{ EffectName.ToString() };
	Effect = SkillUtil::GetEffectByString(TCHAR_TO_UTF8(*strEft));

	FString functionName = *node->GetAttribute("Function");
	FunctionEffect = SkillUtil::GetEffectByString(TCHAR_TO_UTF8(*functionName));

	Group = SkillUtil::GetEffectGroupByString(TCHAR_TO_UTF8(*node->GetAttribute("Group")));

	HudGroundColor = FColor::FromHex(node->GetAttribute("GroundColor"));
	checkf(effectsByName.Find(EffectName) == nullptr, TEXT("duplicate effect name![%s]"), *strEft);
	effectsByName.Emplace(EffectName, this);
	effects.Emplace(Effect, this);
	ApplyType = SkillUtil::GetApplyTypeByString(TCHAR_TO_UTF8(*strEft));

	TArray<FString> outArray;
	GetTrimedStringArray(node->GetAttribute("Immun_Group"), outArray);
	for (auto& value : outArray) {
		SkillEffect target = SkillUtil::GetEffectByString(TCHAR_TO_UTF8(*value));
		if (target != SkillEffect::None) {
			Immun_Group.Emplace(target);
		}
	}

	GetTrimedStringArray(node->GetAttribute("Immun_Grade"), outArray);
	static const UEnum* GradeEnumType = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECharacterGrade"), true);
	for (auto& value : outArray) {
		ECharacterGrade grade = (ECharacterGrade)(GradeEnumType->GetIndexByName(*value));
		if (grade != ECharacterGrade::None) {
			Immun_Grade.Emplace(grade);
		}
	}

	FString toastMsgID = node->GetAttribute("ToastMsg");
	if (toastMsgID != "None") {
		_toastMsg = AnuText::Get_CommonTable(toastMsgID);
	}
}

////////////////////////////////////////////////////////////////////////////////////
void URefSkillTimeline::Parse(UScriptStruct* type, FTableRowBase* row)
{
	Super::Parse(type, row);

	auto dataBase = static_cast<FRefSkillTimelineDataBase*>(row);
	HitParticleRowName = dataBase->HitParticleRowHandle.RowName;
}

float URefSkillTimeline::GetSearchRange()
{
	switch (SearchRule) {
		case ESearchRule::InRange:
			return SearchRuleParam[0];
		case ESearchRule::InSquare:
			return SearchRuleParam[1];
		case ESearchRule::InAngle:
			return SearchRuleParam[2];
		default:
			return 0;
	}
}

SkillEffect URefSkillTimeline::GetType() const
{
	return _refEffect != nullptr ? _refEffect->Effect : SkillEffect::None;
}

bool URefSkillTimeline::IsServerEffect() const
{
	if (_refSkill == nullptr) {
		return false;
	}

	if (_value && _value->IsServerEffect()) {
		return true;
	}

	return false;
}

bool URefSkillTimeline::IsBasicAttack()
{
	auto skill = _refSkill;
	if (skill == nullptr) {
		return false;
	}

	if (skill->InputType != ESkillInputType::Basic && skill->InputType != ESkillInputType::None) {
		return false;
	}

	if (GetType() != SkillEffect::Damage) {
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
ETargetType URefSkill::GetTargetTypeByString(const FString& temp)
{
	static std::map<FString, ETargetType> TARGET_MAPPING = {
		{"None", ETargetType::None},
		{"Self", ETargetType::Self},
		{"Enemy", ETargetType::Enemy},
		{"Alliance", ETargetType::Alliance},
		{"Any", ETargetType::Any},
	};
	return TARGET_MAPPING[temp];
}

const float URefSkill::GetRange() const
{
	return Range;
}

const FSkillAnimation* URefSkill::GetAnimation(EGender gender) const
{
	uint8 genderIdx = FMath::Clamp(static_cast<uint8>(gender), (uint8)0, (uint8)(SkillAnimation.Num() - 1));
	return SkillAnimation.IsValidIndex(genderIdx) ?  &SkillAnimation[genderIdx] : nullptr;
}

void URefSkill::GetTimelines(TArray<URefSkillTimeline*>& out, uint8 tier)
{
	if (auto timelines = _refTimelines.Find(tier)) {
		out = timelines->_timelines;
	}
}

void URefSkill::AddTimeline(URefSkillTimeline* reference)
{
	auto& data = _refTimelines.FindOrAdd(reference->Tier);
	data._timelines.Emplace(reference);
}

void URefSkill::RemoveTimeline(URefSkillTimeline* reference)
{
	auto& data = _refTimelines.FindOrAdd(reference->Tier);
	data._timelines.Remove(reference);
}

void URefSkill::ClearTimelines()
{
	_refTimelines.Empty();
}

void URefSkill::Init()
{
	for (auto& itr : _refTimelines) {
		itr.Value.Init();
	}
}

uint32 URefSkill::GetPropertyValue(const FName& propName)
{
	if (FProperty* prob = GetClass()->FindPropertyByName(propName))	{
		void* memberProp = prob->ContainerPtrToValuePtr<void>(this);
		if (auto numProb = CastField<FNumericProperty>(prob)) {
			return static_cast<uint32>(numProb->GetSignedIntPropertyValue(memberProp));
		}
	}
	return 0;
}

URefSkillTimeline* URefSkill::GetTimelineByType(SkillEffect type)
{
	auto& data = _refTimelines.FindOrAdd(CLASS_SKILL_DEFAULT_TIER);
	URefSkillTimeline** refEffect = data._timelines.FindByPredicate([type](URefSkillTimeline* one) {
		return one->GetType() == type;
	});

	return refEffect ? *refEffect : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////
void URefSkillObject::Parse(UScriptStruct* type, FTableRowBase* row)
{
	Super::Parse(type, row);
	FRefSkillObjectDataBase* dataBase = static_cast<FRefSkillObjectDataBase*>(row);
	// object common
	typeNames.Empty();
	typeNames.Emplace(dataBase->TID_1);
	typeNames.Emplace(dataBase->TID_2);
	typeNames.Emplace(dataBase->TID_3);
	typeNames.Emplace(dataBase->TID_4);
	// skill object specified
	Bone_Fx = dataBase->Bone_Effect.RowName;
	Hit_Fx = dataBase->Hit_Effect.RowName;
	Hit_Fx_Type = dataBase->Hit_Effect_Type;
	SphereColliderRadius = dataBase->SphereColliderRadius;
}

////////////////////////////////////////////////////////////////////////////////////
void FSkillTimelineData::Init()
{
	_timelines.Sort([](URefSkillTimeline& lhs, URefSkillTimeline& rhs) {
		return lhs.Timeline <= rhs.Timeline;
	});
}
