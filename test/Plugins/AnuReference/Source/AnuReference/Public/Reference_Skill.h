// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Reference.h"
#include "Camera/CameraShakeBase.h"
#include "Curves/CurveVector.h"
#include "Reference_Skill.generated.h"

const FName BB_KEY_TARGET_GID = "TargetGID";
const FName BB_KEY_SKILL_GUID = "SkillGUID";
const FName BB_KEY_PREV_SKILL_UID = "PrevSkillUID";
const FName BB_KEY_ELAPSED_SECONDS = "ElapsedSeconds";
const FName BB_KEY_TIMER_INDEX = "TimerIndex";
const FName BB_KEY_DESTINATION = "Destination";
const FName BB_KEY_DESTINATION_UPDATED = "DestinationUpdated";
const FName BB_KEY_PURSUIT_TIMER = "PursuitAnimTimer";
const FName BB_KEY_KNOCKDOWN_TIMER = "KnockDownAnimTimer";
const FName BB_KEY_HIT_DELAY = "HitDelay";
const FName BB_KEY_STATE = "State";
const FName BB_KEY_SQUAD_SECTOR = "SquadSector";
const FName BB_KEY_PHASE = "Phase";

//뭔가 최초 한번을 하고싶은데ㅜㅜ
const FName BB_KEY_IS_SPAWNED = "IsSpawned";

const FString DATATABLE_SKILL_DIR = "/Game/Anu/DataTable/References/Skill/";


#pragma region Skill
enum class SkillEffect : uint8;

UENUM(BlueprintType)
enum class ESkillInputType : uint8 {
	None, Basic, Active, Charge, Passive, ClassChange, DashAttack,
};

UENUM(BlueprintType)
enum class ESkillTriggerType : uint8 {
	None, Spawned, Dead, CollisionEntered, CollisionExited, ChargingStarted, ChargingFailed, Village, NoramlStage, Challenge,

	// Test
	DashAttack
};

UENUM(BlueprintType)
enum class ERelationType : uint8 {
	None, Enemy, Friend,
};

UENUM(BlueprintType)
enum class ETargetType : uint8
{
	None, Self, Enemy, Alliance, Any
};

UENUM(BlueprintType)
enum class EFireRule : uint8
{
	None, NoneTarget, Directional, TargetInRange, Locational
};

UENUM(BlueprintType)
enum class EHitEffectType : uint8 {
	Default, Ground
};

USTRUCT(BlueprintType)
struct FSkillGroup
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName SkillGroupKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName SkillGroupValue;
};

USTRUCT(BlueprintType)
struct FSkillAnimation
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<class UAnimMontage> Montage;
	UPROPERTY(EditAnywhere)
	FName Montage_Section;
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "LevelSequence"))
	TSoftObjectPtr<UObject> Sequence;
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "LevelSequence"))
	bool PerformAbort;
};

#pragma region skilleffectbase
USTRUCT(BlueprintType)
struct FRefSkillDataBase : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FName UID;
	UPROPERTY(EditAnywhere)
	FSkillGroup Skill_Group;
	UPROPERTY(EditAnywhere)
	ESkillInputType InputType;
	UPROPERTY(EditAnywhere)
	int32 SlotOrder;
	UPROPERTY(EditAnywhere)
	ESkillTriggerType TriggerType;
	UPROPERTY(EditAnywhere)
	ETargetType Target;
	UPROPERTY(EditAnywhere)
	EFireRule FireRule;
	UPROPERTY(EditAnywhere)
	int32 Range;
	UPROPERTY(EditAnywhere)
	int32 CoolTime;
	UPROPERTY(EditAnywhere)
	int32 ActionTime;
	UPROPERTY(EditAnywhere)
	int32 AfterDelay;
	UPROPERTY(EditAnywhere)
	int32 AnimLoopTime;
	UPROPERTY(EditAnywhere)
	int32 SequenceWaitTime;

	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
	TArray<FSkillAnimation> SkillAnimation;
	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
	FText Name;
	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
	FText Desc;
	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
	FText Desc2;
	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
	TSoftObjectPtr<UTexture2D> Asset_Icon;
	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
	int32 SkillDisplayGrade;
	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
	FName Display_Tag;
};

UENUM(BlueprintType)
enum class EIndicatorRule : uint8 {
	None, InAngle, InRange, InSquare
};

