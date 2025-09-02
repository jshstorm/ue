#pragma once

#include "anu_global_def.h"

#include "CoreMinimal.h"
#include "XmlParser.h"
#include "Engine/DataTable.h"
#include "Sound/SoundBase.h"
#include "UObject/TextProperty.h"
#include "Engine/Texture2D.h"

#include "Reference.generated.h"

#define DATA_NULL_VALUE TEXT("null")

struct ANUREFERENCE_API FAnuResourceModelMesh;
class FJsonObject;
class URefSkill;
class URefSkillTimeline;

UENUM(BlueprintType)
enum class EPermissionType : uint8
{
	None,
	Tutorial = static_cast<uint8>(PermissionType::Tutorial),
	Beginner = static_cast<uint8>(PermissionType::Beginner),
	Originals = static_cast<uint8>(PermissionType::Originals),
	Friend = static_cast<uint8>(PermissionType::Friend),
	Arbeit = static_cast<uint8>(PermissionType::Arbeit),
	BattleStage = static_cast<uint8>(PermissionType::BattleStage) UMETA(DisplayName = "Stage"),
	Class = static_cast<uint8>(PermissionType::Class),
	Fashion_Shop = static_cast<uint8>(PermissionType::Fashion_Shop),
	Streaming = static_cast<uint8>(PermissionType::Streaming),
	Title = static_cast<uint8>(PermissionType::Title),
	Equip = static_cast<uint8>(PermissionType::Equip),
	Topic = static_cast<uint8>(PermissionType::Topic),
	Status = static_cast<uint8>(PermissionType::Status),
	MagicShop = static_cast<uint8>(PermissionType::MagicShop),
	UserInteraction = static_cast<uint8>(PermissionType::UserInteraction),
	Equip_Upgrade = static_cast<uint8>(PermissionType::Equip_Upgrade),
	Feed = static_cast<uint8>(PermissionType::Feed),
	Crew = static_cast<uint8>(PermissionType::Crew),
	MMO_Stage = static_cast<uint8>(PermissionType::MMO_Stage),
	Blacklist = static_cast<uint8>(PermissionType::Blacklist),
	QuestChallenge = static_cast<uint8>(PermissionType::QuestChallenge) UMETA(DisplayName = "Challenge"),
	Collection = static_cast<uint8>(PermissionType::Collection),
	Fashion_Color = static_cast<uint8>(PermissionType::Fashion_Color),
	Fashion_Snap = static_cast<uint8>(PermissionType::Fashion_Snap),
	Fashion_PVP = static_cast<uint8>(PermissionType::Fashion_PVP),
	Profile = static_cast<uint8>(PermissionType::Profile),
	MMO_Stage_Raid = static_cast<uint8>(PermissionType::MMO_Stage_Raid),
	MMO_Games = static_cast<uint8>(PermissionType::MMO_Games),
	MMO_FashionShow = static_cast<uint8>(PermissionType::MMO_FashionShow),
	Pamphlet = static_cast<uint8>(PermissionType::Pamphlet),
	Portal_Wolfgang = static_cast<uint8>(PermissionType::Portal_Wolfgang),
	Reforge = static_cast<uint8>(PermissionType::Reforge),
	Arbeit_Benefit = static_cast<uint8>(PermissionType::Arbeit_Benefit),
	Market_Benefit = static_cast<uint8>(PermissionType::Market_Benefit),
	Fashion_Emblem = static_cast<uint8>(PermissionType::Fashion_Emblem),
	Social_Benefit = static_cast<uint8>(PermissionType::Social_Benefit),
	Event_Menu = static_cast<uint8>(PermissionType::Event_Menu),
	MMO_Stage_Raid_Observation = static_cast<uint8>(PermissionType::MMO_Stage_Raid_Observation),
	Pass = static_cast<uint8>(PermissionType::Pass),
	Fashion_ColorPigment = static_cast<uint8>(PermissionType::Fashion_ColorPigment),
	Tower = static_cast<uint8>(PermissionType::Tower),
	Stage_Hard = static_cast<uint8>(PermissionType::Stage_Hard),
	SkillTree = static_cast<uint8>(PermissionType::SkillTree),
	Weapon_SubSlot,
	Weapon_Swap,

	Inaccessible = UINT8_MAX // 기능은 있으나 접근을 주고 싶지 않을때
};

UENUM(BlueprintType)
enum class EShortcutType : uint8
{
	None, // : 획득루트 표시 없음
	Life, 
	Stage,
	Shop,
	EquipCollection,
	Blacklist,
	Arbeit, 
	PamphletStamp,
	Subscription,
	Challenge,
	Championship,  // 챔피언십 UI이동 > 랭킹 토스트 메시지
	Championship_Pre, // 챔피언십 UI 이동 > 랭킹 토스트 메시지 x
	Fashionshow, // 패션쇼 UI 이동
};

class AnuText {
private:
	static FText Get_Internal(const FName& path, const FString& key) {
//#if UE_BUILD_SHIPPING
//		return FText::FromStringTable(path, key);
//#else
		FText found = FText::FromStringTable(path, key);
		if (found.IsFromStringTable() == false) {
			found = FText::FromString(FString::Printf(TEXT("<MISSING: %s>"), *key));
		}
		return found;
//#endif
	}

public:
	static const FString& GetStringTablePath() {
		static FString Path{ "/Game/Anu/DataTable/Strings" };
		return Path;
	}

	static const FName& Get_CommonTableID() {
		static FName TableID{ "/Game/Anu/DataTable/Strings/DT_Text.DT_Text" };
		return TableID;
	}

	static const FName& Get_DialogTableID() {
		static FName DialogTableID{ "/Game/Anu/DataTable/Strings/DT_Dialog_Text.DT_Dialog_Text" };
		return DialogTableID;
	}

	static FText Get_CommonTable(const FString& key)	{
		return Get_Internal(Get_CommonTableID(), key);
	}
	static FText Get_UITable(const FString& key) {
		return Get_Internal(TEXT("/Game/Anu/DataTable/Strings/DT_UI_Text.DT_UI_Text"), key);
	}
	static FText Get_EmbeddedTable(const FString& key) {
		return Get_Internal(TEXT("/Game/Embedded/DataTable/EM_DT_Text.EM_DT_Text"), key);
	}
	static FText Get_DialogTable(const FString& key) {
		return Get_Internal(Get_DialogTableID(), key);
	}
};

#define ANUTEXT(key) AnuText::Get_CommonTable(key)
#define ANUTEXT_UI(key) AnuText::Get_UITable(key)
#define ANUTEXT_EMBEDDED(key) AnuText::Get_EmbeddedTable(key)
#define ANUTEXT_DIALOG(key) AnuText::Get_DialogTable(key)

UENUM(BlueprintType)
enum class EAttachmentParts : uint8
{
	AP_Invalid,

	AP_Outfit,
	AP_Face,
	AP_Hair,
	AP_Shoes,

	AP_LBracelet,
	AP_Gloves,
	AP_Hat,
	AP_Eyewear,
	AP_Mask,
	AP_Back,
	AP_Tail,

	AP_RWeapon, // weapon
	AP_LWeapon,
	AP_RHand, // fairy gift
	AP_LHand, // phone

	AP_Ear,
	AP_Necklace,
	AP_Armband,
	AP_Lens,
	AP_MakeUp,
	
	AP_Max
};

UENUM(BlueprintType)
enum class ESubMeshType : uint8
{
	None,
	Pants,
	ThighLeg,
	ThinLeg,
	Hands,
	Weapon,
	Boots
};

UENUM(BlueprintType)
enum class EBodyParts : uint8 
{
	None,
	Face,
	Skin,
	Height,
	Frog,
	Npc,
};

UENUM(BlueprintType)
enum class ELifeObjectSequencerOption : uint8
{
	None = 0x00,
	MiniGame = 0x01,
	CineCamera = 0x02,
	Blueprint = 0x04,
};

#define SAFE_CAST(src, Type, dst) \
	check(Cast<Type>(src)); Type* dst = static_cast<Type*>(src);

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FConditionRule
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName Rule;
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> Values;
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, int32> MapValues;

	TArray<TPair<FName, int32>> Items; // <uid, amount>
	TArray<TPair<FName, int32>> EquipClassItems; // <class, slot>

	inline static FName NAME_QuestProgress{ "QuestProgress" };
	static void Parse(TArray<FConditionRule>& dst, const FString& conditionStr);

private:
	static void AddRule(TArray<FConditionRule>& paramDest, const FName& rule, const TArray<FString>& values);
	static void AddRule_OrderOfArrival(TArray<FConditionRule>& dst, const FName& rule, const TArray<FString>& values);
	static void AddRule_TakeItemBundle(TArray<FConditionRule>& dst, const FName& rule, const TArray<FString>& values);
	static void AddRule_Item(TArray<FConditionRule>& dst, const FName& rule, const TArray<FString>& values);
	static void AddRule_Equip(TArray<FConditionRule>& dst, const FName& rule, const TArray<FString>& values);
};

UENUM(BlueprintType)
enum class EGender : uint8 {
	Female,
	Male = static_cast<uint8>(Gender::Male),
	Invalid = static_cast<uint8>(Gender::Invalid),
};

USTRUCT()
struct ANUREFERENCE_API FDisplayInfo
{
	GENERATED_BODY()

public:
	TArray<TSoftObjectPtr<UTexture2D>> _icon;
	TArray<FText> _name;
	TArray<FText> _desc;

	UTexture2D* GetIcon(EGender gender) const;
	const FText& GetName(EGender gender) const;
	const FText& GetDesc(EGender gender) const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefBase : public UObject
{
	GENERATED_BODY()

// static
public:
	static TMap<FName, EPermissionType> PermissionNameOverrides;

	static void GetTrimedStringArray(const FString& originText, TArray<FString>& outArray, const FString& delimeter = "|");
	static void GetIntegerArray(const FString& originText, TArray<int32>& outArray);
	static void GetIntegerArray(const TArray<FString>& inArray, TArray<int32>& outArray);
	static void VisitAttributes(const FXmlNode* node, const FString& columName, TFunction<void(const FXmlAttribute*)>&& visitor);
	static const FString& GetShortenGenderString(EGender gender);
	static EPermissionType GetPermissionType(const FName& permissionStr);
	static FString GetPermissionStr(EPermissionType permission);

	static bool ParseStatInfo(const FString& statColumn, FStatInfoData& output);
	static void ParseStatInfo(const FXmlNode* node, const FString& statColumn, TMap<EStatTypes, FStatInfoData>& output);
	static void ParseTag(const FXmlNode* node, const FString& tagColumn, TMap<FName, int32>& output);

	static int32 ParseInteger(const FString& attrValue, bool infMax = true);
	static void ParseJson(const FJsonObject* jsonObj, URefBase* target);

// non-static
public:
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FName UID;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		int32 GUID = 0;

	FDisplayInfo _displayInfo;

	virtual void Parse(const FXmlNode* node);
	virtual void Parse(UScriptStruct* type, FTableRowBase* row);
	virtual void Parse(const FJsonObject* root);

	void ParseArrayProperty(const FArrayProperty* arrProp, const TArray<FString>& strValues, void* memberProp);

	FName GetEnumValueName(const FName& enumMemberName, int64 value);
	FString GetEnumValueString(const FName& enumMemberName, int64 value, bool trimNamespace = false);
};

// 삭제된 데이터 guid 수집할 때 쓰는 디버깅 용도
UCLASS(BlueprintType)
class ANUREFERENCE_API URefLegacy : public URefBase
{
	GENERATED_BODY()
public:
};

UENUM(BlueprintType)
enum class EShopCostType : uint8
{
	None,
	Currency,
	Item,
	QuestHost,
	ShopCost,
	Payment,
	Ad
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FDynamicCost
{
	GENERATED_BODY()

// non-static
public:
	UPROPERTY(BlueprintReadOnly)
		FName UID;

	FName Discount_Schedule;
	int32 Discount_Value = 0;
	// runtime
	FName _typeName;
	TArray<FDynamicCost> _replacables;
	class URefSchedule* _discountSchedule = nullptr;
	UPROPERTY(BlueprintReadOnly)
		TSubclassOf<URefBase> _type;
	UPROPERTY(BlueprintReadOnly)
		URefBase* _target = nullptr;

	void Parse(const FXmlNode* node, const std::string& typeColName = "Cost_Type", const std::string& uidColName = "Cost_UID", const std::string& valColName = "Cost_Value");
	void Parse(const FString& type, const FString& uid, const FString& amount);
	void ParseDiscount(const FXmlNode* node);


	UTexture2D* GetIcon(EGender gender) const;
	const FText& GetName(EGender gender) const;

	int32 GetOriginAmount() const;
	void SetOriginAmount(int32 amount);

private:
	int32 Amount = 0;
};

UCLASS(Abstract)
class ANUREFERENCE_API URefResourceBase : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FString Route;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefResourceAnimation : public URefResourceBase
{
	GENERATED_BODY()
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefResourceCore : public URefResourceBase
{
	GENERATED_BODY()
};

UENUM(BlueprintType)
enum class EStatFirstTypes : uint8
{
	Basic,
	Intensified
};

UENUM(BlueprintType)
enum class EStatSecondTypes : uint8
{
	None,
	Adventure,
	Travel,
	Social,
	ETC,
	Count	UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EStatSecondTypes, EStatSecondTypes::Count);

UENUM(BlueprintType)
enum class EStatTypes : uint8
{
	MoveSpeed,
	Tension,
	Strength,
	Will,
	Agility,
	Intelligence,
	Sense,
	HP,
	Melee_Attack,
	Magic_Attack,
	Melee_Defense,
	Magic_Defense,
	CriticalChance,
	CriticalDamage,
	HP_Regen,
	HP_RegenTick,
	ElementalDamage,
	Charge_Speed,
	Charge_Power,
	Attack_Chance_Basic,
	Shield_Hit,
	DamageReduction,
	FinalDamage,
	HP_Recovery,
	Observation,
	Concentration,
	Talent,
	Wit,
	Creativity,
	Expression,
	Luck,
	Sociality,
	Purchasing,
	Curiousity,
	Challenge_ItemBonus,
	Challenge_ScoreBonus,
	SkillEffect_Increase,
	SkillEffect_Decrease,
	Popularity_Total,

	SkillPower_Nature,
	SkillPower_Tidy,
	SkillPower_Fancy,
	SkillPower_RhythmicalDance,

	SkillDuration_InfiniteSlash,
	SkillRange_InfiniteSlash,
	SkillPower_InfiniteSlash,
	SkillPower_InfiniteSlash1,
	SkillPower_InfiniteSlash2,
	SkillPower_InfiniteSlash3,
	SkillDuration_ShieldOfAegis,
	Defense_ShieldOfAegis,
	CoolTime_ShieldOfAegis,
	SkillPower_ShieldOfAegis,
	SkillPower_ShieldOfAegis2,
	SkillPower_ShieldOfAegis3,
	SkillRange_RuneCross,
	SkillPower_RuneCross,
	SkillPower_RuneCross1,
	SkillPower_RuneCross2,
	SkillPower_RuneCross3,

	SkillRange_LightiningShock,
	SkillRange_LightiningShockMissile,
	SkillPower_LightiningShock,
	SkillPower_LightiningShock1,
	SkillPower_LightiningShock2,
	SkillPower_LightiningShock3,
	SkillDuration_ThousandArrows,
	SkillRange_ThousandArrows,
	SkillPower_ThousandArrows,
	SkillPower_ThousandArrows1,
	SkillPower_ThousandArrows2,
	Attack_Chance_ThousandArrows2,
	SkillPower_ThousandArrows3,
	SkillPower_ThousandArrows3_2,
	SkillRange_MagnumShot,
	SkillPower_MagnumShot,
	SkillPower_MagnumShot1,
	SkillPower_MagnumShot2,
	SkillPower_Boss_MagnumShot2,
	SkillPower_MagnumShot3,

	SkillTreeLevelUp_Runeknight_11,
	SkillTreeLevelUp_Runeknight_12,
	SkillTreeLevelUp_Runeknight_13,
	SkillTreeLevelUp_Runeknight_21,
	SkillTreeLevelUp_Runeknight_22,
	SkillTreeLevelUp_Runeknight_31,
	SkillTreeLevelUp_Runeknight_32,
	SkillTreeLevelUp_Runeknight_33,
	SkillTreeLevelUp_Runeknight_41,
	SkillTreeLevelUp_Runeknight_42,
	SkillTreeLevelUp_Runeknight_51,
	SkillTreeLevelUp_Runeknight_52,
	SkillTreeLevelUp_Runeknight_53,
	SkillTreeLevelUp_Runeknight_61,
	SkillTreeLevelUp_Runeknight_62,
	SkillTreeLevelUp_Runeknight_71,
	SkillTreeLevelUp_Runeknight_72,
	SkillTreeLevelUp_Runeknight_73,
	SkillTreeLevelUp_Blink_11,
	SkillTreeLevelUp_Blink_12,
	SkillTreeLevelUp_Blink_13,
	SkillTreeLevelUp_Blink_21,
	SkillTreeLevelUp_Blink_22,
	SkillTreeLevelUp_Blink_31,
	SkillTreeLevelUp_Blink_32,
	SkillTreeLevelUp_Blink_33,
	SkillTreeLevelUp_Blink_41,
	SkillTreeLevelUp_Blink_42,
	SkillTreeLevelUp_Blink_51,
	SkillTreeLevelUp_Blink_52,
	SkillTreeLevelUp_Blink_53,
	SkillTreeLevelUp_Blink_61,
	SkillTreeLevelUp_Blink_62,
	SkillTreeLevelUp_Blink_71,
	SkillTreeLevelUp_Blink_72,
	SkillTreeLevelUp_Blink_73,

	ClassExp_All,
	ClassMastery_All,
	ClassExp_Runeknight,
	ClassMastery_Runeknight,
	ClassExp_Blink,
	ClassMastery_Blink,
	ClassExp_ForestKeeper,
	ClassMastery_ForestKeeper,
	ClassExp_TownManager,
	ClassMastery_TownManager,
	ClassExp_Fisher,
	ClassMastery_Fisher,
	ClassExp_AnimalSitter,
	ClassMastery_AnimalSitter,
	ClassExp_Metalist,
	ClassMastery_Metalist,
	ClassExp_Gardener,
	ClassMastery_Gardener,
	ClassExp_FashionModel,
	ClassMastery_FashionModel,
	ClassExp_Photographer,
	ClassMastery_Photographer,
	ClassExp_Streamer,
	ClassMastery_Streamer,

	Currency_ChargeBonus,
	Currency_ChargeMax,
	Currency_AccumulateRate,
	Currency_ExchangeRate,
	
	StageReward_ClassExp,
	StageReward_Currency,
	StageDamage,

	Shop_SellBonus,
	Shop_BuyBonus,
	EquipUpgrade_CostDiscount,
	EmblemUpgrade_CostDiscount,
	ArbeitReward_Currency,
	UserInteractionReward_Currency,

	Count,
	None
};

UENUM(BlueprintType)
enum class EAnuElementType : uint8
{
	None,
	Fire,
	Water,
	Nature,
	Light,
	Dark
};

UENUM(BlueprintType)
enum class EEventSchedule : uint8
{
	None,
	Halloween,
	Christmas,
	Spring,
	Summer
};

UENUM(BlueprintType)
enum class EStatApplyType : uint8
{
	Invalid,
	//Invalid = static_cast<uint8>(StatApplyType::Invalid),
	Add = static_cast<uint8>(StatApplyType::Add),
	Per = static_cast<uint8>(StatApplyType::Per),
	Sec, // for display..
};

UENUM(BlueprintType)
enum class EClassLicense : uint8 {
	None, Green, Yellow, Red, Purple, Master,
};

USTRUCT(BlueprintType)
struct FStatInfoData
{
	GENERATED_USTRUCT_BODY()
public:
	FStatInfoData() {}
	FStatInfoData(const FName& statString, EStatTypes statType, StatValue value, EStatApplyType applyType = EStatApplyType::Add)
		: StatString(statString), StatType(statType), Value(value), ApplyType(applyType) {}

	UPROPERTY(BlueprintReadWrite)
	FName StatString = NAME_None;
	UPROPERTY(BlueprintReadWrite)
	EStatTypes StatType = EStatTypes::None;
	UPROPERTY(BlueprintReadWrite)
	int64 Value = 0; // StatValue와 동일해야 함. using이 uproperty에서는 안먹히네요..ㅎ >> 현재(11.02) int32로 변경되어있음..
	UPROPERTY(BlueprintReadWrite)
	EStatApplyType ApplyType = EStatApplyType::Invalid;
	UPROPERTY(BlueprintReadWrite)
	FName StatStringArg = NAME_None;
};
struct ANUREFERENCE_API FAnuTableRow;

UCLASS(BlueprintType)
class ANUREFERENCE_API URefObject : public URefBase
{
	GENERATED_BODY()

// static
public:
	inline static FName NAME_Object{ "Object" };
	static TMap<FName, EAttachmentParts> PartsByName;

	static EStatTypes GetStatEnum(const FName& text);
	static FName GetStatString(EStatTypes statType);
	UFUNCTION(BlueprintPure, Category="Reference|Stat")
	static FText GetStatText(EStatTypes statType);
	static EAttachmentParts GetAttachmentPart(const FName& partName);

// non-static
public:
	TypeID typeId;

	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	TArray<FName> typeNames;

	UPROPERTY()
	TArray<FText> Name;

	UPROPERTY()
	TArray<FText> Desc;

	UPROPERTY()
	TArray<FText> Desc_2;

	UPROPERTY()
	TArray<FName> Icon;

	UPROPERTY()
	TArray<FName> Icon_2;

	UPROPERTY()
	TArray<FName> Model;

	//runtime data
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	TArray<URefQuestSequence*> _listenQuests;

	UPROPERTY()
	TArray<TSoftObjectPtr<UTexture2D>> _icon2ObjectPtrs;

	TArray<FAnuTableRow*> _models;

	// getter
	UFUNCTION(BlueprintPure)
		UTexture2D* GetIconTexture(EGender gender = EGender::Female) const;
	UFUNCTION(BlueprintPure)
		UTexture2D* GetIcon2Texture(EGender gender = EGender::Female) const;
	UFUNCTION(BlueprintPure)
		const FName& GetIcon(EGender gender = EGender::Female);
	UFUNCTION(BlueprintPure)
		FName GetIcon2(EGender gender = EGender::Female);
	UFUNCTION(BlueprintPure)
		const FText& GetName(EGender gender = EGender::Female) const;
	UFUNCTION(BlueprintPure, DisplayName = "Get Desc")
		const FText& GetDescription(EGender gender = EGender::Female) const;
	UFUNCTION(BlueprintPure)
		const FText& GetDesc2(EGender gender = EGender::Female);
	UFUNCTION(BlueprintPure)
		UObject* GetModelObject(EGender gender = EGender::Female);

	/** Return dyeing texture sample if item is had, otherwise return nullptr. */
	UFUNCTION(BlueprintPure)
	UTexture2D* GetIconDyeingTexture(EGender gender, bool bMasking) const;
	UFUNCTION(BlueprintPure)
	bool IsDyeingIconTexture(EGender gender) const;

	virtual EGender GetGender() const { return EGender::Invalid; }

	virtual void Parse(const FXmlNode* node) override;
	virtual void Parse(UScriptStruct* type, FTableRowBase* row) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper);
	virtual void ParseStatData(const FXmlNode* node);