UENUM(BlueprintType)
enum class ESearchRule : uint8 {
	None, Self, InAngle, InRange, InSquare, Hit, SearchedTargets
};

UENUM(BlueprintType)
enum class ELocationRule : uint8 {
	None, StartPoint, TargetPoint, CasterActorLocation, TargetActorLocation
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectBase : public UObject
{
	GENERATED_BODY()
public:
	virtual bool IsServerEffect() { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectTimeScale : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	float timeScale;

	virtual bool IsServerEffect() override { return true; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectDamage : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	StatValue value = 0;	// 데미지
	StatValue poise = 0;	// 강인도 감쇄력, 대상의 PoiseHealth스텟보다 클 경우 '강' 0에 가까워 질수록 '약'에 해당하는 경직 적용
	TOptional<StatValue> extraBossDamage;
	TOptional<float> pireceRate;		// 0.0 ~ 1.0

	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectAirborne : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	float speed;
	UPROPERTY()
	float verticalSpeed;

	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectPull : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	float distance;
	UPROPERTY()
	float speed;

	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectPush : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	float distance;
	UPROPERTY()
	float speed;

	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectMove : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	uint8 direction;
	UPROPERTY()
	float distance;
	UPROPERTY()
	float speed;
	UPROPERTY()
	float radius;

	virtual bool IsServerEffect() override { return true; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectBump : public URefSkillEffectMove
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool anyTarget;
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectWarp : public URefSkillEffectMove
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString effectType = "World";
	UPROPERTY()
	float x;
	UPROPERTY()
	float y;
	UPROPERTY()
	float yaw;

	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectChangeState : public URefSkillEffectBase
{
	GENERATED_BODY()

public:
	TMap<SkillUtil::EffectState, uint8> states;

	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectAttribute : public URefSkillEffectBase
{
	GENERATED_BODY()

public:
	EStatTypes statType;
	StatValue statValue;
	StatApplyType applyType;

	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectMakeObject : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FName skillObjectUID;
	UPROPERTY()
	FVector locationOffset;

	// runtime data
	UPROPERTY()
	URefSkillObject* refSkillObject;

	virtual bool IsServerEffect() override { return true; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectLockOn : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	int32 rotationSpeed;

	virtual bool IsServerEffect() override { return true; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectSpawnParticle : public URefSkillEffectBase
{
	GENERATED_BODY()

public:
	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectDispel : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	DispelMatchType matchType = DispelMatchType::Group;
	SkillEffectGroup targetSkillGroup = SkillEffectGroup::None;
	uint32 targetSkillGUID = 0U;

	virtual bool IsServerEffect() override { return true; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectCoolDown : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	FString effectType = "Per";
	float value = 0;

	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectScore : public URefSkillEffectBase
{
	GENERATED_BODY()
public:
	class URefTag* TagType = nullptr;
	uint32 value = 0;
	virtual bool IsServerEffect() override { return true; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectCounterSkill : public URefSkillEffectAttribute
{
	GENERATED_BODY()
public:
	UPROPERTY()
	URefSkill* successSkill;
	UPROPERTY()
	URefSkill* failureSkill;

	virtual bool IsServerEffect() override { return false; }
};

UCLASS()
class ANUREFERENCE_API URefSkillEffectImmune : public URefSkillEffectAttribute
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<URefSkillTimeline*> targets;

	virtual bool IsServerEffect() override { return true; }
};

UCLASS()
class ANUREFERENCE_API USkillObjParameters : public UObject
{
	GENERATED_BODY()
public:
	virtual void Parse(TArray<FString>& values) {}
};

UCLASS()
class ANUREFERENCE_API USkillObjParameters_Projectile : public USkillObjParameters
{
	GENERATED_BODY()
public:
	UPROPERTY()
	float speed;
	UPROPERTY()
	int32 pierce;

public:
	virtual void Parse(TArray<FString>& values) override
	{
		if (values.Num() > 0) {
			speed = FCString::Atof(*values[0]);
		}
		if (values.Num() > 1) {
			pierce = FCString::Atoi(*values[1]);
		}
	}
};

UCLASS()
class ANUREFERENCE_API USkillObjParameters_Orb : public USkillObjParameters
{
	GENERATED_BODY()
public:
	UPROPERTY()
	float speed;

public:
	virtual void Parse(TArray<FString>& values) override
	{
		if (values.Num() > 0) {
			speed = FCString::Atof(*values[0]);
		}
	}
};

UCLASS()
class ANUREFERENCE_API URefSkillObject : public URefObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
		TArray<FString> Values;
	UPROPERTY()
		int32 LifeTime = 0;
	UPROPERTY()
		float SphereColliderRadius = 0.0f;
	UPROPERTY()
		FName Bone_Fx;
	UPROPERTY()
		FName Hit_Fx;
	UPROPERTY()
		EHitEffectType Hit_Fx_Type;
	UPROPERTY()
		FName Skill_UID;
	UPROPERTY()
		float DestinationRandomMin;
	UPROPERTY()
		float DestinationRandomMax;
	UPROPERTY()
		FName Brain;

	virtual void Parse(UScriptStruct* type, FTableRowBase* row) override;

public: //runtime
	UPROPERTY()
		URefSkill* refSkill = nullptr;
	UPROPERTY()
		USkillObjParameters* parameters = nullptr;
	UPROPERTY()
		TArray<class URefSkill*> _builtInSkills;
	UPROPERTY()
		TArray<URefStat*> _extraDuration;
};
#pragma endregion skilleffectbase

enum class ECharacterGrade : uint8;

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkillEffect : public URefBase
{
	GENERATED_BODY()
public:
	SkillEffect Effect = SkillEffect::None;
	SkillEffect FunctionEffect = SkillEffect::None;;
	SkillEffectGroup Group = SkillEffectGroup::None;
	StatApplyType ApplyType = StatApplyType::Invalid;

	TSet<SkillEffect> Immun_Group;
	TArray<ECharacterGrade> Immun_Grade;

	UPROPERTY()
		FName Icon;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FName OpenWidget;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FName EffectName;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FText Name;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		int32 Priority;

public:
	virtual void Parse(const FXmlNode* node) override;
	static TMap<SkillEffect, URefSkillEffect*> effects;
	static TMap<FName, URefSkillEffect*> effectsByName;

public: // runtime
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FColor HudGroundColor;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	TArray<TSoftObjectPtr<UTexture2D>> Asset_Icon;

	FText _toastMsg;

	UFUNCTION(BlueprintPure)
	UTexture2D* GetAssetIcon()
	{
		if (Asset_Icon.Num() == 0) {
			return nullptr;
		}

		// Use primary only.
		if (UTexture2D* texture = Asset_Icon[0].Get()) {
			return texture;
		}
		else {
			return Asset_Icon[0].LoadSynchronous();
		}
	}

	UFUNCTION(BlueprintPure)
	UTexture2D* GetAssetIconWithEvaluatedValue(int32 evaluatedLevel)
	{
		if (Asset_Icon.Num() <= evaluatedLevel) {
			return nullptr;
		}

		if (UTexture2D* texture = Asset_Icon[evaluatedLevel].Get()) {
			return texture;
		}
		else {
			return Asset_Icon[evaluatedLevel].LoadSynchronous();
		}
	}
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkillTimeline : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Effect;
	UPROPERTY()
		TArray<FString> Value;
	UPROPERTY()
		uint8 Tier;
	UPROPERTY()
		ETargetType Target;
	UPROPERTY()
		bool Overlap;
	UPROPERTY()
		float DelayTime;
	UPROPERTY()
		float Timeline;
	UPROPERTY()
		float Interval;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool DisplayAttackArea;
	UPROPERTY()
		float DisplayAttackAreaDuration;
	UPROPERTY(EditAnywhere)
		EIndicatorRule IndicatorRule;
	UPROPERTY(EditAnywhere)
		TArray<int32> IndicatorRuleParam;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool DisplayStatusEffect;
	UPROPERTY()
		float Duration_Instant;
	UPROPERTY()
		float Duration_Absolute;
	UPROPERTY()
		int32 MinTargetCnt;
	UPROPERTY()
		int32 MaxTargetCnt;
	UPROPERTY()
		float RotationYaw;
	UPROPERTY()
		ELocationRule LocationRule;
	UPROPERTY()
		FVector LocationRuleOffset;
	UPROPERTY()
		float MinRandomPoint;
	UPROPERTY()
		float MaxRandomPoint;
	UPROPERTY(EditAnywhere)
		int32 RepeatCount;
	UPROPERTY(EditAnywhere)
		float RepeatInterval;
	UPROPERTY()
		ESearchRule SearchRule;
	UPROPERTY()
		TArray<int32> SearchRuleParam;
	UPROPERTY()
		TSoftObjectPtr<USoundBase> HitSound;
	UPROPERTY()
		TSubclassOf<UCameraShakeBase> HitCameraShake;
	UPROPERTY()
		TSoftObjectPtr<UCurveVector> HitDampCurve;
	UPROPERTY()
		FName HitParticleRowName;
	UPROPERTY()
		TSoftObjectPtr<class UParticleSystem> HitParticle;
	UPROPERTY()
		TMap<EAnuElementType, TSoftObjectPtr<UParticleSystem>> HitParticleElements;

public:
	virtual void Parse(UScriptStruct* type, FTableRowBase* row) override;

	float GetSearchRange();
	SkillEffect GetType() const;
	bool IsServerEffect() const;
	bool IsBasicAttack();

public://runtime
	UPROPERTY(BlueprintReadOnly)
		class URefSkill* _refSkill;
	UPROPERTY(BlueprintReadOnly)
		URefSkillEffect* _refEffect;
	UPROPERTY()
		URefSkillEffectBase* _value;
	UPROPERTY()
		bool _ignoreImmun;
	UPROPERTY()
		TArray<URefStat*> _extraPower;
	UPROPERTY()
		TArray<URefStat*> _extraBossDamage;
	UPROPERTY()
		TArray<URefStat*> _extraRange;
	UPROPERTY()
		TArray<URefStat*> _extraDuration;
};

USTRUCT(BlueprintType)
struct FRefSkillTimelineDataBase : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
		FName UID;
	UPROPERTY(EditAnywhere)
		FName Skill_UID;
	UPROPERTY(EditAnywhere)
		ETargetType Target;
	UPROPERTY(EditAnywhere)
		float RotationYaw;
	UPROPERTY(EditAnywhere)
		ELocationRule LocationRule;
	UPROPERTY(EditAnywhere)
		FVector LocationRuleOffset;
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Min"))
		float MinRandomPoint;
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Max"))
		float MaxRandomPoint;

	UPROPERTY(EditAnywhere)
		ESearchRule SearchRule;
	UPROPERTY(EditAnywhere)
		TArray<int32> SearchRuleParam;
	UPROPERTY(EditAnywhere)
		int32 MinTargetCnt;
	UPROPERTY(EditAnywhere)
		int32 MaxTargetCnt;

	UPROPERTY(EditAnywhere)
		float Timeline;
	UPROPERTY(EditAnywhere)
		FName Effect;
	UPROPERTY(EditAnywhere)
		bool Overlap = true;
	UPROPERTY(EditAnywhere)
		TArray<FString> Value;
	UPROPERTY(EditAnywhere)
		float Duration_Instant;
	UPROPERTY(EditAnywhere)
		float Duration_Absolute;
	UPROPERTY(EditAnywhere)
		float Interval;
	UPROPERTY(EditAnywhere)
		int32 RepeatCount;
	UPROPERTY(EditAnywhere)
		float RepeatInterval;
	UPROPERTY(EditAnywhere)
		bool DisplayAttackArea;
	UPROPERTY(EditAnywhere)
		float DisplayAttackAreaDuration = 2.0f;
	UPROPERTY(EditAnywhere)
		EIndicatorRule IndicatorRule;
	UPROPERTY(EditAnywhere)
		TArray<int32> IndicatorRuleParam;

	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
		bool DisplayStatusEffect;

	UPROPERTY(EditAnywhere, Category = "Resources", meta = (MetaTag1 = "ClientOnly"))
		TSoftObjectPtr<USoundBase> HitSound;
	UPROPERTY(EditAnywhere, Category = "Resources", meta = (MetaTag1 = "ClientOnly"))
		TSubclassOf<UCameraShakeBase> HitCameraShake;
	UPROPERTY(EditAnywhere, Category = "Resources", meta = (MetaTag1 = "ClientOnly"))
		TSoftObjectPtr<UCurveVector> HitDampCurve;
	UPROPERTY(EditAnywhere, Category = "Resources", meta = (MetaTag1 = "ClientOnly"))
		FDataTableRowHandle HitParticleRowHandle;
	UPROPERTY(EditAnywhere, Category = "Resources", meta = (MetaTag1 = "ClientOnly"))
		TSoftObjectPtr<UParticleSystem> HitParticle;
	UPROPERTY(EditAnywhere, Category = "Resources", meta = (MetaTag1 = "ClientOnly"))
		TMap<EAnuElementType, TSoftObjectPtr<UParticleSystem>> HitParticleElements;
};

USTRUCT(BlueprintType)
struct FRefSkillObjectDataBase : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	FRefSkillObjectDataBase();

public:
	UPROPERTY(EditAnywhere)
		FName UID;
	UPROPERTY(VisibleAnywhere)
		FName TID_1{"SkillObject"};
	UPROPERTY(EditAnywhere)
		FName TID_2;
	UPROPERTY(EditAnywhere)
		FName TID_3;
	UPROPERTY(EditAnywhere)
		FName TID_4;
	UPROPERTY(EditAnywhere)
		TArray<FName> Model;
	UPROPERTY(EditAnywhere)
		TArray<FString> Values;
	UPROPERTY(EditAnywhere)
		FName Skill_UID;
	UPROPERTY(EditAnywhere)
		FName Brain;
	UPROPERTY(EditAnywhere)
		int32 LifeTime = 3000;
	UPROPERTY(EditAnywhere)
		float DestinationRandomMin;
	UPROPERTY(EditAnywhere)
		float DestinationRandomMax;
	UPROPERTY(EditAnywhere)
		float SphereColliderRadius;
	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
		FDataTableRowHandle Bone_Effect;
	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
		FDataTableRowHandle Hit_Effect;
	UPROPERTY(EditAnywhere, meta = (MetaTag1 = "ClientOnly"))
		EHitEffectType Hit_Effect_Type = EHitEffectType::Default;
};

USTRUCT()
struct FSkillTimelineData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
		TArray<URefSkillTimeline*> _timelines;

public:
	void Init();
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkill : public URefBase
{
	GENERATED_BODY()

public:
	static ETargetType GetTargetTypeByString(const FString& temp);

	// inputType == type
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		ESkillInputType InputType = ESkillInputType::None;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		ESkillTriggerType TriggerType = ESkillTriggerType::None;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FText Name;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FText Desc;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FText Desc2;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		int32 SkillDisplayGrade = 0;
	UPROPERTY()
		int32 SlotOrder;
	UPROPERTY()
		EFireRule FireRule;
	UPROPERTY()
		int32 Range;
	UPROPERTY()
		int32 CoolTime;
	UPROPERTY()
		int32 ActionTime;
	UPROPERTY(EditAnywhere)
		ETargetType Target;
	UPROPERTY()
		int32 AfterDelay;
	UPROPERTY()
		int32 AnimLoopTime;
	UPROPERTY()
		int32 SequenceWaitTime;
	UPROPERTY()
		FName Display_Tag;
	UPROPERTY()
		TArray<FSkillAnimation> SkillAnimation;

public:
	//runtime data
	UPROPERTY()
		FSkillGroup Skill_Group;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		int32 Duration = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		TSoftObjectPtr<UTexture2D> Asset_Icon;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference|Skill")
		URefClass* _refClass;
	UPROPERTY()
		URefSkillTreeSlot* SkillTreeSlot;
	UPROPERTY()
		URefSkill* BaseSkill;
	UPROPERTY()
		TArray<URefSkill*> ComboSkills;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		TArray<FText> _displayTags;
	UPROPERTY()
		TArray<URefStat*> _extraRange;
	UPROPERTY()
		TMap<int32, FSkillTimelineData> _refTimelines;
	UPROPERTY()
		int32 DamageCount;
	UPROPERTY()
		TSet<URefStat*> StatEffects;

	ESearchRule _timelineSearchRule = ESearchRule::None;

public:
	virtual FText GetName() { return Name; }

	const float GetRange() const;
	const FSkillAnimation* GetAnimation(EGender gender = EGender::Female) const;
	void GetTimelines(TArray<URefSkillTimeline*>& out, uint8 tier);
	void AddTimeline(URefSkillTimeline* reference);
	void RemoveTimeline(URefSkillTimeline* reference);
	void ClearTimelines();

	void Init();

	uint32 GetPropertyValue(const FName& propName);

	URefSkillTimeline* GetTimelineByType(SkillEffect type);
};
#pragma endregion Skill