	void ResolveTypeID(const TMap<FName, int32>& mapper);
	FAnuResourceModelMesh* GetResourceModel(EGender gender = EGender::Female);
	EAttachmentParts GetAttachPart() const;
	const FName& GetLeafTypeID() const;
	bool EnableSpawnClientActor();

private:
	static EAttachmentParts GetAttachPart(TypeID typeId)
	{
		if (typeId.IsWeapon()) {
			if (typeId.IsSword()) {
				return EAttachmentParts::AP_RWeapon;
			}
			else if (typeId.IsBow()) {
				return EAttachmentParts::AP_LWeapon;
			}
		}
		else if (typeId.IsBracelet()) {
			return EAttachmentParts::AP_LBracelet;
		}
		else if (typeId.IsGloves()) {
			return EAttachmentParts::AP_Gloves;
		}
		else if (typeId.IsHat()) {
			return EAttachmentParts::AP_Hat;
		}
		else if (typeId.IsEyewear()) {
			return EAttachmentParts::AP_Eyewear;
		}
		else if (typeId.IsMask()) {
			return EAttachmentParts::AP_Mask;
		}
		else if (typeId.IsBack()) {
			return EAttachmentParts::AP_Back;
		}
		else if (typeId.IsTail()) {
			return EAttachmentParts::AP_Tail;
		}
		else if (typeId.IsEar()) {
			return EAttachmentParts::AP_Ear;
		}
		else if (typeId.IsNecklace()) {
			return EAttachmentParts::AP_Necklace;
		}
		else if (typeId.IsArmband()) {
			return EAttachmentParts::AP_Armband;
		}
		else if (typeId.IsLens()) {
			return EAttachmentParts::AP_Lens;
		}
		else if (typeId.IsMakeUp()) {
			return EAttachmentParts::AP_MakeUp;
		}
		else if (typeId.IsHair()) {
			return EAttachmentParts::AP_Hair;
		}
		else if (typeId.IsOutfit()) {
			return EAttachmentParts::AP_Outfit;
		}
		else if (typeId.IsShoes()) {
			return EAttachmentParts::AP_Shoes;
		}
		else if (typeId.IsPhone()) {
			return EAttachmentParts::AP_LHand;
		}

		return EAttachmentParts::AP_Invalid;
	}
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefLifeObject : public URefObject
{
	GENERATED_BODY()

public:
	inline static int32 MaxPriority = 0;
	inline static FName NAME_TID_1{ "LifeObject" };

public:
	UPROPERTY()
	FName Class_UID;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	EClassLicense License = EClassLicense::None;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	FName Sequencer_Type = NAME_None;
	UPROPERTY()
	TArray<FString> Sequencer_Option;
	UPROPERTY()
	FName Currency_UID;
	UPROPERTY(BlueprintReadOnly)
	int32 Currency_Cost;
	UPROPERTY()
	int32 Share_Max;
	UPROPERTY()
	int32 Total_Count;
	UPROPERTY()
	int32 Class_Exp;
	UPROPERTY()
	TArray<int32> Minigame_Class_Exp;
	UPROPERTY()
	FName LifeReward_UID;
	UPROPERTY()
	int32 Reward_Count;
	UPROPERTY()
	int32 Interaction_Time = 9000;
	UPROPERTY()
	int32 Priority;
	UPROPERTY()
	FText Action_Name;

public:
	//runtime data
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		class URefClass* _class = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		class URefCurrency* _costCurrency = nullptr;

	ELifeObjectSequencerOption _sequencerOptionFlags = ELifeObjectSequencerOption::None;

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;

	int32 GetTagOffset() const;

	bool ContainsSequencerOption(ELifeObjectSequencerOption value) { return ((uint8)_sequencerOptionFlags & (uint8)value) != 0; }
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefPortal : public URefLifeObject
{
	GENERATED_BODY()
	
public:
	PermissionType Permission = PermissionType::Invalid;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	FName PermissionErrorMsgID;

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefCurrency : public URefBase
{
	GENERATED_BODY()

public:
	inline static FName TableName{ "Currency" };

public:
	UPROPERTY(BlueprintReadOnly)
	int32 Max;
	UPROPERTY(BlueprintReadOnly)
	int32 Charge_Max;
	UPROPERTY()
	int32 Charge_Time;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY(BlueprintReadOnly)
	FName Icon;
	UPROPERTY(BlueprintReadOnly)
	int32 Inven_Display;
	UPROPERTY()
	FName Display_Schedule;
	UPROPERTY()
	TArray<EShortcutType> Shortcut_Type;
	UPROPERTY()
	FName Advertisement_Shop_UID;

public: // runtime
	URefSchedule* _displaySchedule = nullptr;
	TArray<URefCurrency*> _groupBySchedule;
	UPROPERTY(BlueprintReadOnly)
	URefShopItem* _advertisementShopItem = nullptr;

	bool IsChargeable() const;

	UFUNCTION(BlueprintPure)
	UTexture2D* GetIcon()
	{
		return _displayInfo.GetIcon(EGender::Female);
	}
};

UCLASS()
class ANUREFERENCE_API URefCurrencyStress : public URefCurrency
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Stat_Group;
};

UENUM(BlueprintType)
enum class EItemQualityUnit : uint8 {
	KG, CM, Quality, Score,
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItem : public URefObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	uint8 Grade = 1;
	UPROPERTY(BlueprintReadOnly)
	int32 Inven_Stack_Max = 9999;
	UPROPERTY()
	FName Effect_UID;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	int32 Effect_Value;
	UPROPERTY(BlueprintReadOnly)
	EItemQualityUnit Quality_Unit;
	UPROPERTY(BlueprintReadOnly)
	FName SetGroup;
	UPROPERTY()
	int32 Price = 0;
	UPROPERTY()
	TArray<EShortcutType> Shortcut_Type;
	UPROPERTY(BlueprintReadOnly)
	int32 Expiration_Min = 0;

public:
	//runtime data
	TMap<FName, int32> _tagWithValues;
	FName _dupReplacementUID;
	int32 _dupReplacementAmount = 0;
	UPROPERTY(BlueprintReadOnly)
		class URefItemReview* _review;
	UPROPERTY(BlueprintReadOnly)
		TMap<class URefTag*, int32> _tags;
	UPROPERTY(BlueprintReadOnly)
		bool _isPrizeItem = false;

public:
	UFUNCTION(BlueprintPure)
		class URefTag* GetFirstTag() const;
	UFUNCTION(BlueprintPure)
		bool IsStackable() const { return Inven_Stack_Max > 1; }
	UFUNCTION(BlueprintPure)
		bool IsPeriodic() const { return Expiration_Min != 0; }
	UFUNCTION(BlueprintPure)
		int32 GetTotalTagValue();
	UFUNCTION(BlueprintPure)
		int32 GetTotalTagValueByFilter(TSet<FString> filters, int32& applyCount);

	bool HasTag(const FName& tagName);

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
	virtual bool IsTranscendable() const { return false; }
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItemEquip : public URefItem
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Target_Class;
public:
	// runtime
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	class URefClass* _class = nullptr;
	UPROPERTY(BlueprintReadOnly)
	class URefEquipCraft* _craft = nullptr;
	UPROPERTY(BlueprintReadOnly)
	class URefEquipReforge* _reforge = nullptr;

	TMap<FName, TArray<class URefEquipAttribute*>> _attributes;
	TArray<URefEquipUpgradeEffect*> _upgradeEfts;
	StatValue StatMinSum = 0;
	StatValue StatMaxSum = 0;

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
	virtual bool IsTranscendable() const override { return true; }

	URefEquipUpgradeEffect* GetUpgradeEffect(int32 upgrade);
	URefEquipAttribute* GetAttribute(FName type);
	FEquipAttributeInfo* GetAttribute(FName type, FName effect) const;
	void GetStatInfoData(FName type, TArray<FStatInfoData>& output);
	FName GetWeaponType() const;
	FString GetWeaponAnimString() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItemCostume : public URefItem
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Resource_Key;
	UPROPERTY()
	FName Duplicate_Replacement;
	// runtime
	TOptional<TArray<uint32>> _defaultColorValues;
	EGender _cachedDefaultGender = EGender::Invalid;
	UPROPERTY(BlueprintReadOnly)
	TArray<FLinearColor> _defaultColors;
	UPROPERTY(BlueprintReadOnly)
	URefBody* _effectBody = nullptr;

	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItemEtc : public URefItem
{
	GENERATED_BODY()

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItemEmblem : public URefItem
{
	GENERATED_BODY()

// static
public:
	inline static int32 MaxUpgradeValue = 0;
	inline static int32 MaxUpgradeValue_NonTranscended = 0;
	inline static int32 MaxTranscendValue = 0;
	inline static int32 TranscendableGrade = 0;
	inline static TArray<int32> TranscendedUpgradeValues;

// non-static
public:
	UPROPERTY()
		int32 Order;
	UPROPERTY()
		FString Schedule;
	UPROPERTY()
		TArray<FString> Gacha_Sequence;
	UPROPERTY()
		TArray<int32> Material_Exp;
	UPROPERTY()
		TArray<int32> Upgrade_Exp;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		URefSchedule* _schedule = nullptr;

	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
	virtual bool IsTranscendable() const override;

	class URefEmblemEffect* GetUpgradeEffect(uint8 upgradeValue) const;
	int32 GetUpgradeExp(int32 curUpgradeValue) const;

	UFUNCTION(BlueprintPure)
		int32 GetMaterialExp(int32 upgradeValue) const;

	TArray<URefEmblemEffect*> _upgradeEffects;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItemUsable : public URefItem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
		URefCurrency* _targetCurrency = nullptr;
	UPROPERTY(BlueprintReadOnly)
		URefReward* _targetReward = nullptr;
	UPROPERTY(BlueprintReadOnly)
		URefClass* _targetClass = nullptr;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		TArray<URefShopGroup*> _targetShops;

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItemDyeing : public URefItemUsable
{
	GENERATED_BODY()

// static
public:
	inline static FName TID3{ "Dyeing" };

// non-static
public:	
	// runtime
	FDynamicCost _fixedCost;
	UPROPERTY(BlueprintReadOnly)
		URefColor* _color = nullptr;
	UPROPERTY(BlueprintReadOnly)
		URefPaletteGroup* _paletteGroup = nullptr;
	UPROPERTY(BlueprintReadOnly)
		FName _dyeingType;

	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItemQuest : public URefItem
{
	GENERATED_BODY()

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefBody : public URefBase
{
	GENERATED_BODY()
	// static
public:
	inline static FName SkinType = "Skin";
	inline static FName FaceType = "Face";
	static EBodyParts GetBodyParts(URefBody* refBody);
	static EBodyParts GetBodyParts(const FName& partsName);

	// non-static
public:
	UPROPERTY(BlueprintReadOnly)
		FName Part;
	UPROPERTY()
		TArray<FName> Model;
	UPROPERTY()
		TArray<FName> Icon;
	UPROPERTY()
		FName CustomDetail_UID;
	UPROPERTY()
		FName Default_Color_UID;
	UPROPERTY()
		FName Target_Character;
	// runtime
	TArray<FAnuResourceModelMesh*> _meshes;
	class URefCustomDetail* _refCustomDefault = nullptr;
	class URefColor* _defaultColor = nullptr;
	class URefCharacter* _targetCharacter = nullptr;
	class URefItemCostume* _skinCostume = nullptr;
	UPROPERTY(BlueprintReadOnly)
	EBodyParts _parts = EBodyParts::None;

public:
	virtual void Parse(const FXmlNode* node) override;

	FAnuResourceModelMesh* GetResourceMesh(EGender gender = EGender::Female);
	UFUNCTION(BlueprintPure)
		UTexture2D* GetIconTexture(EGender gender = EGender::Female);
};

struct ANUREFERENCE_API FAnuResourceCharacterClass;

UCLASS(Abstract)
class ANUREFERENCE_API URefCharacter : public URefObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 Weight = 1;
	UPROPERTY()
	FString Name_Key;
	UPROPERTY(BlueprintReadOnly)
	TArray<FName> GroupName;
	UPROPERTY()
	FName StatKey;

	//runtime
	UPROPERTY()
	URefCharacterStat* BaseStat;
	UPROPERTY(BlueprintReadOnly)
	TMap<EStatTypes, FStatInfoData> _stats;

	TMap<FName, TArray<class URefDialog*>> _dialogbyTriggerType;

public:
	// runtime
	TArray<URefSkill*> _builtInSkills;

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseStatData(const FXmlNode* node) override;
	virtual FName GetDialogType() const;
	
	UFUNCTION(BlueprintPure)
	void GetDialogs(const FName& triggerType, TArray<URefDialog*>& dialogs);
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefPC : public URefCharacter
{
	GENERATED_BODY()

public:
	inline static FName BasicUID{ "objt.pc.basic" };

public:
	UPROPERTY()
	FName Base_UID;

	UPROPERTY(BlueprintReadOnly)
	EGender Gender = EGender::Invalid;

	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;

	virtual EGender GetGender() const override { return Gender; }
	virtual FName GetDialogType() const override { return "PC"; }
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefMonster : public URefCharacter
{
	GENERATED_BODY()

public:
	UPROPERTY()
	uint32 Pursuit_StartAnim;
	UPROPERTY()
	FName Brain;
	UPROPERTY()
	float SphereColliderRadius;
	UPROPERTY()
	int32 SimulateCollision;
	UPROPERTY()
	float RotationSpeed = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	FText Title;
	UPROPERTY()
	int32 InvincibleTime;
	UPROPERTY()
	float GetUp_Frame = 0.0f;
	UPROPERTY()
	int32 HPCount;
	UPROPERTY()
	TArray<int32> PhaseRange;

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefGimmick : public URefObject
{
	GENERATED_BODY()

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefNPC : public URefCharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
		TArray<FName> Menu;
	UPROPERTY()
		FName Unlock_Condition;
	UPROPERTY()
		TArray<FName> Shop_UID;
	UPROPERTY()
		FName Spawn_Schedule;
	UPROPERTY(BlueprintReadOnly)
		float Recognition_Dist_Squared = 0.0f;
	UPROPERTY(BlueprintReadOnly)
		int32 Friend;
	UPROPERTY()
		int32 Order;
	UPROPERTY()
		FText Nickname;
	UPROPERTY(BlueprintReadOnly)
		FName BackGround_UID;
	// runtime
	URefQuest* _unlockQuest = nullptr;
	URefQuest* _friendOpenQuest;
	TArray<FConditionRule> _interactionConditions;
	TArray<URefShopGroup*> _shops;
	FName _moveEvtUID;
	FName _menuEvtUID;
	FName _marketEvtUID;
	UPROPERTY(BlueprintReadOnly)
		URefSchedule* _schedule = nullptr;
	
	//////////FOR BATTLE NPC//////////
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		int32 Level;
	UPROPERTY()
		uint32 Pursuit_StartAnim;
	UPROPERTY()
		FName Brain;
	UPROPERTY()
		float SphereColliderRadius;
	UPROPERTY()
		int32 SimulateCollision;
	UPROPERTY()
		float RotationSpeed = 0.0f;
	UPROPERTY(BlueprintReadOnly)
		FText Title;
	UPROPERTY()
		float GetUp_Frame = 0.0f;
	UPROPERTY()
		int32 HPCount;
	UPROPERTY()
		TArray<int32> PhaseRange;
	//////////FOR BATTLE NPC//////////

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseTypeID(const FXmlNode* node, const TMap<FName, int32>& mapper) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefCharacterStat : public URefCharacter
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName PrimaryKey;
	UPROPERTY()
	TMap<EStatTypes, int32> DefaultStats;
	UPROPERTY()
	TMap<EStatTypes, int32> OffenseStats;
	UPROPERTY()
	TMap<EStatTypes, int32> DefenseStats;

public:
	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefGlobal : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString Key;
	UPROPERTY()
		FString Value;

	virtual void Parse(const FXmlNode* node) override { }
	static URefGlobal* StaticParse(const FXmlNode* node);
};

USTRUCT()
struct ANUREFERENCE_API FRTRefClassLicense
{
	GENERATED_USTRUCT_BODY()

public:
	TArray<class URefQuest*> _openRefQuest;
	TArray<class URefClassLicenseMastery*> _masteries;
	TArray<FStatInfoData> _statRwd;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefClass : public URefBase
{
	GENERATED_BODY()

public:
	static inline TMap<FName, URefClass*> s_OpenPreQuests;
	static EClassLicense GetLicenceType(const FString& value);
	static FName GetLicenseString(EClassLicense license);
	static FText GetLicenseDisplayText(EClassLicense license);

public:
	UPROPERTY(BlueprintReadOnly)
	FName Category;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	FText Desc;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	FText Desc_2;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	FText Desc_3;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	FText Desc_4;
	UPROPERTY(BlueprintReadOnly)
	FName Icon;
	UPROPERTY()
	FString Default_CameraProperty;
	UPROPERTY()
	int32 Order;
	UPROPERTY()
	bool Activate = true;
	UPROPERTY()
	TArray<FName> SpecialStat;
	UPROPERTY(BlueprintReadOnly)
	EClassLicense Active_License;
	UPROPERTY()
	TArray<FText> LicenseDesc;
	TMap<EClassLicense, TArray<FName>> OpenQuest_UID;

public:
	virtual void Parse(const FXmlNode* node) override;

	void ForeachOpenQuest(EClassLicense type, TFunction<bool(URefQuest*)>&& visitor);
	void ForeachMastery(EClassLicense type, TFunction<bool(URefClassLicenseMastery*)>&& visitor);
	bool FindOpenQuest(const FName& questUID) const;

	const FText& GetRawName() const;
	const FText& GetRawEngName() const;
	FRTRefClassLicense& GetRTLicense(EClassLicense type);
	UFUNCTION(BlueprintPure)
	URefQuest* GetLicenseUnlockQuest(EClassLicense type);
	UFUNCTION(BlueprintPure)
	FText GetLicenseDesc(EClassLicense type) const;
	UFUNCTION(BlueprintPure)
	UTexture2D* GetIconTexture() const;

protected:
	UPROPERTY()
		FText Name; // Local; use GetRefClassName for getting localized-text
	UPROPERTY()
		FText Name_Title; //English name; use GetRefClassEngName for getting localized-text

public:
	TSoftObjectPtr<UTexture2D> _iconTexture;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	TMap<int32, class URefClassLevel*> _classLevels;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	UObject* _cameraProperty;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	FLinearColor _color;
	UPROPERTY()
	TArray<URefSkill*> _lvSkills; // class level accm skills
	UPROPERTY()
	TArray<URefStat*> _extraExpStats;
	UPROPERTY()
	TArray<URefStat*> _extraMasteryStats;
	UPROPERTY()
	TArray<URefStat*> _specialStats;

public:
	TMap<EClassLicense, FRTRefClassLicense> _rtLicense;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefClassLevel : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Class_UID;
	UPROPERTY()
		int32 Level;
	UPROPERTY(BlueprintReadOnly)
		int32 Exp;
	UPROPERTY()
		TArray<FName> Open_Skill;
public:
	virtual void Parse(const FXmlNode* node) override;

public:
	//runtime data
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		URefClass* _class = nullptr;
	UPROPERTY(BlueprintReadOnly)
		TArray<FStatInfoData> _statGrowthValues;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefClassLicenseMastery : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	uint8 Order;
	UPROPERTY()
	FName Class_UID;
	UPROPERTY(BlueprintReadOnly)
	EClassLicense License_Type;
	UPROPERTY()
	FName Condition;
	UPROPERTY(BlueprintReadOnly)
	int32 Mastery_Count;
	UPROPERTY(BlueprintReadOnly)
	int32 Condition_Count;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY()
	FText Desc_Detail_1;
	UPROPERTY()
	FText Desc_Detail_2;
	UPROPERTY()
	FText Desc_Detail_3;
	UPROPERTY(BlueprintReadOnly)
	FName Icon;
	UPROPERTY()
	TArray<FConditionRule> _conditions;

public:
	virtual void Parse(const FXmlNode* node) override;

	UFUNCTION(BlueprintPure)
	FText GetHintDesc();

public: // runtime
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	class URefObject* _refObject = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	class URefClass* _refClass = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
	FText _hintDesc;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefLevelPC : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		int32 Level;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		int64 Exp;
	UPROPERTY()
		TArray<FName> Currency_MAX;
	UPROPERTY()
		TArray<int32> Currency_MAX_Value;

	// runtime data
	UPROPERTY(BlueprintReadOnly)
	TMap<EStatTypes, int32> _statCurrencyValues;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefLevelNPC : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		int32 Level;
	UPROPERTY()
		int64 Exp;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefRegion : public URefBase
{
	GENERATED_BODY()

public:
	inline static FName DefaultVillage{ "region.village.basic" };

public:
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY()
	FName Model;
	UPROPERTY()
	FName Rule_Quest;
	UPROPERTY()
	FName Game_Rule;
	UPROPERTY(BlueprintReadOnly)
	FName Game_Rule_1;
	UPROPERTY()
	FName WorldUID;
	UPROPERTY()
	FName Unlock_Condition;
	UPROPERTY()
	FString SpawnPath;

public: //runtime data
	UPROPERTY()
	URefWorld* _world;
	UPROPERTY(BlueprintReadOnly)
	URefQuest* _ruleQuest;
	UPROPERTY(BlueprintReadOnly)
	URefQuest* _unlockQuest;
	UPROPERTY(BlueprintReadOnly)
	URefStageContest* _contest;

public:
	FName _modelName;
};

UENUM(BlueprintType)
enum class EStageType : uint8 {
	None /*= static_cast<uint8>(StageType::None)*/,
	Village, Tutorial, Normal, Multi, Raid, Game, Fashion, QuickMatch, Arbeit, Tower
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefStageInfo : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	FName StageGroup_UID;
	UPROPERTY(BlueprintReadOnly)
	int32 Order;
	UPROPERTY(BlueprintReadOnly)
	int32 Display_Order;
	UPROPERTY()
	FName Unlock_Condition;
	UPROPERTY()
	FName Region;
	UPROPERTY()
	FName LevelDesign;
	UPROPERTY()
	FName StageReward_UID;
	UPROPERTY()
	int32 Class_Exp;
	UPROPERTY()
	FName Reward_Currency;
	UPROPERTY()
	int32 Reward_Currency_Value; 
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY(BlueprintReadOnly)
	FText Guide;
	UPROPERTY()
	FName Icon;
	UPROPERTY(BlueprintReadOnly)
	TArray<FText> Tag;
	TArray<TPair<FName, int32>> Currency;
	UPROPERTY()
	TArray<FName> Display_Reward;
	UPROPERTY(BlueprintReadOnly)
	int32 Recom_FP;
	UPROPERTY(BlueprintReadOnly)
	int32 Required_FP;
	UPROPERTY(BlueprintReadOnly)
	int32 Round_Count;
	UPROPERTY(BlueprintReadOnly)
	int32 Time_Limit;
	UPROPERTY()
	int32 Level_Defense;
	UPROPERTY()
	int32 Level_Offense;
	UPROPERTY()
	TMap<EStatTypes, int32> Mod_Stat;
	UPROPERTY()
	FName Intro_Seq;
	UPROPERTY()
	FName Ending_Seq;
	UPROPERTY()
	TArray<FName> Redirect_Stage;
	UPROPERTY()
	FName ResetSchedule;
	UPROPERTY(BlueprintReadOnly)
	FName OpenSchedule;
	UPROPERTY()
	int32 EnterCount;
	UPROPERTY(BlueprintReadOnly)
	int32 TeamCount;
	UPROPERTY(BlueprintReadOnly)
	int64 Power;

public:
	UFUNCTION(BlueprintPure)
	URefWorld* GetRefWorld() const { return _region->_world; }
	UFUNCTION(BlueprintPure)
	bool UseMatchMaking() const;
	UFUNCTION(BlueprintPure)
	EStageType GetStageType() const;
	UFUNCTION(BlueprintPure)
	UTexture2D* GetAssetIcon();
	UFUNCTION(BlueprintPure)
	bool IsFinalStage() const;

	virtual void Parse(const FXmlNode* node) override;
public:
	//runtime data
	UPROPERTY()
	int32 Entry_Min;
	UPROPERTY(BlueprintReadOnly)
	int32 Entry_Max;
	UPROPERTY(BlueprintReadOnly)
	class URefStageGroup* _refStageGroup = nullptr;
	UPROPERTY(BlueprintReadOnly)
	class URefRegion* _region = nullptr;
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> Asset_Icon;
	UPROPERTY(BlueprintReadOnly)
	TArray<URefBase*> _displayRewards;
	UPROPERTY(BlueprintReadOnly)
	URefStageInfo* _unlockCondition = nullptr;
	TArray<TPair<URefCurrency*, int32>> _costCurrency;
	UPROPERTY(BlueprintReadOnly)
	URefSchedule* _resetSchedule = nullptr;
	UPROPERTY(BlueprintReadOnly)
	URefSchedule* _openSchedule = nullptr;
	UPROPERTY(BlueprintReadOnly)
	URefStageContest* _refStageContest = nullptr;
	UPROPERTY(BlueprintReadOnly)
	TArray<URefStageInfo*> _redirectStage;
	
	UPROPERTY()
	TArray<class URefStageReward*> _rewards;

public:
	UFUNCTION(BlueprintPure, Category=Runtime)
	bool GetEnterCostInfo(int32 index, URefCurrency*& outCurrency, FName& outCurrencyUID, int32& outAmount) const;
};

UENUM(BlueprintType)
enum class EStageDifficulty : uint8 {
	None, Easy, Hard, Hell
};

UENUM(BlueprintType)
enum class EStageRegion : uint8 {
	None = 0,

	ParellelWorld,
	UnderColony,
	RedNova,

	Challange,
	Observation,

	Prototype,
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefStageGroup : public URefBase
{
	GENERATED_BODY()
public:
	int32 Index = 0;
	UPROPERTY()
	bool Activate = false;
	UPROPERTY()
	FName Region;
	UPROPERTY(BlueprintReadOnly)
	int32 Order;
	UPROPERTY(BlueprintReadOnly)
	EStageType Type;
	UPROPERTY(BlueprintReadOnly)
	EStageDifficulty Difficulty;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY(BlueprintReadOnly)
	FName Icon;
	UPROPERTY(BlueprintReadOnly)
	FName BackgroundUID;
	UPROPERTY()
	FName Episode_UID;
	UPROPERTY()
	TArray<FName> Keywords;
	
public:
	//runtime data
	URefQuestGroup* _episode = nullptr;
	FString _typeString;
	TArray<EPermissionType> _permissions;
	EStageRegion _stageRegion;
	UPROPERTY(BlueprintReadOnly)
	TArray<class URefStageInfo*> _stages;
	UPROPERTY(BlueprintReadOnly)
	TArray<URefQuest*> _keywords;
	UPROPERTY(BlueprintReadOnly)
	FText _regionText;
	UPROPERTY(BlueprintReadOnly)
	FText _regionDescText;

public:
	virtual void Parse(const FXmlNode* node) override;
};

UENUM(BlueprintType)
enum class EContestType : uint8
{
	None,
	FashionShow,
	Championship,
	Games,
	GamesQuick
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefStageContest : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FString ContestType;
	UPROPERTY(BlueprintReadOnly)
	TArray<FName> StageGroupUID;
	UPROPERTY()
	FName OpenScheduleUID;
	UPROPERTY()
	FName CloseScheduleUID;
	UPROPERTY(BlueprintReadOnly)
	FName RankingRewardName;
	UPROPERTY(BlueprintReadOnly)
	int32 RankerCount;
	UPROPERTY(BlueprintReadOnly)
	int32 WinnerCount;
	UPROPERTY(BlueprintReadOnly)
	int32 QualiferScore;
	UPROPERTY(BlueprintReadOnly)
	int32 MinSpectatorCount;
	UPROPERTY(BlueprintReadOnly)
	int32 MaxSpectatorCount;
	UPROPERTY(BlueprintReadOnly)
	int32 ReadyTimeMS;
	UPROPERTY(BlueprintReadOnly)
	int32 ReadyNotiIntervalMS;
	UPROPERTY(BlueprintReadOnly)
	FName UpdateAlarmID;
	UPROPERTY(BlueprintReadOnly)
	FName StartAlarmID;

public:
	// runtime
	inline static TMap<EContestType, URefStageContest*> map;

	EContestType _type = EContestType::None;
	EPermissionType _playerPermission = EPermissionType::None;
	EPermissionType _spectatorPermission = EPermissionType::None;
	TArray<URefStageInfo*> _stages;
	TArray<URefRankingReward*> _rewards;
	URefSchedule* _closeSchedule;
	UPROPERTY(BlueprintReadOnly)
	URefSchedule* _openSchedule;
	
public:
	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefContestDonationGuide : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FName Contest;
	UPROPERTY(BlueprintReadOnly)
	FName GuideKey;
	UPROPERTY(BlueprintReadOnly)
	FString Value;

	virtual void Parse(const FXmlNode* node) override;

	static TMap<FName, TMap<FName, URefContestDonationGuide*>> _byContest;

	UFUNCTION(BlueprintPure)
	static URefContestDonationGuide* GetDonationGuide(FName contestUID, FName key);
};

UENUM(BlueprintType)
enum class EChatStickerGroup : uint8 { 
	None	UMETA(DisplayName = "None"), 
	Sticker UMETA(DisplayName = "Sticker"),
	Emoji	UMETA(DisplayName = "Emoji"),
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefChatStickerGroup : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FName Icon;
	UPROPERTY()
	int32 Order;
	UPROPERTY(BlueprintReadOnly)
	EChatStickerGroup Type = EChatStickerGroup::None;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY(BlueprintReadOnly)
	bool IsSubscription{ false };
	UPROPERTY()
	TArray<EShortcutType> Shortcut_Type;

public:
	//runtime data
	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> ImageTexture = nullptr;
	UFUNCTION(BlueprintPure)
	UTexture2D* GetIconTexture() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefWorld : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
		FName Type;
	UPROPERTY()
		FString Name;
	UPROPERTY()
		FName Model;
	UPROPERTY()
		TArray<FName> MoveSequence;
	UPROPERTY()
		TArray<FName> SequencePlayWorld;
	UPROPERTY()
		FName LoadingType = "normal";
public:
	//runtime data
	UPROPERTY(BlueprintReadOnly)
		TArray<URefRegion*> _regions;
	
	FName _modelName;
};

UENUM(BlueprintType)
enum class ERewardTge : uint8 {
	Invalid,
	//Invalid = static_cast<uint8>(RewardGrade::None),
	Good,
	Great,
	Excellent,
};

UENUM(BlueprintType)
enum class ERewardObjectType : uint8 {
	Invalid,
	//Invalid = static_cast<uint8>(RewardObjectType::Invalid),
	Object = static_cast<uint8>(RewardObjectType::Object),
	Currency = static_cast<uint8>(RewardObjectType::Currency),
	ClassExp = static_cast<uint8>(RewardObjectType::ClassExp),
	Follow = static_cast<uint8>(RewardObjectType::Follow),
	Friend = static_cast<uint8>(RewardObjectType::Friend),
	Favor = static_cast<uint8>(RewardObjectType::Favor),
	Popularity = static_cast<uint8>(RewardObjectType::Popularity),
	Title = static_cast<uint8>(RewardObjectType::Title),
	UserInteraction = static_cast<uint8>(RewardObjectType::UserInteraction),
	ClassLicense = static_cast<uint8>(RewardObjectType::ClassLicense),
	Tag = static_cast<uint8>(RewardObjectType::Tag),
	Permission = static_cast<uint8>(RewardObjectType::Permission),
	Stat = static_cast<uint8>(RewardObjectType::Stat),
	QuestPoint = static_cast<uint8>(RewardObjectType::QuestPoint),
	Palette = static_cast<uint8>(RewardObjectType::Palette),
	Stress = static_cast<uint8>(RewardObjectType::Stress),
	ChatSticker = static_cast<uint8>(RewardObjectType::ChatSticker),
	SocialMotion = static_cast<uint8>(RewardObjectType::SocialMotion),
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefRewardBase : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Reward_UID;
	UPROPERTY(BlueprintReadOnly)
		FName Reward_Type;
	UPROPERTY()
		FName Reward_Item_UID;
	UPROPERTY(BlueprintReadOnly)
		int32 Amount = 0;
	UPROPERTY(BlueprintReadOnly)
		ERewardTge Reward_Tag;
	UPROPERTY()
		TArray<FName> Display_Tag;

public:
	// runtime data
	TArray<URefTag*> _displayTags;
	UPROPERTY(BlueprintReadOnly)
		ERewardObjectType _rewardObjType;
	UPROPERTY(BlueprintReadOnly)
		class URefReward* _reward = nullptr;
	UPROPERTY(BlueprintReadOnly)
		int32 _implGuid = 0;
	UPROPERTY(BlueprintReadOnly)
		class URefItem* _item = nullptr;
	UPROPERTY(BlueprintReadOnly)
		class URefCurrency* _currency = nullptr;
	UPROPERTY(BlueprintReadOnly)
		class URefCharacter* _character = nullptr;
	UPROPERTY(BlueprintReadOnly)
		class URefTitle* _title = nullptr;
	UPROPERTY(BlueprintReadOnly)
		class URefPaletteGroup* _palette = nullptr;
	UPROPERTY(BlueprintReadOnly)
		EStatTypes _statType = EStatTypes::None;
	UPROPERTY(BlueprintReadOnly)
		EClassLicense LicenseType = EClassLicense::None;
	UPROPERTY(BlueprintReadOnly)
		class URefClass* _class = nullptr;
	UPROPERTY(BlueprintReadOnly)
		class URefChatStickerGroup* _chatSticker = nullptr;
	UPROPERTY(BlueprintReadOnly)
		class URefTag* _tag = nullptr;

	UFUNCTION(BlueprintPure)
		const FText& GetName(EGender gender) const;
	UFUNCTION(BlueprintPure)
		const FText& GetDescription(EGender gender) const;
	UFUNCTION(BlueprintPure)
		UTexture2D* GetIconTexture(EGender gender) const;
	UFUNCTION(BlueprintPure)
		URefBase* GetRewardItemRef() const;
	UFUNCTION(BlueprintPure)
		virtual float GetProbability() const { return 1.f; }
	UFUNCTION(BlueprintPure)
		bool HasDisplayTag(FName tagUID);

public:
	FText _rewardObjectName{ FText::GetEmpty() };
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefRewardStatic : public URefRewardBase
{
	GENERATED_BODY()
public:
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefRewardRandom : public URefRewardBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		int32 Prob;

	virtual float GetProbability() const override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefLifeReward : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FName LifeReward_UID;
	UPROPERTY()
	FName Reward_UID;
	UPROPERTY()
	int32 Enable_Shortcut;
};

UCLASS()
class ANUREFERENCE_API URefStageReward : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FName StageReward_UID;
	UPROPERTY()
	FName Reward_UID;

public:
	TArray<FConditionRule> _condition;

public:
	virtual void Parse(const FXmlNode* node);
	const FConditionRule* GetRule(const FName& ruleName) const;

private:
	void ParseCondition(const FXmlNode* node);
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefRewardSelectable : public URefRewardBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Select_Group;
	UPROPERTY(BlueprintReadOnly)
		int32 Select_Index;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefRewardPost : public URefRewardBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName PostTemplate_UID;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefRewardPeriod : public URefRewardBase
{
	GENERATED_BODY()

public:

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> Source_Contents;
	UPROPERTY(BlueprintReadOnly)
	FText Source_Name;
	UPROPERTY(BlueprintReadOnly)
	FText Source_Desc;
	UPROPERTY(BlueprintReadOnly)
	FText Source_ShortenName;
	UPROPERTY()
	int32 Display_Priority = 0;
	UPROPERTY()
	int32 Duration = 0;
	// runtime
	UPROPERTY(BlueprintReadOnly)
	TArray<URefSchedule*> _calendar;
};

UENUM(BlueprintType)
enum class ERewardType : uint8 {
	Invalid,
	Static,
	Random,
	Selectable,
	PostBox,
	Period,
};

USTRUCT(BlueprintType)
struct FRewardInfo
{
	GENERATED_USTRUCT_BODY()
public:
	FRewardInfo()
		: Reward(nullptr) {}

	FRewardInfo(URefRewardBase* reward)
		: Reward(reward), Amount(reward->Amount) {}

	UPROPERTY(BlueprintReadOnly)
		URefRewardBase* Reward;

	UPROPERTY(BlueprintReadOnly)
		int32 Amount = 0;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefReward : public URefBase
{
	GENERATED_BODY()
public:
	static TMap<FName, ERewardObjectType> s_rewardObjTypes;
	static ERewardObjectType GetType(const FName& typeStr);
	static FName GetTypeStr(ERewardObjectType rwdObjType);

public:
	UPROPERTY()
	FName Group_UID;
	UPROPERTY(BlueprintReadOnly)
	ERewardType RewardType;
	UPROPERTY(BlueprintReadOnly)
	TArray<FRewardInfo> RewardItems;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY(BlueprintReadOnly)
	FName Icon;
	// runtime
	int32 _rewardCount = 0;
	int32 _sumOfProbability = 0;
	URefRankingReward* _rankingReward = nullptr;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefSubscriptionReward*> _subsRwd;

public:
	UFUNCTION(BlueprintCallable)
	void GetRewardInfo(int32 index, FRewardInfo& info);
	UFUNCTION(BlueprintCallable)
	void GetRewardByGroup(const FName& selectGroup, TArray<URefRewardBase*>& rewards);
	UFUNCTION(BlueprintPure)
	URefRewardBase* GetReward(ERewardObjectType type);
	UFUNCTION(BlueprintPure)
	URefRewardBase* GetRepresentative(ERewardObjectType type, FName tagUID) const;
	UFUNCTION(BlueprintPure)
	UTexture2D* GetIconTexture(EGender gender, FName tagUID) const;
	UFUNCTION(BlueprintPure)
	void GetRewardsByTag(FName displayTag, TArray<FRewardInfo>& output) const;

	int32 GetCurrencyAmount(URefCurrency* currency);
	void Visit(ERewardObjectType type, TFunction<void(URefRewardBase*, int32)>&& visitor);
};

UENUM(BlueprintType)
enum class EQuestGroupType : uint8 {
	Invalid,
	//Invalid = static_cast<uint8>(QuestGroupType::Invalid),
	Tutorial = static_cast<uint8>(QuestGroupType::Tutorial),
	Essential = static_cast<uint8>(QuestGroupType::Essential),
	Arbeit = static_cast<uint8>(QuestGroupType::Arbeit),
	Epic = static_cast<uint8>(QuestGroupType::Epic),
	Keyword = static_cast<uint8>(QuestGroupType::Keyword),
	Hidden = static_cast<uint8>(QuestGroupType::Hidden),
	Hidden_Event = static_cast<uint8>(QuestGroupType::Hidden_Event),
	Fairy = static_cast<uint8>(QuestGroupType::Fairy),
	TimeEvent = static_cast<uint8>(QuestGroupType::TimeEvent),
	Attendance = static_cast<uint8>(QuestGroupType::Attendance),
	PassMission = static_cast<uint8>(QuestGroupType::PassMission),
	Activity = static_cast<uint8>(QuestGroupType::Activity),
	Stage = static_cast<uint8>(QuestGroupType::Stage),
	Blacklist = static_cast<uint8>(QuestGroupType::Blacklist),
	Challenge = static_cast<uint8>(QuestGroupType::Challenge),
	Prototype = static_cast<uint8>(QuestGroupType::Prototype),
	ContentsGuide = static_cast<uint8>(QuestGroupType::ContentsGuide),
	Class = static_cast<uint8>(QuestGroupType::Class),
	MassiveArbeit = static_cast<uint8>(QuestGroupType::MassiveArbeit),
	Count = static_cast<uint8>(QuestGroupType::Count),
};
ENUM_RANGE_BY_COUNT(EQuestGroupType, EQuestGroupType::Count);

UENUM(BlueprintType)
enum class EArbeitCategory : uint8 {
	Invalid,
	None,
	Subscription,
	Massive_Default,
	Massive_Battle,
	Seasonal_Christmas,
	Seasonal_Luna,
	Seasonal_Spring,
	Seasonal_Summer,
	Seasonal_Halloween,
};

UENUM(BlueprintType)
enum class EQuestProgress : uint8 {
	Acceptable UMETA(DisplayName = "Acceptable"),
	Progress UMETA(DisplayName = "Progress"),
	Complete UMETA(DisplayName = "Complete"),
	Expired UMETA(DisplayName = "Expired"),
	Invalid
};

class URefQuestEvent;
UCLASS(BlueprintType)
class ANUREFERENCE_API URefQuest : public URefBase
{
	GENERATED_BODY()
public:
	static bool IsUseHUDQuest(EQuestGroupType group);
	static EQuestProgress GetProgressByStr(const FName& prgStr);
	static TMap<int32, TSet<EQuestProgress>> FeedTriggers;

	UPROPERTY(BlueprintReadOnly)
		FName QuestGroup;
	UPROPERTY(BlueprintReadOnly)
		int32 Order;
	UPROPERTY()
		FName PreCondition;
	UPROPERTY(BlueprintReadOnly)
		int32 Repeat = 1;
	UPROPERTY(BlueprintReadOnly)
		FName SubGroup;
	UPROPERTY(BlueprintReadOnly)
		FName Category;
	UPROPERTY(BlueprintReadOnly)
		FName Icon;
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		TArray<FText> Desc;
	UPROPERTY()
		TArray<FName> Reward;
	UPROPERTY()
		TArray<FName> Reward_Bonus;
	UPROPERTY()
		TArray<FString> Class_Exp;
	UPROPERTY(BlueprintReadOnly)
		FText OpenComment;
	UPROPERTY(BlueprintReadOnly)
		int32 Grade = 0;
	UPROPERTY(BlueprintReadOnly)
		FName Thumbnail;
	UPROPERTY(BlueprintReadOnly)
		int32 Schedule_Value;
	UPROPERTY()
		FName Reset_Schedule;
	UPROPERTY()
		TArray<FName> Unlock_Permission;
	// runtime data
	UPROPERTY(BlueprintReadOnly)
		class URefQuestGroup* _group = nullptr;
	UPROPERTY(BlueprintReadOnly)
		class URefQuest* _prevQuest = nullptr;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefQuest*> _nextQuests;
	UPROPERTY(BlueprintReadOnly)
		TArray<class URefQuestSequence*> _sequences;
	UPROPERTY()
		TArray<class URefQuestEvent*> _acceptEvents;
	UPROPERTY()
		TArray<class URefQuestEvent*> _rewardEvents;
	UPROPERTY()
		TArray<class URefQuestEvent*> _cookieEvents;
	UPROPERTY()
		TArray<URefReward*> _rewards;
	UPROPERTY()
		TArray<URefReward*> _manualRewards;
	UPROPERTY(BlueprintReadOnly)
		EQuestGroupType _groupType = EQuestGroupType::Invalid;
	UPROPERTY()
		TMap<UClass*, URefObject*> _hostObject;
	UPROPERTY(BlueprintReadOnly)
		TSet<class URefTitle*> _titles;
	UPROPERTY()
		TArray<FConditionRule> _acceptConditions;
	UPROPERTY(BlueprintReadOnly)
		TArray<FConditionRule> _rewardConditions;
	UPROPERTY(BlueprintReadOnly)
		TArray<class URefQuestChallenge*> _challenges;
	UPROPERTY(BlueprintReadOnly)
		URefClass* _rewardClass;
	UPROPERTY(BlueprintReadOnly)
		int32 _rewardClassExp = 0;
		
	TMap<EQuestProgress, TArray<FName>> _feeds;
	TSet<URefQuest*> _listenQuests;
	TArray<FName> _descKeys;
	URefSchedule* _resetSchedule = nullptr;

	bool _lastInGroup = false;
	bool _skipCompleteUI = false;

	UFUNCTION(BlueprintPure)
		bool IsResetable() const;
	UFUNCTION(BlueprintPure)
		bool IsRepeatable() const;
	UFUNCTION(BlueprintPure)
		bool IsAccumulatable() const;
	UFUNCTION(BlueprintPure)
		void GetRewards(TArray<FRewardInfo>& output);
	UFUNCTION(BlueprintPure)
		void GetManualRewards(TArray<FRewardInfo>& output);
	UFUNCTION(BlueprintPure, meta = (ComponentClass = "URefObject"), meta = (DeterminesOutputType = "clazz"))
		URefObject* GetHost(TSubclassOf<URefObject> clazz);
	UFUNCTION(BlueprintPure)
		URefObject* GetRepresentativeHost();
	UFUNCTION(BlueprintPure)
		URefQuestChallenge* GetChallengeByName(const FName& rankingName) const;
	UFUNCTION(BlueprintPure)
		bool IsManualRewardArbeit() const;
	UFUNCTION(BlueprintPure)
		TArray<URefQuestSequence*> GetQuestSequences(int32 beginIndex, int32 count);

	virtual void Parse(const FXmlNode* node) override;

	int32 GetMaxAvailableCount() const;
	int32 GetInitialStateIndex() const;
	URefReward* GetMainReward() const;
	URefQuestChallenge* GetChallenge(QuestChallengeRankType rankType) const;

	bool IsInfiniteRepeatable() const;
	bool CheckRemainToComplete(int32 curCompleteCnt) const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefQuestGroup : public URefBase
{
	GENERATED_BODY()

public:
	static TMap<FName, EQuestGroupType> GroupTypes;
	static TSet<EQuestGroupType> CompleteUITypes;
	inline static FName AttendanceGroup{ "Pass" };

	UFUNCTION(BlueprintPure)
		static EQuestGroupType GetQuestGroupType(const FName& group);
	UFUNCTION(BlueprintPure)
		static FName GetQuestGroupTypeName(EQuestGroupType type);

public:
	UPROPERTY(BlueprintReadOnly)
		FName Type;
	UPROPERTY()
		TArray<FName> Reward;
	UPROPERTY()
		TArray<FName> Active_Schedule;
	UPROPERTY()
		FName UserGroup;
	UPROPERTY(BlueprintReadOnly)
		int32 GiveUpCount;
	// runtime data
	TArray<class URefReward*> _rewards;
	TArray<URefSchedule*> _activeSchedules;
	TArray<URefQuestSchedule*> _appendSchedules;
	TOptional<URefSchedule*> _curAppendSchedule;
	EQuestGroupType _type;
	int32 _initialStateIndex = 0;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefQuest*> Quests;
	UPROPERTY(BlueprintReadOnly)
		bool _manualAddType = false;

	virtual void Parse(const FXmlNode* node) override;
	virtual void ParseType(const FXmlNode* node);

	URefSchedule* GetCurAppendSchedule();

	UFUNCTION(BlueprintPure)
		bool UseUserGroup() const;
	UFUNCTION(BlueprintPure)
		void GetRewards(TArray<FRewardInfo>& output);
	UFUNCTION(BlueprintPure)
		URefSchedule* GetActiveSchedule(int32 index = 0) const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefArbeitQuestGroup : public URefQuestGroup
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FString Category;
	UPROPERTY(BlueprintReadOnly)
	EArbeitCategory CategoryType = EArbeitCategory::Invalid;

	virtual void Parse(const FXmlNode* node) override;

	UFUNCTION(BlueprintPure)
		bool IsScheduleAvailable() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefStampTourQuestGroup : public URefQuestGroup
{
	GENERATED_BODY()

public:
	virtual void ParseType(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefPassQuestGroup : public URefQuestGroup
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName MissionGroup_UID;
	UPROPERTY(BlueprintReadOnly)
	FName SubType;
	// runtime
	URefQuestGroup* _missionGroup = nullptr;

	virtual void ParseType(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefQuestSequence : public URefBase
{
	GENERATED_BODY()

public:
	inline static FString String_HostObject{"Host_Object"};
	inline static FName Situation_Event{ "Situation_Event" };
	inline static FName DefaultHostObject{ "objt.pc.basic" };

public:
	UPROPERTY(BlueprintReadOnly)
		int32 Order = 1;
	UPROPERTY()
		FName Quest_UID;
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		FText Desc;
	UPROPERTY(BlueprintReadOnly)
		FName Condition;
	UPROPERTY()
		TArray<FName> Host_Object;
	UPROPERTY()
		TArray<FName> Reward;
	UPROPERTY()
		TArray<FName> Reward_Bonus;

	TArray<int32> Condition_Count;
	// runtime data
	TArray<URefLifeObject*> _activeTargets;
	TArray<URefObject*> _hostObjects;
	TArray<FConditionRule> _completeConditions;
	URefBase* _conditionTarget = nullptr;
	bool _autoRunProgressEvent = false;
	UPROPERTY(BlueprintReadOnly)
		URefQuest* _quest = nullptr;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefQuestEvent*> _progressEvents;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefQuestEvent*> _completeEvents;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefReward*> _reward;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefReward*> _manualRewards;
	UPROPERTY(BlueprintReadOnly)
		URefSchedule* _conditionSchedule = nullptr;
	UPROPERTY()
		TArray<URefQuestEvent*> _situationEvents;
	UPROPERTY(BlueprintReadOnly)
		bool UseExtraBanner;

	virtual void Parse(const FXmlNode* node) override;

	bool UseNotifyWidget() const;
	const FConditionRule* GetRule(const FName& ruleName) const;
	int32 GetRuleCount(const FName& ruleName) const;
	void GetTakeItems(TMap<FName, int32>& output) const;
	FName GetTargetDialog() const;
	FName GetTargetObject() const;
	FName GetTargetQuest() const;
	FName GetTargetCurrency() const;
	FString GetDebugString() const;

	UFUNCTION(BlueprintPure)
		virtual bool IsHelpable() const { return false; }
	UFUNCTION(BlueprintPure)
		int32 GetConditionCount() const;
	UFUNCTION(BlueprintPure)
		FText GetConditionText(FText originText) const;
	UFUNCTION(BlueprintPure)
		FText GetTargetName(EGender gender) const;
	UFUNCTION(BlueprintPure)
		UTexture2D* GetTargetIcon(EGender gender) const;
	UFUNCTION(BlueprintPure)
		bool GetTimeContext(int32& goalSeconds) const;
	UFUNCTION(BlueprintCallable)
		void GetRewards(TArray<FRewardInfo>& output) const;
	UFUNCTION(BlueprintCallable)
		void GetManualRewards(TArray<FRewardInfo>& output) const;
	UFUNCTION(BlueprintPure)
		bool IsConditionTargetItem() const;
	UFUNCTION(BlueprintPure)
		bool IsFirstSequence() const;
	UFUNCTION(BlueprintPure)
		bool IsLastSequence() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefQuestArbeit : public URefQuest
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName ArbeitReward_UID;
	// runtime
	class URefArbeitReward* _arbeitReward = nullptr;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefQuestSequenceArbeit : public URefQuestSequence
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName ArbeitReward_UID;
	// runtime
	class URefArbeitReward* _arbeitReward = nullptr;
	bool _helpable = false;

	virtual void Parse(const FXmlNode* node) override;
	virtual bool IsHelpable() const override { return _helpable; }
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefQuestPass : public URefQuest
{
	GENERATED_BODY()
public:
	static bool IsPointPass(URefQuest* quest);

public:
	UPROPERTY()
	FName ShopCost_Boost;
	UPROPERTY()
	FName MissionGroup_UID;
	UPROPERTY()
	TArray<FName> ShopItem_Reroll;
	UPROPERTY(BlueprintReadOnly)
	FText QuestComment;
	UPROPERTY(BlueprintReadOnly)
	FText InfoPopUpHeader;
	UPROPERTY(BlueprintReadOnly)
	FText InfoPopUpDesc;
	// runtime
	UPROPERTY(BlueprintReadOnly)
	URefQuestGroup* _missionGroup = nullptr;
	UPROPERTY(BlueprintReadOnly)
	bool _premiumOnly = false;	
	UPROPERTY(BlueprintReadOnly)
	TArray<URefShopItem*> _rerollShopItems;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefQuestEvent : public URefBase
{
	GENERATED_BODY()

public:
	inline static FName RegisterShorcutEventType{ "Register_Quest_Shortcut" };

	static FName GetChallengeCeremonyEvtUID(URefQuest* quest);

public:
	UPROPERTY()
		FName Type;
	UPROPERTY()
		FString Type_Value;
	UPROPERTY()
		TArray<URefQuestEvent*> _failEvents;
	
	TArray<FConditionRule> _condition;
	FName Region;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefQuestChallenge : public URefBase
{
	GENERATED_BODY()

public:
	enum class RankingMethod : uint8 {
		Add,
		Submit_Highest,
		Submit_Lowest,
	};

	static QuestChallengeRankType GetRankingType(const FName& rankingName);

public:
	UPROPERTY()
		FName Quest_UID;
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		FText Desc;
	UPROPERTY()
		int32 Order;
	UPROPERTY()
		FName Ranking_Name;
	UPROPERTY()
		FName Ranking_Type;
	UPROPERTY()
		TArray<int32> Ranking_Value;
	UPROPERTY()
		TArray<FName> Upload_Actor;
	UPROPERTY()
		FName Reward;
	UPROPERTY()
		TArray<FName> Ranking_Reward;
	UPROPERTY()
		TArray<FName> Host_Object;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		URefQuest* _quest;
	UPROPERTY(BlueprintReadOnly)
		bool _submitRanking = false;
	UPROPERTY()
		TArray<URefObject*> _hostObjects;
	UPROPERTY(BlueprintReadOnly)
		URefReward* _reward;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefRankingReward*> _rankingRewards;

	QuestChallengeRankType _rankingType = QuestChallengeRankType::Invalid;
	RankingMethod _rankingMethod = RankingMethod::Add;

	virtual void Parse(const FXmlNode* node) override;
	FString GetDebugString() const;
	bool UseSubmitNotify() const;

	UFUNCTION(BlueprintPure)
		FText GetName(EGender gender) const;
	UFUNCTION(BlueprintPure, meta = (ComponentClass = "URefObject"), meta = (DeterminesOutputType = "clazz"))
		URefObject* GetHost(TSubclassOf<URefObject> clazz);
	UFUNCTION(BlueprintPure)
		bool IsMainRanking() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefDialogEvent : public URefBase
{
	GENERATED_BODY()

public:
	inline static FName SystemMessageEvent{ "SystemMessage" };
	inline static FName FormatParamEvent{ "FormatParam" };
	inline static FName RewardSelectionEvent{ "RewardSelection_Icon" };
	inline static FName ViewerJoinEvent{ "Viewer_Join" };

public:
	UPROPERTY(BlueprintReadOnly)
		FName Type;
	UPROPERTY()
		FString Type_Value;
	UPROPERTY(BlueprintReadOnly)
		TArray<FString> _typeValues;
	// runtime
	bool _runtimeBuilt = false;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefDialogSub : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FString Dlg_UID;
	UPROPERTY()
		FName Key;
	UPROPERTY()
		int32 ID;
	UPROPERTY()
		int32 StringTableID;
	UPROPERTY(BlueprintReadOnly)
		TArray<int32> Choice_ID;
	UPROPERTY(BlueprintReadOnly)
		int32 Go_ID;
	UPROPERTY(BlueprintReadOnly)
		FName Character_UID;
	UPROPERTY(BlueprintReadOnly)
		FName Speaker_Pin {	NAME_None };
	UPROPERTY(BlueprintReadOnly)
		TArray<FName> Emotion;
	UPROPERTY()
		FName Emotion_Particle;
	UPROPERTY()
		TArray<FName> Camera;
	UPROPERTY(BlueprintReadOnly)
		FName Override_Icon;
	UPROPERTY(BlueprintReadOnly)
		FName Override_Name;
	UPROPERTY(BlueprintReadOnly)
		FName Style;
	UPROPERTY()
		FName Voice;
	UPROPERTY()
		FName Go_Key;
	UPROPERTY()
		TArray<FName> Choice_Key;

	UFUNCTION(BlueprintPure)
		bool IsChoicable() const;
	UFUNCTION(BlueprintPure)
		bool UseVoice(EGender gender, const FString& voiceLangCode);
	UFUNCTION(BlueprintPure)
		FName GetCamera(EGender gender) const;

	FString GetDebugString() const;
	FString GetTextUID() const;

	FString GetVoiceAssetName(EGender gender, const FString& voiceLangCode) const;
	FString GetVoiceAssetPath(EGender gender, const FString& voiceLangCode) const;
	FString GetVoiceAssetObjectPath(EGender gender, const FString& voiceLangCode) const;
	USoundWave* GetVoiceSound(UObject* outer, EGender gender, const FString& voiceLangCode) const;

	//runtime
	UPROPERTY(BlueprintReadOnly)
		URefDialog* _dlg = nullptr;
	UPROPERTY(BlueprintReadOnly)
		URefCharacter* _character = nullptr;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefDialogEvent*> _events;
	UPROPERTY(BlueprintReadOnly)
		bool _isPCSpeech = false;

	TArray<FConditionRule> _playCondition;
	TArray<FName> _choiceDlgs;
	TOptional<bool> _cachedUseVoice;
	FText _overrideString;
};

UENUM(BlueprintType)
enum class ESkipType : uint8
{
	Selectable,
	Auto,
	Never,
	Auto_Quest,
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefDialog : public URefBase
{
	GENERATED_BODY()
public:
	static FString RootFieldName;
	static FString VoiceAssetPath;
	static ESkipType GetSkipType(const FName& skipTypeStr);
	static void PrintVoiceLog(FString&& log);
	static FString BuildVoiceAssetPath(const FString& voiceLangCode);

public:
	UPROPERTY(BlueprintReadOnly)
		FName Owner_Character_UID;
	UPROPERTY(BlueprintReadOnly)
		FName Trigger_Type;
	UPROPERTY()
		FName Schedule;
	UPROPERTY(BlueprintReadOnly)
		FName BackGround_UID;
	UPROPERTY(BlueprintReadOnly)
		ESkipType SkipType;
	UPROPERTY(BlueprintReadOnly)
		FName Type;
	UPROPERTY()
		FName CameraShake;
	UPROPERTY()
		FName BGM;
	UPROPERTY()
		FName StreamingNPC_Group;
	UPROPERTY()
		FName Donation_Group;
	UPROPERTY()
		bool Use_Auto_Convert_Type = true;
	UPROPERTY()
		bool Use_Streaming_Donation = true;
	UPROPERTY(BlueprintReadOnly)
		FName CaptureCamera_UID;
	// runtime data
	TMap<FName, TMap<FName, FString>> EventArgs; // for export to verify some server events
	TSet<class URefCharacter*> _participants;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefDialogSub*> _speeches;
	UPROPERTY()
		class URefSchedule* _schedule = nullptr;
	UPROPERTY(BlueprintReadOnly)
		URefCharacter* _owner = nullptr;
	UPROPERTY()
		TArray<FConditionRule> _triggerConditions;

	UFUNCTION(BlueprintPure)
		URefDialogSub* GetSpeechByID(int32 id, int32& optionIndex) const;

	virtual void Parse(const FJsonObject* root) override;

	void CacheFlowControlID();

private:
	void PostParse();
	void PostParse_Event();

	static void PostParseEvent_RewardSelection(URefDialog* dlg, URefDialogSub* speech, URefDialogEvent* evt);
	static void PostParseEvent_ViewerJoin(URefDialog* dlg, URefDialogSub* speech, URefDialogEvent* evt);
	static void PostParseEvent_ViewerJoinRandom(URefDialog* dlg, URefDialogSub* speech, URefDialogEvent* evt);
	static void PostParseEvent_ChoiceNextDialog(URefDialog* dlg, URefDialogSub* speech, URefDialogEvent* evt);
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefCustomDetail : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
		TArray<FName> Parts;
	UPROPERTY()
		TArray<FString> CustomValue;
	// runtime
	TArray<uint8> _numValue;

	const uint8 GetIntValue(EGender gender);
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefColor : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY()
		FName ColorPigment_UID;
	UPROPERTY()
		FName ColorPigment_Replacable;
	// runtime
	uint32 _applyColorHex = 0;
	TArray<URefColorPigment*> _pigmentRecipes;
	TArray<URefShopCost*> _pigmentRecipeReplacable;
	UPROPERTY(BlueprintReadOnly)
		FLinearColor _displayLinear;
	UPROPERTY(BlueprintReadOnly)
		FLinearColor _applyLinear;
	UPROPERTY(BlueprintReadOnly)
		URefItemDyeing* _pigment;

	virtual void Parse(const FXmlNode* node) override;

	UFUNCTION(BlueprintPure)
		bool IsDefaultColor() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefColorPigment : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Pigment_UID;
	// runtime
	FDynamicCost _impl;

	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefPalette : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Group_UID;
	UPROPERTY()
		int32 Order;
	UPROPERTY()
		FName Color_UID;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		class URefPaletteGroup* _group = nullptr;
	UPROPERTY(BlueprintReadOnly)
		URefColor* _color = nullptr;

	virtual void Parse(const FXmlNode* node) override;
	FString GetDebugString() const;
	
	UFUNCTION(BlueprintPure, meta = (ComponentClass = "URefBase"), meta = (DeterminesOutputType = "clazz"))
		URefBase* GetCost(TSubclassOf<URefBase> clazz) const;
	UFUNCTION(BlueprintPure)
		UTexture2D* GetCostIcon(EGender gender) const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefPaletteGroup : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY()
		int32 Order;
	UPROPERTY()
		TArray<FName> Parts;
	UPROPERTY()
		FName Schedule;
	UPROPERTY()
		FName Icon;
	UPROPERTY()
		FName Category;
	UPROPERTY()
		FName LocalGroup;
	UPROPERTY()
		FName Display_Type;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		TMap<URefColor*, URefPalette*> _colors;
	UPROPERTY(BlueprintReadOnly)
		URefSchedule* _schedule = nullptr;
	UPROPERTY(BlueprintReadOnly)
		FDynamicCost _dyeingCost;
	UPROPERTY(BlueprintReadOnly)
		FDynamicCost _unlockCost;

	virtual void Parse(const FXmlNode* node) override;
	bool IsDefault() const;

	UFUNCTION(BlueprintPure)
		UTexture2D* GetIconTexture() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefCurve : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
		FString Name;
	UPROPERTY(BlueprintReadWrite)
		float TimeDistance;
	UPROPERTY(BlueprintReadWrite)
		FString Values;
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable)
		void SetValue(FRuntimeFloatCurve curve);
#endif
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FEquipAttributeInfo
{
	GENERATED_USTRUCT_BODY()

public:
	FName Apply_Type;
	FName Apply_UID;
	FName Apply_Effect;
	EStatApplyType Value_Type;

	UPROPERTY(BlueprintReadOnly)
	int32 Value = 0; // StatValue

public:
	void Parse(const FXmlNode* node);
};

UCLASS()
class ANUREFERENCE_API URefEquipAttribute : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Item_UID;
	UPROPERTY()
	FEquipAttributeInfo Attribute;
	UPROPERTY()
	int32 MinValue = 0; // statValue
	UPROPERTY()
	int32 MaxValue = 0;

public:
	UPROPERTY()
	FStatInfoData StatInfoData;

public:
	UFUNCTION(BlueprintPure)
	const FName& GetApplyEffect() const;

public:
	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEquipSetEffect : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FName SetGroup;
	UPROPERTY(BlueprintReadOnly)
	uint8 EquipCount;
	UPROPERTY(BlueprintReadOnly)
	FEquipAttributeInfo Attribute;
	// runtime
	UPROPERTY(BlueprintReadOnly)
	FStatInfoData StatInfoData;
	UPROPERTY(BlueprintReadOnly)
	URefTag* Tag;

public:
	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEquipUpgrade : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Parts;
	UPROPERTY()
	int32 Grade = 0;
	UPROPERTY()
	int32 Upgrade_Count = 0;
	UPROPERTY()
	FName Material_Item_UID;
	UPROPERTY(BlueprintReadOnly)
	int32 Material_Item_Count = 0;
	UPROPERTY()
	FName Currency_UID;
	UPROPERTY(BlueprintReadOnly)
	int32 Currency_Count = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 Prob = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 HeavyFailProb = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 Transcendence_Condition = 0;

public:
	// runtime data
	UPROPERTY(BlueprintReadOnly)
	class URefItem* _materialItem = nullptr;
	UPROPERTY(BlueprintReadOnly)
	class URefCurrency* _costCurrency = nullptr;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEquipUpgradeEffect : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Item_UID;
	UPROPERTY()
	int32 Upgrade;
	UPROPERTY()
	TArray<FStatInfoData> _stats;

public:
	virtual void Parse(const FXmlNode* node) override;

	int64 GetStatValue(EStatTypes statType) const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEquipCarveEffect : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
		FName Stat_UID;
	UPROPERTY()
		int32 Prob;
	UPROPERTY()
		int32 Stat_Value_Min;
	UPROPERTY()
		int32 Stat_Value_Max;
	UPROPERTY()
		int32 Value_Unit;
	// runtime
	TArray<TArray<FString>> Target_TID;
	TArray<TypeID> _targetTID;
	URefStat* _stat = nullptr;

	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEquipMolding : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
		FName ItemEquip_UID;
	UPROPERTY()
		FName Currency_UID;
	UPROPERTY(BlueprintReadOnly)
		int32 Currency_Value;
	UPROPERTY()
		TArray<FName> Material_Item_UID;
	UPROPERTY()
		TArray<int32> Material_Item_Count;

public: //runtime
	UPROPERTY(BlueprintReadOnly)
		URefItemEquip* _itemEquip = nullptr;
	UPROPERTY(BlueprintReadOnly)
		URefCurrency* _currency = nullptr;

	TArray<TPair<URefItem*, int32>> _materials;

	UFUNCTION(BlueprintCallable)
		void GetMaterials(TArray<URefItem*>& outItems, TArray<int32>& outCounts) const;

public:
	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEquipCraft : public URefEquipMolding
{
	GENERATED_BODY()

};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEquipReforge : public URefEquipMolding
{
	GENERATED_BODY()

};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FPeriodCondition
{
	GENERATED_USTRUCT_BODY()

public:
	enum class Type : uint8 {
		None,
		Daily,
		Weekly,
		Weekly_Time,
		Monthly,
		Period,
		Period_Yearly,
		Duration
	};
	static Type GetPeriodType(const FName& periodType);

	static uint8 GetDayOfWeekInTmRule(EDayOfWeek dayOfWeek);
	static FDateTime GetNextDateWithTime(const FDateTime& srcDate, const tm& time);
	static FDateTime GetNextDateWithDayOfWeek(const FDateTime& srcDate, const tm& dayofWeekAndTime);
	static FDateTime GetNextDateWithDayOfMonth(const FDateTime& srcDate, const tm& dayOfMonthAndTime);

	static FDateTime GetLastDateWithTime(const FDateTime& srcDate, const tm& time);
	static FDateTime GetLastDateWithDayOfWeek(const FDateTime& srcDate, const tm& time);
	static FDateTime GetLastDateWithDayOfMonth(const FDateTime& srcDate, const tm& time);

	Type PeriodType = Type::None;
	tm PeriodValue_From { 0, };
	tm PeriodValue_To { 0, };
	FTimespan PeriodDeltaTime;
	TArray<int32> NumberArgs;

	bool Parse(const FName& Period_Type, const FString& Period_Value);

	bool StringToTime(const FString& hhmmss, tm& output);
	void StringToDayOfWeek(const FString& MONtoSUN, tm& output);
	bool StringToDate(const FString& yyyymmddhhmmss, tm& output);

	FDateTime GetNextStartDate(bool useLocalTime) const;
	FDateTime GetNextStartDate(const FDateTime& targetTime, bool useLocalTime) const;
	FDateTime GetNextEndDate(bool useLocalTime) const;
	FDateTime GetNextEndDate(const FDateTime& targetTime, bool useLocalTime) const;
	FDateTime GetLastStartDate(bool useLocalTime) const;
	FDateTime GetLastStartDate(const FDateTime& targetTime, bool useLocalTime) const;
	int32 GetDurationInSeconds(bool useLocalTime) const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefShopGroup : public URefBase
{
	GENERATED_BODY()

public:
	inline static FName CustomizeType{ "Customize" };
	inline static FName FashionType{"Fashion"};
	inline static FName MarketType{ "Market" };
	inline static FName GachaType{ "Gacha" };
	inline static FName MagicShopType{ "MagicShop" };

public:
	UPROPERTY(BlueprintReadOnly)
		FName Type;
	UPROPERTY(BlueprintReadOnly)
		FName SubType;
	UPROPERTY(BlueprintReadOnly)
		int32 Order = 0;
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		FText Desc;
	UPROPERTY()
		FName LocalGroup;
	UPROPERTY(BlueprintReadOnly)
		FText Shorten_Name;
	UPROPERTY()
		FName Bonus_Reward;

public:
	// runtime
	bool _isPostType = false;
	bool _isRuntimeBuildShop = false;
	URefReward* _bonusReward;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		URefNPC* _ownerNPC = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		TArray<URefShopItem*> _packages;
	
	virtual void Parse(const FXmlNode* node) override;

	bool IsPaymentType() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefShopItem : public URefBase
{
	GENERATED_BODY()

public:
	inline static FName SubscriptionType{ "Subscription" };
	inline static FName AdvertisementType{ "Advertisement" };

public:
	UPROPERTY()
	FName ShopGroup;
	UPROPERTY()
	int32 Order = 0;
	UPROPERTY(BlueprintReadOnly)
	FName Target_Type;
	UPROPERTY()
	FName Target_UID;
	UPROPERTY(BlueprintReadOnly)
	int32 Amount = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 Purchase_Limit_Count = 0;
	UPROPERTY()
	FString Purchase_Reset_Schedule;
	UPROPERTY()
	int32 Purchase_CoolTime;
	UPROPERTY(BlueprintReadOnly)
	int32 Pre_Discount_Value;
	UPROPERTY()
	TArray<FName> Display_Tag;
	UPROPERTY()
	FString Schedule;
	UPROPERTY(BlueprintReadOnly)
	FName Category;
	UPROPERTY(BlueprintReadOnly)
	int32 Activate = 1;
	UPROPERTY()
	FString Resource_UID;
	UPROPERTY()
	FText Post_Name;
	UPROPERTY()
	FText Post_Desc;
	UPROPERTY()
	FName Post_Icon;
	UPROPERTY(BlueprintReadOnly)
	TArray<FConditionRule> Conditions;
	UPROPERTY(BlueprintReadOnly)
	TArray<FText> ConditionTexts;
	UPROPERTY(BlueprintReadOnly)
	FText Warn_Text;
	UPROPERTY(BlueprintReadOnly)
	FText Discount_Text;
	UPROPERTY(BlueprintReadOnly)
	FName Banner_Style;

public:
	TArray<FString> ApplyTags;
	// runtime
	TArray<FDynamicCost> _cost;
	TArray<URefTag*> _displayTags;
	TMap<URefTag*, int32> _applyTags;
	TSubclassOf<URefBase> _type;
	bool _defaultAvailable = true;
	bool _registeredInApp = false;
	TSet<URefReward*> UnlockRewards;
	TArray<FConditionRule> _warnConditions;

public:
	UPROPERTY(BlueprintReadOnly)
	URefShopGroup* _shopGroup = nullptr;
	UPROPERTY(BlueprintReadOnly)
	URefSchedule* _schedule = nullptr;
	UPROPERTY(BlueprintReadOnly)
	URefSchedule* _purchaseResetSchedule = nullptr;
	UPROPERTY(BlueprintReadOnly)
	URefBase* _target = nullptr;
	UPROPERTY(BlueprintReadOnly)
	URefShopItem* _paymentTarget;
	UPROPERTY(BlueprintReadOnly)
	bool _isPrizeItem;

public:
	// target
	UFUNCTION(BlueprintPure, meta = (ComponentClass = "URefBase"), meta = (DeterminesOutputType = "clazz"))
	URefBase* GetTarget(TSubclassOf<URefBase> clazz) const;
	UFUNCTION(BlueprintPure)
	FText GetName(EGender gender);
	UFUNCTION(BlueprintPure)
	FText GetDescription(EGender gender);
	UFUNCTION(BlueprintPure)
	const FName& GetTargetUID() const;
	UFUNCTION(BlueprintPure)
	int32 GetTargetGuid() const;
	UFUNCTION(BlueprintPure)
	UTexture2D* GetIcon(EGender gender);
	UFUNCTION(BlueprintPure)
	FName GetDisplayTagIcon();
	UFUNCTION(BlueprintPure)
	URefItem* GetDyeingIconTarget(EGender gender);
	UFUNCTION(BlueprintPure)
	void GetTargetDisplayInfos(EGender gender, TArray<FText>& outNames, TArray<int32>& outAmounts, TArray<UTexture2D*>& outIcons);
	UFUNCTION(BlueprintPure)
	URefTag* GetFirstDisplayTag() const;
	UFUNCTION(BlueprintPure)
	int32 GetGettableCount();
	UFUNCTION(BlueprintPure)
	URefTag* FindDisplayTag(const FName& tagUID) const;
	UFUNCTION(BlueprintPure)
	bool IsPaymentType() const;
	UFUNCTION(BlueprintPure)
	bool IsPostType() const;
	UFUNCTION(BlueprintPure)
	bool IsSubscriptionType() const;
	UFUNCTION(BlueprintPure)
	bool IsAdvertisementType() const;

	// cost
	UFUNCTION(BlueprintPure, meta = (ComponentClass = "URefBase"), meta = (DeterminesOutputType = "clazz"))
	URefBase* GetCost(TSubclassOf<URefBase> clazz) const;
	UFUNCTION(BlueprintPure)
	URefBase* GetCostTarget() const;
	UFUNCTION(BlueprintPure)
	FName GetCostType() const;
	UFUNCTION(BlueprintPure)
	FName GetCostUID() const;
	UFUNCTION(BlueprintPure)
	int32 GetCostGuid() const;
	UFUNCTION(BlueprintPure)
	UTexture2D* GetCostIcon(EGender gender) const;
	UFUNCTION(BlueprintPure)
	const FText& GetCostName(EGender gender) const;
	UFUNCTION(BlueprintPure)
	void GetCostItems(TMap<URefItem*, int32>& output) const;
	UFUNCTION(BlueprintPure)
	void GetCostCurrency(TMap<URefCurrency*, int32>& output) const;

	template<typename T>
	T* GetTargetImpl() const {
		return Cast<T>(_target);
	}

	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefShopItemInApp : public URefShopItem
{
	GENERATED_BODY()

public:
	// runtime
	TSet<URefShopItem*> _drivingShopItems;

	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefShopItemRandom : public URefShopItem
{
	GENERATED_BODY()

public:
	UPROPERTY()
		FName ShopPool_UID;
	// runtime
	class URefShopPool* _pool = nullptr;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefShopCost : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Cost_UID;
	UPROPERTY()
		TArray<FName> Replace_ShopCost;
	// runtime
	FDynamicCost _impl;
	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefShopPool : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Schedule;
	// runtime
	class URefSchedule* _schedule = nullptr;
};

UENUM(BlueprintType)
enum class EAnuTimeZoneRule : uint8
{
	Utc0,
	ServerLocal,
	ClientLocal,
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSchedule : public URefBase
{
	GENERATED_BODY()

public:
	// static
	inline static int64 ServerLocalUtcDelta = 0;
	inline static int64 ClientLocalUtcDelta = 0;
	static int32 ServerLocalTimeZone;

	static bool IsAvailable(const FPeriodCondition& condition, const FDateTime& curDate, bool useLocalTime);
	UFUNCTION(BlueprintPure)
	static bool AnyAvailableSchedule(const TArray<URefSchedule*>& schedules);
	UFUNCTION(BlueprintPure)
	static FDateTime GetServerNow();
	UFUNCTION(BlueprintPure)
	static FDateTime GetAnuNow(EAnuTimeZoneRule tzRule);

	// local: client, server-local, utc
	UFUNCTION(BlueprintPure)
	static FDateTime ConvertServerLocalToUtc(const FDateTime& serverLocalTime);
	UFUNCTION(BlueprintPure)
	static FDateTime ConvertServerTimeToLocalUTCTime(const FDateTime& serverTime);
	UFUNCTION(BlueprintPure)
	static FDateTime ConvertServerTimeToLocalTime(const FDateTime& serverTime);
	UFUNCTION(BlueprintPure)
	static FDateTime ConvertUtcToServerLocal(const FDateTime& utcTime);
	UFUNCTION(BlueprintPure)
	static FDateTime ConvertLocalToUtc(const FDateTime& clientLocalTime);
	UFUNCTION(BlueprintPure)
	static FDateTime ConvertUtcToLocal(const FDateTime& utcTime);

public:
	// non-static
	UPROPERTY()
	FName Period_Type;
	UPROPERTY()
	FString Period_Value;
	UPROPERTY()
	FPeriodCondition PeriodCondition;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY()
	FText Desc;
	UPROPERTY()
	FName Icon;
	UPROPERTY(BlueprintReadOnly)
	bool Use_Local_Time = false;

public:	// runtime
	bool _isDaySchedule = false;

public:
	UFUNCTION(BlueprintPure)
		FDateTime GetNow() const;
	UFUNCTION(BlueprintPure)
		FDateTime GetLastStartDate();
	UFUNCTION(BlueprintPure)
		FDateTime GetNextStartDate();
	UFUNCTION(BlueprintPure)
		FDateTime GetNextEndDate();
	UFUNCTION(BlueprintPure)
		float GetDurationSeconds() const;
	UFUNCTION(BlueprintPure)
		bool IsAvailable() const;
	UFUNCTION(BlueprintPure)
		bool IsExpired();

	virtual void Parse(const FXmlNode* node) override;

	FDateTime GetLastStartDateWith(const FDateTime& targetTime);
	FDateTime GetNextStartDateWith(const FDateTime& targetTime);
	FDateTime GetNextEndDateWith(const FDateTime& targetTime);
	FDateTime GetNextStartDateInUtc();
	FDateTime GetNextEndDateInUtc();

	bool IsToday(const FDateTime& now);
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefQuestSchedule : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
		FName QuestGroup_UID;
	UPROPERTY()
		URefSchedule* _schedule;

	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefPostTemplate : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		FText Desc;
	UPROPERTY(BlueprintReadOnly)
		FName Icon;
	UPROPERTY(BlueprintReadOnly)
		FName Open_Widget_Type;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		TSoftObjectPtr<UTexture2D> _iconTexture;
	UPROPERTY(BlueprintReadOnly)
		FString _iconRoute;

	UFUNCTION(BlueprintPure)
	UTexture2D* GetIconTexture() const
	{
		return _iconTexture.IsNull() == false ? _iconTexture.LoadSynchronous() : nullptr;
	}
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefNPCProfile : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
		FText Msg;
	UPROPERTY()
		FName NPC_UID;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefTitle : public URefBase
{
	GENERATED_BODY()

public:
	// static
	static TMap<FName, TSet<URefQuest*>> KEYWORDS_BY_CATEGORY;

	UFUNCTION(BlueprintPure)
		static void GetKeywordCategory(TArray<FName>& category);
	UFUNCTION(BlueprintPure)
		static void GetKeywordByCategory(FName category, TSet<URefQuest*>& keywords);

	// non-static
	TMap<EStatTypes, TMap<EStatApplyType, FStatInfoData>> Stat;

	UPROPERTY(BlueprintReadOnly)
		int32 Activate;
	UPROPERTY(BlueprintReadOnly)
		int32 Order;
	UPROPERTY(BlueprintReadOnly)
		FText Desc;
	UPROPERTY(BlueprintReadOnly)
		FString Color;
	UPROPERTY(BlueprintReadOnly)
		FName Display_Type;
	UPROPERTY(BlueprintReadOnly)
		FName Display_Value;
	UPROPERTY()
		TArray<FString> Value_Index;
	UPROPERTY()
		TArray<FString> Value;
	UPROPERTY()
		TArray<FName> Keywords;	
	UPROPERTY()
		int32 Show_List = 1;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		TArray<URefQuest*> _keywords;
protected:
	UPROPERTY(BlueprintReadOnly)
		FText Name;
public:
	virtual void Parse(const FXmlNode* node) override;
	const FText& GetRawName() { return Name; }

	UFUNCTION(BlueprintCallable)
		void GetStatInfoDatas(TArray<FStatInfoData>& output);
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefReply : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
		FName Reply_UID;
	UPROPERTY()
		int32 Order;
	UPROPERTY()
		FName Host;
	UPROPERTY()
		FName Override_Name;
	UPROPERTY()
		FName Override_Icon;
	UPROPERTY(BlueprintReadOnly)
		FName Sticker_UID;
	UPROPERTY(BlueprintReadOnly)
		FText String;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		URefObject* _host = nullptr;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefReaction : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
		FName Type;
	UPROPERTY(BlueprintReadOnly)
		FName Value;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefStreaming : public URefBase
{
	GENERATED_BODY()

public:
	static FName NAME_Streaming;
	inline static FName Type_Discovery{ "Discovery" };

public:
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		FText Desc;
	UPROPERTY(BlueprintReadOnly)
		FName Icon;
	UPROPERTY(BlueprintReadOnly)
		FName Finder_Text;
	UPROPERTY(BlueprintReadOnly)
		FName Type;
	UPROPERTY(BlueprintReadOnly)
		FName Widget_Type;
	UPROPERTY()
		FString Type_Value;
	UPROPERTY()
		int32 Order = 0;
	UPROPERTY()
		FName Schedule;
	UPROPERTY()
		int32 Interaction_Count = 1;
	UPROPERTY()
		int32 Cool_Time = 0;
	UPROPERTY()
		int32 Repeatable = 1;
	UPROPERTY()
		TArray<FName> Display_Tag;
	UPROPERTY(BlueprintReadOnly)
		FString Fail_Message;
	// runtime
	URefSchedule* _schedule = nullptr;
	TArray<FConditionRule> _openCondition;
	TArray<URefTag*> _displayTags;
	UPROPERTY(BlueprintReadOnly)
		TArray<URefQuest*> _listenQuests;

	virtual void Parse(const FXmlNode* node) override;
	virtual bool CheckShowScore() const;

	FName GetConditionQuestUID() const;
	bool UsePredict() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefInspecting : public URefStreaming
{
	GENERATED_BODY()

public:
	static FName TypeName;

public:
	UPROPERTY()
		int32 Show_List;
	UPROPERTY()
		FName Accept_Dialog;
	UPROPERTY()
		FString Helper_UID;
	UPROPERTY()
		FString Toast_Message;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		URefDialog* _acceptDlg = nullptr;

	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefTakePhoto : public URefStreaming
{
	GENERATED_BODY()

public:
	static FName TypeName;

	UPROPERTY(BlueprintReadOnly)
		int32 Enable_Retake;

public:
	virtual void Parse(const FXmlNode* node) override;
	virtual bool CheckShowScore() const override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefInterview : public URefStreaming
{
	GENERATED_BODY()

public:
	static FName TypeName;

public:
	UPROPERTY()
		int32 Show_List;
	UPROPERTY()
		FName Pin;
	UPROPERTY()
		FString Toast_Message;
	UPROPERTY()
		FName Accept_Dialog;
	UPROPERTY()
		FString Helper_UID;
	UPROPERTY(BlueprintReadOnly)
		FText Title_Main;
	UPROPERTY(BlueprintReadOnly)
		FText Title_Sub;
	// runtime
	URefDialog* _dlg = nullptr;
	UPROPERTY(BlueprintReadOnly)
		URefCharacter* _target = nullptr;
	UPROPERTY(BlueprintReadOnly)
		URefDialog* _acceptDlg = nullptr;

	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItemReview : public URefBase
{
	GENERATED_BODY()

public:
	inline static FName NAME_ItemReview{ "ItemReview" };

public:
	UPROPERTY(BlueprintReadOnly)
		FName Icon;
	UPROPERTY(BlueprintReadOnly)
		FName ViewType;
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		FText Desc;
	UPROPERTY()
		FName Pin;
	UPROPERTY()
		FString Dialog;
	UPROPERTY(BlueprintReadOnly)
		TArray<FText> Summary;
	UPROPERTY()
		TArray<FName> Display_Tag;
	// runtime
	URefDialog* _dlg = nullptr;
	TArray<URefTag*> _displayTags;
	UPROPERTY(BlueprintReadOnly)
		URefItem* _item = nullptr;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefItemTrade : public URefBase
{
	GENERATED_BODY()

public:
	inline static FName NAME_ItemTrade{ "ItemTrade" };
	inline static FName AutomationTrigger_Enter{ "ItemTrade_Enter" };
	inline static FName AutomationTrigger_Apeal{ "ItemTrade_Apeal" };
	inline static FName AutomationTrigger_Success{ "ItemTrade_Success" };
	inline static FName AutomationTrigger_Fail{ "ItemTrade_Fail" };

public:
	UPROPERTY(BlueprintReadOnly)
		FName Icon;
	UPROPERTY(BlueprintReadOnly)
		FName ViewType;
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		FText Desc;
	UPROPERTY()
		TArray<FName> Dialog;
	UPROPERTY()
		TArray<FName> Reward;
	UPROPERTY()
		int32 Initial_Bundle = 1;
	UPROPERTY()
		int32 Trade_Bundle = 1;
	UPROPERTY()
		int32 Repeat = 0;
	UPROPERTY()
		TArray<FName> Display_Tag;
	// runtime
	TArray<URefDialog*> _dlgs;
	TArray<URefReward*> _rewards;
	TArray<URefTag*> _displayTags;
	UPROPERTY(BlueprintReadOnly)
		URefItem* _item = nullptr;

	URefDialog* GetDialog(int32 tradeCount) const;
	URefReward* GetReward(int32 tradeCount) const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefRankingReward : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Range_Type;
	UPROPERTY()
	FName Reward;
	UPROPERTY(BlueprintReadOnly)
	FName Ranking_Name;
	UPROPERTY(BlueprintReadOnly)
	int32 Level;

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> _rangeValues;
	UPROPERTY(BlueprintReadOnly)
	URefReward* _reward;
	
	virtual void Parse(const FXmlNode* node) override;
};

UENUM()
enum class EUserInteractionAddType : uint8 {
	Default,
	Reward,
	System,
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefUserInteraction : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
		int32 Priority;
	UPROPERTY(BlueprintReadOnly)
		int32 Requester_Reward_Limit;
	UPROPERTY(BlueprintReadOnly)
		int32 Accepter_Reward_Limit;
	UPROPERTY(BlueprintReadOnly)
		int32 Requester_Limit_Count;
	UPROPERTY(BlueprintReadOnly)
		int32 Accepter_Limit_Count;
	UPROPERTY()
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		FName Icon;
	UPROPERTY(BlueprintReadOnly)
		FName Attach_Widget {NAME_None};
	UPROPERTY(BlueprintReadOnly)
		FName Requester_Animation;
	UPROPERTY(BlueprintReadOnly)
		FName Accepter_Animation;
	UPROPERTY(BlueprintReadOnly)
		FName Requester_Effect;
	UPROPERTY(BlueprintReadOnly)
		FName Accepter_Effect;
	UPROPERTY()
		EUserInteractionAddType Add_Type;
	UPROPERTY(BlueprintReadOnly)
		FName Toast_Info;
	UPROPERTY()
		int32 View_Tag_Limit;
	UPROPERTY()
		float Cool_Time = 0.f;
	UPROPERTY()
		float Requester_Count_Reset_Cool_Time = 0.f;
	UPROPERTY()
		FName Reset_Schedule;
	// runtime
	URefSchedule* _resetSchedule = nullptr;
	UPROPERTY(BlueprintReadOnly)
		TSoftObjectPtr<UTexture2D> _iconTexture;

	UFUNCTION(BlueprintPure)
	UTexture2D* GetIconTexture() {
		return _iconTexture.IsNull() == false ? _iconTexture.LoadSynchronous() : nullptr;
	}

	virtual void Parse(const FXmlNode* node) override;
};

UENUM(BlueprintType)
enum class ESkillPatronageTarget : uint8
{
	PC,
	Mob
};

inline FString ToString(ESkillPatronageTarget value)
{
	switch (value) {
	case ESkillPatronageTarget::PC:
		return TEXT("PC");
	case ESkillPatronageTarget::Mob:
		return TEXT("Mob");
	default:
		return TEXT("Invalid");
	}
}

inline bool TryParse(FName inName, ESkillPatronageTarget& outValue)
{
	static FName Name_PC = TEXT("PC");
	static FName Name_Mob = TEXT("Mob");

	if (inName == Name_PC) {
		outValue = ESkillPatronageTarget::PC;
		return true;
	}
	else if (inName == Name_Mob) {
		outValue = ESkillPatronageTarget::Mob;
		return true;
	}
	return false;
}

class URefSkillPatronageTier;

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkillPatronage : public URefBase
{
	GENERATED_BODY()
	friend class UReferenceBuilder;

public:
	UPROPERTY(BlueprintReadOnly)
	FName SkillTimelineUID;
	UPROPERTY(BlueprintReadOnly)
	FName Name;
	UPROPERTY(BlueprintReadOnly)
	FName Desc;
	UPROPERTY(BlueprintReadOnly)
	FName Icon;
	UPROPERTY(BlueprintReadOnly)
	FName Target;
	UPROPERTY(BlueprintReadOnly)
	int32 Rate;
	UPROPERTY(BlueprintReadOnly)
	int32 Tier;

	UFUNCTION(BlueprintPure)
	UTexture2D* GetIcon() const { return _icon.LoadSynchronous(); }
	UFUNCTION(BlueprintGetter)
	FText GetText() const { return _text; }
	UFUNCTION(BlueprintPure)
	TArray<URefSkillTimeline*> GetSkillTimeline() const { return _timelines; }
	UFUNCTION(BlueprintPure)
	ESkillPatronageTarget GetSkillTarget() const { return _target; }
	UFUNCTION(BlueprintPure)
	URefSkillPatronageTier* GetTier() const { return _tier; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, BlueprintGetter=GetText, meta=(AllowPrivateAccess="true"))
	FText _text;
	TSoftObjectPtr<UTexture2D> _icon;
	TArray<URefSkillTimeline*> _timelines;
	ESkillPatronageTarget _target;
	URefSkillPatronageTier* _tier;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkillPatronageTier : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Target;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Tier;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Pool;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName UI_Color;

	virtual void Parse(const FXmlNode* node) override;

	UFUNCTION(BlueprintPure)
	ESkillPatronageTarget GetTarget();
	UFUNCTION(BlueprintPure)
	FLinearColor GetUIColor();

private:
	ESkillPatronageTarget _target;
	FLinearColor _uicolor;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkillPatronageCost : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Level;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Cost_PC;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Cost_Mob;
};

UENUM(BlueprintType)
enum class EClientType :uint8 {
	Invalid = 0,
	Test = 2,
	Stable_Dev = 3,
	LiveQA = 6,
	QA = 7,
	Review = 8,
	Production = 9,
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefFashionContentsScore : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Category;
	UPROPERTY()
	int32 Add_Score;
	UPROPERTY()
	int32 Minus_Score;
	UPROPERTY()
	FName TargetUID;

public:
	class URefTag* _tag = nullptr;
	class URefTagGroup* _tagGroup = nullptr;

public:
	bool IsValidTag(URefTag* tag) const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefFashionContentsFactor : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString Value;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API UMultiDonation : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	int32 Index = 0;
	UPROPERTY(BlueprintReadOnly)
	FName Key = NAME_None;
	UPROPERTY(BlueprintReadOnly)
	int32 Amount = 0;
	UPROPERTY()
	int32 Score = 0;

public:
	void Init(const int32& index, TArray<FString>& output);
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefFashionContentsGroup : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Type;
	UPROPERTY(BlueprintReadOnly)
	EClassLicense Unlock_LicenseType;
	UPROPERTY()
	TArray<FName> Schedule;
	UPROPERTY()
	TArray<UMultiDonation*> Donations;

public:
	UPROPERTY()
	TArray<URefSchedule*> _refSchedule;
	UPROPERTY()
	UMultiDonation* _specialDonation = nullptr;
	UPROPERTY()
	TArray<class URefFashionContentsStage*> _stages;

public:
	virtual void Parse(const FXmlNode* node) override;
	bool IsTutorial() const;
	URefSchedule* GetSeasonSchedule() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefFashionContentsStage : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Group_UID;
	UPROPERTY()
	FName Stage_UID;
	UPROPERTY()
	TArray<FName> Add_Score_UID;
	UPROPERTY()
	TArray<FName> Minus_Score_UID;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY()
	TArray<FString> ClassExp;

	UPROPERTY()
	URefClass* rewardClass;
	int32 rewardClassExp = 0;

public:
	virtual void Parse(const FXmlNode* node) override;
	static void ParseScore(const FXmlNode* node, const FString& columName, TArray<FName>& output);
	bool IsTutorial() const;

public:
	UPROPERTY(BlueprintReadOnly)
	URefStageInfo* _refStageInfo = nullptr;
	UPROPERTY(BlueprintReadOnly)
	URefFashionContentsGroup* _refGroup;
	TArray<URefFashionContentsScore*> _addScores;
	TArray<URefFashionContentsScore*> _minusScores;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefFashionContentsNPC : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName NPC_UID;
	UPROPERTY()
	FName Condition;
	UPROPERTY()
	TArray<FName> ConditionValue;
	UPROPERTY()
	int32 ScoreValue = 0;
	UPROPERTY()
	TArray<FName> Add_Score_UID;
	UPROPERTY()
	TArray<FName> Minus_Score_UID;
	UPROPERTY()
	TArray<FName> MotionSection;

public:
	virtual void Parse(const FXmlNode* node) override;
	std::pair<uint16, float> GetScore(URefTag* tag) const;

public:
	TArray<URefFashionContentsScore*> _addScores;
	TArray<URefFashionContentsScore*> _minusScores;
	TArray<URefFashionContentsScore*> _likeTag;
	TArray<URefFashionContentsScore*> _hateTag;

	UPROPERTY(BlueprintReadOnly)
	URefNPC* _refNPC = nullptr;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefTagGroup : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
		FName Category;
	UPROPERTY()
		FName Icon;
	UPROPERTY()
		int32 Order;
	UPROPERTY()
		int32 Enable_Label;
	UPROPERTY(BlueprintReadOnly)
		FText Name;
	UPROPERTY(BlueprintReadOnly)
		int32 Max_Selection;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		TArray<class URefTag*> _tags;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefTag : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FName Group_UID;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	int32 Order = 0;
	UPROPERTY(BlueprintReadOnly)
	FString Color;
	UPROPERTY(BlueprintReadOnly)
	FName Icon;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY(BlueprintReadOnly)
	bool Enable_Display{ false };
	UPROPERTY(BlueprintReadOnly)
	FName Motion;
	UPROPERTY(BlueprintReadOnly)
	FName ImageUUID;
	UPROPERTY()
	FString Schedule;
	UPROPERTY()
	FName Effect_UID;
	// runtime
	UPROPERTY(BlueprintReadOnly)
	URefTagGroup* _group = nullptr;
	UPROPERTY(BlueprintReadOnly)
	URefSchedule* _schedule = nullptr;
};

UCLASS()
class ANUREFERENCE_API URefContentsSrc : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Key;
	UPROPERTY()
		TArray<URefQuestEvent*> Events;
};

UCLASS()
class ANUREFERENCE_API URefStreamingNPC : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Group;
	UPROPERTY()
		int32 Allow_Duplicates;
	UPROPERTY()
		int32 Enter_Prob;
	UPROPERTY()
		FName Style;

	TArray<FConditionRule> _condition;
	URefCharacter* _npc = nullptr;

	virtual void Parse(const FXmlNode* node) override;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FEquipCollectionStatReward
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	uint8 Count = 0;
	UPROPERTY(BlueprintReadOnly)
	EStatTypes ApplyStat;
	UPROPERTY(BlueprintReadOnly)
	int32 ApplyValue = 0;

	static FEquipCollectionStatReward Parse(const FString& value);
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEquipCollection : public URefBase
{
	GENERATED_BODY()

public:
	static inline TMap<uint32, TSet<URefEquipCollection*>> ByEquipGuids;
	static void GetCollectionsByItem(URefItem* item, TSet<URefEquipCollection*>& output);

public:
	UPROPERTY()
		FName Item_UID;
	UPROPERTY()
		FName Group_UID;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		URefItemEquip* _equipItem = nullptr;
	UPROPERTY()
		class URefEquipCollectionGroup* _group = nullptr;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEquipCollectionGroup : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	TArray<FEquipCollectionStatReward> Reward_Stat;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY(BlueprintReadOnly)
	FName Desc;
	UPROPERTY(BlueprintReadOnly)
	FName Category;
	UPROPERTY()
	FName Reward_UID;
	
public:
	virtual void Parse(const FXmlNode* node) override;
	
public: // runtime
	UPROPERTY(BlueprintReadOnly)
	TArray<URefEquipCollection*> _collections;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefStat : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	EStatTypes Type;
	UPROPERTY()
	FText Name;
	UPROPERTY()
	FText Desc;
	UPROPERTY()
	FString Icon;
	UPROPERTY()
	int32 Order;
	UPROPERTY()
	FString Color;
	UPROPERTY()
	TArray<FName> Group;
	UPROPERTY()
	TArray<FName> SpecificTargets;
	UPROPERTY(BlueprintReadOnly)
	int32 MaxLimit;
	UPROPERTY(BlueprintReadOnly)
	EStatApplyType Display_ApplyType;
	UPROPERTY(BlueprintReadOnly)
	int32 BattlePower_Conversion_Factor;

public:
	// runtime
	TArray<URefSkillTimeline*> _targetSkills;
	TArray<URefSkillTreeSlot*> _targetTreeSlots;

	virtual void Parse(const FXmlNode* node) override;
	bool HasTarget(const FName& target) const;
};

UENUM(BlueprintType)
enum class ERankingType : uint8
{
	None,
	Popularity,
	Fashionshow,
	Championship,
	Donation,
	Qualifier,
	QuestChallenge,
	End
};

UENUM(BlueprintType)
enum class EPeriodicType : uint8
{
	None,
	Periodic,
	Persistent,
};

UENUM(BlueprintType)
enum class EOverwriteMethod : uint8
{
	None,
	Greater,
	Less,
	GreaterEquals,
	LessEquals,
	Accum
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefRanking : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	ERankingType RankType;
	UPROPERTY()
	EPeriodicType PeriodicType;
	UPROPERTY()
	FName ResetSchedule;
	UPROPERTY()
	EOverwriteMethod OverwriteMethod;
	UPROPERTY()
	int32 TopRankers;
	UPROPERTY()
	int32 TotalRankers;
	UPROPERTY()
	FName RankingRewardName;
	UPROPERTY()
	TArray<FName> TopRankerRewardName;

	virtual void Parse(const FXmlNode* node) override;

	// runtime
	URefSchedule* _resetSchedule = nullptr;
	TArray<URefRankingReward*> _rewards;
	TArray<URefRankingReward*> _topRankerCommonRewards;
	TMap<int32, URefRankingReward*> _topRankerEachRewards;

private:
	inline static TMap<TPair<ERankingType, EPeriodicType>, URefRanking*> _values;

public:
	static URefRanking* GetReferenceByType(ERankingType ranking, EPeriodicType periodic)
	{
		URefRanking** pptr = _values.Find(TPairInitializer(ranking, periodic));
		return pptr ? *pptr : nullptr;
	}

	URefRankingReward* GetRefRankingRewardByRank(int32 rank = 1) {
		URefRankingReward** pptr = _rewards.FindByPredicate([this, rank](const URefRankingReward* reward){
			return rank >= reward->_rangeValues[0] && rank <= reward->_rangeValues[1];
		});

		return pptr ? *pptr : nullptr;
	}
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefEmblemEffect : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName Emblem_UID;
	UPROPERTY()
		int32 Upgrade_Value;
	
	// runtime
	TMap<FName, int32> _tagWithValues;
	TMap<class URefTag*, int32> _tags;
	TMap<EStatTypes, FStatInfoData> _stats;

	virtual void Parse(const FXmlNode* node) override;

	FString GetDebugString() const;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefPayment : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FName PID_PlayStore;
	UPROPERTY()
		FName PID_AppStore;
	// runtime
	URefShopItem* _targetShopItem = nullptr;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefAdvertisement : public URefBase
{
	// currency type

	GENERATED_BODY()
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefVIP : public URefBase
{
	// currency type

	GENERATED_BODY()
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSubscriptionReward : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
		FName Reward_UID;
	UPROPERTY()
		FName Bonus_Reward;
	// runtime
	UPROPERTY(BlueprintReadOnly)
		URefReward* _bonusReward = nullptr;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefArbeitReward : public URefBase
{
	GENERATED_BODY()
public:
	UPROPERTY()
		bool AutoReward;
	UPROPERTY()
		TArray<FName> Reward;
	// runtime
	TArray<URefReward*> _rewards;
	TArray<FString> ScheduleRewards;
	TMap<URefSchedule*, URefReward*> _scheduledRewards;

	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkillTree : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName Class_UID = NAME_None;
	UPROPERTY()
	int32 Order;

	// runtime
	UPROPERTY()
	URefClass* _class;
	UPROPERTY()
	TArray<URefSkillTreeStep*> _steps;

public:
	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkillTreeStep : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName SkillTree_UID = NAME_None;
	UPROPERTY()
	int32 Step;
	UPROPERTY()
	int32 Condition_PrevStepLevel;

	// runtime
	UPROPERTY()
	URefSkillTree* _tree;
	UPROPERTY()
	TArray<URefSkillTreeSlot*> _slots;
	UPROPERTY()
	URefSkillTreeStep* _next;
	UPROPERTY()
	URefSkillTreeStep* _prev;

public:
	virtual void Parse(const FXmlNode* node) override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkillTreeSlot : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName SkillTree_UID = NAME_None;
	UPROPERTY()
	int32 Step;
	UPROPERTY()
	int32 Order;
	UPROPERTY()
	TArray<FString> Reward_Skill;
	UPROPERTY(BlueprintReadOnly)
	FText Name;
	UPROPERTY()
	FText Desc;
	UPROPERTY()
	FName Icon = NAME_None;

	// runtime
	URefSkillTreeStep* _step = nullptr;
	TSoftObjectPtr<UTexture2D> _iconTexture;
	TArray<URefSkillTreeLevel*> _levels;
	TPair<URefSkill*, URefSkill*> _rewardSkill;
	TSet<URefStat*> _extraLevels;

public:
	virtual void Parse(const FXmlNode* node) override;

	bool IsMaxLevel(int32 level);
	URefSkillTreeLevel* GetLevelReference(int32 level);
};

UCLASS(BlueprintType)
class ANUREFERENCE_API URefSkillTreeLevel : public URefBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName SkillTreeSlot_UID;
	UPROPERTY()
	int32 Level;
	UPROPERTY(BlueprintReadOnly)
	FText Desc;
	UPROPERTY()
	TArray<FString> Reward_Stat;
	UPROPERTY()
	TArray<FString> Condition_License;
	UPROPERTY()
	FName Cost_UID;
	UPROPERTY()
	int32 Cost_Value;

	// runtime
	UPROPERTY(BlueprintReadOnly)
	TArray<FStatInfoData> _rewardStats;
	UPROPERTY(BlueprintReadOnly)
	URefClass* _conditionLicenseClass;
	UPROPERTY(BlueprintReadOnly)
	EClassLicense _conditionLicenseType = EClassLicense::None;
	UPROPERTY(BlueprintReadOnly)
	FDynamicCost _cost;

public:
	virtual void Parse(const FXmlNode* node) override;
};