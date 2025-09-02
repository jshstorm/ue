#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "Reference_Skill.h"
#include "Reference_Interactor.h"
#include "Reference_Resource.h"
#include "KeyGenerator.h"
#include "ReferenceBuilder.generated.h"

DEFINE_LOG_CATEGORY_STATIC(LogReference, Verbose, All);

class FJsonObject;

class ANUREFERENCE_API FReferenceLogBuilder
{
public:
	enum class ELevel
	{
		Error,
		Warning
	};

	static TArray<FString> _errors;
	static TArray<FString> _warnings;

public:
	template<size_t N, class... TArgs>
	static bool AddLog(ELevel level, const FString& expr_t, const TCHAR(&format)[N], TArgs&&... args)
	{
		FString composed = FString::Printf(format, Forward<TArgs>(args)...);
		InternalAddLog(level, expr_t, composed);
		return false;
	}

	/** Cleanup all logs. */
	static void Cleanup();

	/** Report log to dialog. */
	static void Report();
	/** Check contain one more errors. */
	static bool ContainsError();
	/** Check contain one more warning or error. */
	static bool ContainsWarning();
	/** Check that does not contain any errors or warnings. */
	static bool CleanSucceeded();

private:
	static void InternalAddLog(ELevel level, const FString& expr_t, const FString& composed);
};

//#if defined(DO_CHECK) && DO_CHECK

#define checkRefImpl(level, expr, format, ...) (\
	LIKELY((bool)(expr)) ||\
	FReferenceLogBuilder::AddLog(FReferenceLogBuilder::ELevel::level, #expr, TEXT("%s:%d ") format, *FString(__FILE__), __LINE__, ##__VA_ARGS__)\
)

#define checkRefMsgf(level, expr, format, ...) checkRefImpl(level, expr, format, ##__VA_ARGS__)
#define checkRefMsgfRet(level, expr, format, ...) if (!checkRefMsgf(level, expr, format, ##__VA_ARGS__)) return
#define checkRefMsgfCont(level, expr, format, ...) if (!checkRefMsgf(level, expr, format, ##__VA_ARGS__)) continue

#define checkRef(level, expr) checkRefImpl(level, expr, TEXT(""))
#define checkRefRet(level, expr) if (!checkRef(level, expr)) return
#define checkRefCont(level, expr) if (!checkRef(level, expr)) continue

//#else  // defined(DO_CHECK) && DO_CHECK
//
//#define checkRefMsgf(level, expr, format, ...)
//#define checkRefMsgfRet(level, expr, format, ...)
//#define checkRefMsgfCont(level, expr, format, ...)
//
//#define checkRef(level, expr)
//#define checkRefRet(level, expr)
//#define checkRefCont(level, expr)
//
//#endif  // defined(DO_CHECK) && DO_CHECK

UCLASS()
class ANUREFERENCE_API UReferences : public UObject
{
	GENERATED_BODY()

	UPROPERTY()
		TMap<int32, URefBase*> _references;
	UPROPERTY()
		TMap<FName, URefBase*> _referencesByUID;

public:
	void AddReference(URefBase* ref)
	{
		checkRefMsgfRet(Error, ref->GUID == 0 || _references.Find(ref->GUID) == nullptr, TEXT("duplicate data[%s]; guid[%d], uid[%s]"), *ref->GetName(), ref->GUID, *ref->UID.ToString());
		_references.Add(ref->GUID, ref);
		_referencesByUID.Add(ref->UID, ref);
	}

	void RemoveReference(URefBase* ref)
	{
		_references.Remove(ref->GUID);
		_referencesByUID.Remove(ref->UID);
	}

	uint16 GetCount() const { return _references.Num(); }
	
	void Reset()
	{
		_references.Reset();
		_referencesByUID.Reset();
	}

	URefBase* GetReference(int32 id)
	{
		return _references.FindRef(id);
	}

	URefBase* GetReference(const FString& uid)
	{
		return _referencesByUID.FindRef(*uid);
	}

	URefBase* GetReference(const FName& uid)
	{
		return _referencesByUID.FindRef(uid);
	}

	TMap<int32, URefBase*>* GetValues()
	{
		return &_references;
	}

	template<class T>
	FProperty* FindProperty(const FString& fieldName)
	{
		for (TFieldIterator<FProperty> it(T::StaticClass()); it; ++it) {
			FProperty* property = *it;
			if (property->GetName().Equals(fieldName)) {
				return property;
}
		}
		return nullptr;
	}

	template<class T>
	void QueryReference(const FString& fieldName, const FString& value, TFunction<void(T*)>& querier)
	{
		if (FProperty* property = FindProperty<T>(fieldName)) {
			for (auto& refData : _references) {
				URefBase* ref = refData.Value;
				void* memberProp = property->ContainerPtrToValuePtr<void>(ref);
				FString* refValue = (FString*)memberProp;
				if (refValue->Equals(value) == true)
				{
					querier((T*)ref);
				}
			}
		}
	}

	template<class T>
	void QueryReference(const FString& fieldName, const FName& value, TFunction<void(T*)>& querier)
	{
		if (FProperty* property = FindProperty<T>(fieldName)) {
			for (auto& refData : _references) {
				URefBase* ref = refData.Value;
				void* memberProp = property->ContainerPtrToValuePtr<void>(ref);
				FName* refValue = (FName*)memberProp;
				if (*refValue == value)	{
					querier((T*)ref);
				}
			}
		}
	}

	template<class T>
	void QueryReference(const FString& fieldName, int32 value, TFunction<void(T*)>& querier)
	{
		if (FProperty* property = FindProperty<T>(fieldName)) {
			for (auto& refData : _references) {
				URefBase* ref = refData.Value;
				void* memberProp = property->ContainerPtrToValuePtr<void>(ref);
				int32* refValue = (int32*)memberProp;
				if (*refValue == value)
				{
					querier((T*)ref);
				}
			}
		}
	}
};

UCLASS()
class ANUREFERENCE_API UReferenceList : public UObject
{
	GENERATED_BODY()

	UPROPERTY()
		TArray<URefBase*> _list;

public:
	void AddReference(URefBase* ref)
	{
		_list.Emplace(ref);
	}

	TArray<URefBase*>* GetValues()
	{
		return &_list;
	}
};

UCLASS(BlueprintType, Blueprintable)
class UArchiveInstance : public UObject
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	UFUNCTION(BlueprintCallable)
		void AddRefObject(UObject* object);
	UFUNCTION(BlueprintCallable)
		void Export(FName fileName, UClass* specifiedClass);
#endif

private:
	UPROPERTY()
		FArchiveReferences _archive;
};

UCLASS() 
class ANUREFERENCE_API UAnuMesh : public UObject { GENERATED_BODY() };
UCLASS() 
class ANUREFERENCE_API UAnuSnapshot : public UObject { GENERATED_BODY() };
UCLASS() 
class ANUREFERENCE_API UAnuElectronicBoard : public UObject { GENERATED_BODY() };
UCLASS() 
class ANUREFERENCE_API UAnuElectronicBoardNews : public UObject { GENERATED_BODY() };
UCLASS() 
class ANUREFERENCE_API UAnuMeshConstruct : public UObject { GENERATED_BODY() };
UCLASS(BlueprintType)
class ANUREFERENCE_API UAnuSocialMotion : public UObject { 
	GENERATED_BODY()
public: 
	UPROPERTY(BlueprintReadOnly)
	FAnuResourceSocialMotion impl;
};
UCLASS() 
class ANUREFERENCE_API UAnuResLifeAction : public UObject { GENERATED_BODY() };
UCLASS()
class ANUREFERENCE_API UAnuBellSound : public UObject { GENERATED_BODY() };
UCLASS()
class ANUREFERENCE_API UAnuChatTag : public UObject { GENERATED_BODY() };
UCLASS() 
class ANUREFERENCE_API UAnuResGallery : public UObject { GENERATED_BODY() };
UCLASS()
class ANUREFERENCE_API UAnuMeshTransformation : public UObject { GENERATED_BODY() };
UCLASS()
class ANUREFERENCE_API UAnuChatSticker : public UObject { GENERATED_BODY() };
UCLASS()
class ANUREFERENCE_API UAnuFlipBook : public UObject { GENERATED_BODY() };
UCLASS()
class ANUREFERENCE_API UAnuLevelSequenceClassPose : public UObject { GENERATED_BODY() };
UCLASS()
class ANUREFERENCE_API UAnuEventSchedule : public UObject { GENERATED_BODY() };

UENUM(BlueprintType)
enum class EUnderBodyType : uint8
{
	Pants, ThighLeg, ThinLeg,
};

USTRUCT()
struct FAnuMeshHideTable : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
};

USTRUCT(BlueprintType)
struct FAnuUnderBodyHide : public FAnuMeshHideTable
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<ESubMeshType> HideTypes;
};

USTRUCT(BlueprintType)
struct FAnuGlovesMeshHide : public FAnuMeshHideTable
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere)
		bool HideHand = false;
};

USTRUCT(BlueprintType)
struct FAnuAttachmentHide : public FAnuMeshHideTable
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<EAttachmentParts> HideParts;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuEventScheduleRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EEventSchedule EventType;
	UPROPERTY(EditAnywhere)
	FName ScheduleUID;
	UPROPERTY(EditAnywhere)
	FString Comment;
};

UCLASS()
class ANUREFERENCE_API UReferenceBuilder : public UObject
{
	GENERATED_BODY()

	DECLARE_DELEGATE_OneParam(FReferenceHandler, const FXmlNode*);
	TArray<TPair<FString, FReferenceHandler>> _refHandlers;
	DECLARE_DELEGATE_TwoParams(FJsonReferenceHandler, const FString&, const FJsonObject*);
	TArray<TPair<FString, FJsonReferenceHandler>> _refJsonHandlers;

	TArray<TFunction<void()>> _postProcessors;
	TMap<FName, int32> _typeNames;
	TMap<FName, TMap<FName, URefRegion*>> _worldLookupTable;
	TMap<FName, TArray<URefShopCost*>> _shopCosts;

	UPROPERTY()
	TMap<FName, UClass*> _refClasses;
	UPROPERTY()
	TMap<UClass*, UReferences*> _references;
	UPROPERTY()
	TMap<UClass*, UReferenceList*> _referenceList; // references which no have uid. support only iterate
	UPROPERTY()
	TMap<UStruct*, UDataTable*> _resourceTables;
	UPROPERTY()
	TMap<FName, FString> _globals;
	UPROPERTY()
	TArray<URefLevelPC*> _charLevels;

	TArray<int64> _npcLvTable;
	TMap<FName, URefClass*> _classByTID2;
	TMap<uint32, URefLifeObject*> _lifeObjByTID;
	TMap<FName, TArray<URefQuest*>> _questBySubGroup;
	TMap<FName, TArray<URefReply*>> _replys; // <replyUID, replys>

	TMap<UClass*, TMap<FName, TArray<URefBase*>>> _refGroups;
	TArray<URefRewardPeriod*> _calendars;

public:
	inline static FString SpawnPathPrefix{ "Spawn_" };
	inline static URefCurrency* PopularityCurrency = nullptr;

	static FString GetJsonSrcDirectory();
	static FString GetJsonIndexPath();
	static void GetJsonFilePaths(const FString& tableName, TArray<FString>& paths);
	static FString GetWorldLookupFilePath(const FString& worldName);

	static TSharedPtr<FJsonObject> LoadJsonFile(const FString& path);

	static void LoadDialogStringTable();
	static void AddDialogString(const FString& strUID, const FString& strValue);
	static bool AddDialogString(const FString& dlgUID, const FJsonObject* root);
	static bool ClearDialogStringTable();
	static bool CommitDialogStringTable();

	template<typename T>
	static T* CreateReference(UObject* outer, const FString& uid) {
		T* reference = NewObject<T>(outer);
		reference->UID = *uid;
		reference->GUID = UCRC32::GetPtr()->Generate32(*uid);
		return reference;
	}

public:
	bool Initialize();
	void Finalize();
#if WITH_EDITOR
	static void SaveReference(FName table, FArchiveReferences& archive, UClass* specifiedClass);
	static bool ExportSpawner(TArray<ARefInteractor*> spawnActors);
	static void ExportWorldLookupTarget(UWorld* world, const TArray<AActor*>& targetActors);

	void ExecuteSkillTimelineHandler(const FXmlNode* root);
	void ExecuteSkillTimelinePostProcessor();
#endif

	void AddDebugReference(URefBase* reference, TSet<UClass*>&& additionalCacheClasses);

private:
	void InitializeDataTable();
	void InitializeSkillDataTable();
	void InitializeCostumeData();

	bool LoadFiles();
	TSharedPtr<class FXmlFile> LoadTableFile(const FString& name);

	void IterateNodes(const FXmlNode* root, TFunction<void(const FXmlNode*)> handler);

	// debug
	void URefLegacyHandler(const FXmlNode* root);
	void PrintLegacyGuids(UClass* tgtRefClass);

	//handlers
	void RefTypeNameHandler(const FXmlNode* root);
	void URefContentsSrcHandler(const FString& uid, const FJsonObject* root);

	void URefPCHandler(const FXmlNode* root);
	void URefNPCHandler(const FXmlNode* root);
	void URefMonsterHandler(const FXmlNode* root);
	void URefItemHandler(const FXmlNode* root);
	void URefItemEquipHandler(const FXmlNode* root);
	void URefItemCostumeHandler(const FXmlNode* root);
	void URefItemEmblemHandler(const FXmlNode* root);
	void URefItemUsableHandler(const FXmlNode* root);
	void URefItemQuestHandler(const FXmlNode* root);
	void URefItemEtcHandler(const FXmlNode* root);
	void URefLifeObjectHandler(const FXmlNode* root);
	void URefPortalHandler(const FXmlNode* root);
	void URefGimmickHandler(const FXmlNode* root);
	void URefCharacterStatHandler(const FXmlNode* root);
	void URefRegionHandler(const FXmlNode* root);

	void URefClassHandler(const FXmlNode* root);
	void URefClassLevelHandler(const FXmlNode* root);
	void URefClassLicenseMasteryHandler(const FXmlNode* root);

	void URefSkillHandler();
	void URefSkillObjectHandler();
	void URefSkillEffectHandler(const FXmlNode* root);
	void URefSkillTimelineHandler();

	void URefStageGroupHandler(const FXmlNode* root);
	void URefStageInfoHandler(const FXmlNode* root);
	void URefStageContestHandler(const FXmlNode* root);
	void URefContestDonationGuideHandler(const FXmlNode* root);

	void URefCurrencyHandler(const FXmlNode* root);
	void URefCurrencyStressHandler(const FXmlNode* root);
	void URefWorldHandler(const FXmlNode* root);

	void URefGlobalHandler(const FXmlNode* root);
	void URefLevelPCHandler(const FXmlNode* root);
	void URefLevelNPCHandler(const FXmlNode* root);
	void URefChatStickerGroupHandler(const FXmlNode* root);

	void URefQuestGroupHandler(const FXmlNode* root);
	void URefQuestHandler(const FXmlNode* root);
	void URefQuestSequenceHandler(const FXmlNode* root);
	void URefQuestEventHandler(const FXmlNode* root);
	void URefQuestScheduleHandler(const FXmlNode* root);
	void URefQuestChallengeHandler(const FXmlNode* root);
	void URefArbeitQuestGroupHandler(const FXmlNode* root);
	void URefArbeitRewardHandler(const FXmlNode* root);
	void URefQuestArbeitHandler(const FXmlNode* root);
	void URefQuestSequenceArbeitHandler(const FXmlNode* root);
	void URefStampTourQuestGroupHandler(const FXmlNode* root);
	void URefPassQuestGroupHandler(const FXmlNode* root);
	void URefQuestPassHandler(const FXmlNode* root);

	void URefTitleHandler(const FXmlNode* root);
	void URefReplyHandler(const FXmlNode* root);
	void URefReactionHandler(const FXmlNode* root);
	void URefStreamingHandler(const FXmlNode* root);
	void URefInterviewHandler(const FXmlNode* root);
	void URefInspectingHandler(const FXmlNode* root);
	void URefTakePhotoHandler(const FXmlNode* root);
	void URefItemReviewHandler(const FXmlNode* root);
	void URefItemTradeHandler(const FXmlNode* root);
	void URefStreamingNPCHandler(const FXmlNode* root);

	void URefRankingRewardHandler(const FXmlNode* root);
	void URefSubscriptionRewardHandler(const FXmlNode* root);
	void URefStageRewardHandler(const FXmlNode* root);
	void URefUserInteractionHandler(const FXmlNode* root);

	void URefRewardStaticHandler(const FXmlNode* root);
	void URefRewardRandomHandler(const FXmlNode* root);
	void URefLifeRewardHandler(const FXmlNode* root);
	void URefRewardSelectableHandler(const FXmlNode* root);
	void URefRewardPostHandler(const FXmlNode* root);
	void URefRewardPeriodHandler(const FXmlNode* root);
	void URefRewardHandler(const FXmlNode* root);
	void URefDialogHandler(const FString& uid, const FJsonObject* root);
	void URefScheduleHandler(const FXmlNode* root);
	void URefPostTemplateHandler(const FXmlNode* root);
	void URefNPCProfileHandler(const FXmlNode* root);
	void URefStatHandler(const FXmlNode* root);
	void URefRankingHandler(const FXmlNode* root);

	void URefEquipAttributeHandler(const FXmlNode* root);
	void URefEquipUpgradeHandler(const FXmlNode* root);
	void URefEquipUpgradeEffectHandler(const FXmlNode* root);
	void URefEquipCarveEffectHandler(const FXmlNode* root);
	
	void URefEquipCraftHandler(const FXmlNode* root);
	void URefEquipReforgeHandler(const FXmlNode* root);
	void URefEquipSetEffectHandler(const FXmlNode* root);
	void URefEquipCollectionGroupHandler(const FXmlNode* root);
	void URefEquipCollectionHandler(const FXmlNode* root);
	void URefEmblemEffectHandler(const FXmlNode* root);

	void URefSkillPatronageHandler(const FXmlNode* root);
	void URefSkillPatronageTierHandler(const FXmlNode* root);
	void URefSkillPatronageCostHandler(const FXmlNode* root);

	//Customizing
	void URefBodyHandler(const FXmlNode* root);
	void URefCustomDetailHandler(const FXmlNode* root);
	void URefPaletteGroupHandler(const FXmlNode* root);
	void URefPaletteHandler(const FXmlNode* root);
	void URefColorHandler(const FXmlNode* root);
	void URefColorPigmentHandler(const FXmlNode* root);

	//Shop
	void URefShopGroupHandler(const FXmlNode* root);
	void URefShopItemHandler(const FXmlNode* root);
	void URefShopItemInAppHandler(const FXmlNode* root);
	void URefShopItemRandomHandler(const FXmlNode* root);
	void URefShopCostHandler(const FXmlNode* root);
	void URefShopPoolHandler(const FXmlNode* root);
	void URefPaymentHandler(const FXmlNode* root);

	// resource
	void ResourceHandler(const FString& tableName, UStruct* clazz);
	void URefResourceCoreHandler(const FXmlNode* root);

	// fashion contents
	void URefFashionContentsScoreHandler(const FXmlNode* root);
	void URefFashionContentsGroupHandler(const FXmlNode* root);
	void URefFashionContentsStageHandler(const FXmlNode* root);
	void URefFashionContentsNPCHandler(const FXmlNode* root);
	void URefFashionContentsFactorHandler(const FXmlNode* root);

	// tag
	void URefTagGroupHandler(const FXmlNode* root);
	void URefTagHandler(const FXmlNode* root);

	// skilltree
	void URefSkillTreeHandler(const FXmlNode* root);
	void URefSkillTreeStepHandler(const FXmlNode* root);
	void URefSkillTreeSlotHandler(const FXmlNode* root);
	void URefSkillTreeLevelHandler(const FXmlNode* root);

	//post processors
	void URefObjectPostProcessor();
	void URefCharacterPostProcessor();
	void URefItemPostProcessor();
	void URefLevelPCPostProcessor();
	void URefLevelNPCPostProcessor();
	void URefSkillPostProcessor();
	void URefSkillTimelinePostProcessor();
	void URefClassPostProcessor();
	void URefWorldPostProcessor();
	void URefRegionPostProcessor();
	void URefQuestPostProcessor();
	void URefTitlePostProcessor();
	void URefReplyPostProcessor();
	void URefRewardPostProcessor();
	void URefChatStickerGroupPostProcessor();
	void URefStageGroupPostProcessor();
	void URefStageInfoPostProcessor();
	void URefStageContestPostProcessor();
	void URefDialogPostProcessor();	
	void URefCustomDetailPostProcessor();
	void URefCurrencyPostProcessor();
	void URefShopItemPostProcessor();
	void URefPostTemplatePostProcessor();
	void URefStreamingPostProcessor();
	void URefInterviewPostProcessor();
	void URefInspectingPostProcessor();
	void URefItemReviewPostProcessor();
	void URefItemTradePostProcessor();
	void URefStreamingNPCPostProcessor();
	void URefRankingRewardPostProcessor();
	void URefUserInteractionPostProcessor();
	void URefBodyPostProcessor();
	void URefSkillPatronagePostProcessor();
	void FAnuResourceWorldListPostProcessor();
	void URefFashionContentsGroupPostProcessor();
	void URefFashionContentsStagePostProcessor();
	void URefTagGroupPostProcessor();
	void URefPaletteGroupPostProcessor();
	void URefSchedulePostProcessor();
	void URefEquipCollectionGroupPostProcessor();
	void URefEmblemEffectPostProcessor();
	void URefStatPostProcessor();
	void URefCharacterStatPostProcessor();
	void URefArbeitRewardPostProcessor();
	void URefRankingPostProcessor();
	void URefSkillTreePostProcessor();
	void URefSkillTreeStepPostProcessor();
	void URefSkillTreeSlotPostProcessor();
	void URefSkillTreeLevelPostProcessor();

private:
	void BuildSkillTimelineEffect(URefSkillTimeline* reference);
	void PostQuestCondition(URefQuestSequence* questSeq);
	void ReserveQuestEvent(const FName& uid, const FName& type, const FString& typeValue);

public:
	template<class T>
	UReferences* GetReferences()
	{
		return _references.FindRef(T::StaticClass());
	}

	template<class T>
	void DoRefIterationJob(TFunction<void(T*)> job)
	{
		if (UReferences* references = _references.FindRef(T::StaticClass())) {

			for (auto& item : *references->GetValues())
			{
				check(Cast<T>(item.Value) != nullptr);
				job((T*)item.Value);
			}
		}
	}

	template<class T>
	void DoWhile(TFunction<bool(T*)> job)
	{
		if (UReferences* references = _references.FindRef(T::StaticClass())) {

			for (auto& item : *references->GetValues()) {
				if (job((T*)item.Value) == false) {
					break;
				}
			}
		}
	}

	template<class T>
	void DoRefIterationJobNoUID(TFunction<void(T*)> job)
	{
		if (UReferenceList* references = _referenceList.FindRef(T::StaticClass()))
		{
			for (auto& item : *references->GetValues())
			{
				check(Cast<T>(item) != nullptr);
				job((T*)item);
			}
		}
	}

	bool GetRefGlobal(const FName& key, int32& value);
	bool GetRefGlobal(const FName& key, float& value);
	bool GetRefGlobal(const FName& key, FString& value);
	void GetUIDs(TSubclassOf<URefBase> clazz, TArray<FName>& uids);

	URefLevelPC* GetRefLevelPCByExp(int32 exp);
	URefLevelPC* GetRefLevelPCByLevel(int32 level);
	void GetQuestBySubGroup(const FName& subGroupKey, TArray<URefQuest*>& questList);
	int64 GetNPCLevelStartExpOffset(int32 lv);
	void GetReply(const FName& replyUID, TArray<URefReply*>& out);
	int32 GetGuid(const FName& tableName, const FName& uid) const;
	URefRegion* GetTargetRegion(const FName& expectedType, const FName& uid) const;
	URefClass* GetClassByTid2(const FName& tid2) const;
	const TArray<URefRewardPeriod*>& GetCalendars() const { return _calendars; }
	const TArray<URefShopCost*>* GetShopCosts(const FName& shopCostUID) const;

	int32 GetTypeNumber(const FName& typeName)
	{
		return _typeNames.FindRef(typeName);
	}

	template<class T>
	void GetGroupReference(const FName& key, TArray<T*>& outValus)
	{
		outValus.Empty();
		if (auto groupRef = _refGroups.Find(T::StaticClass())) {
			if (auto groupValue = groupRef->Find(key)) {
				for (auto& value : *groupValue) {
					outValus.Emplace(Cast<T>(value));
				}
			}
		}
	}
    
	template<class T>
	T* GetRefObj(int32 idx)
	{
		UReferences* reference = _references.FindRef(T::StaticClass());
		if (reference == nullptr) {
			return nullptr;
		}

		return Cast<T>(reference->GetReference(idx));
	}

	UObject* GetRefObj(const FName& uid, TSubclassOf<URefBase> clazz)
	{
		UReferences* reference = _references.FindRef(clazz);
		if (reference == nullptr) {
			return nullptr;
		}

		return reference->GetReference(uid);
	}

	UObject* GetRefObj(int32 guid, TSubclassOf<URefBase> clazz)
	{
		UReferences* reference = _references.FindRef(clazz);
		if (reference == nullptr) {
			return nullptr;
		}

		return reference->GetReference(guid);
	}

	template<class T>
	T* GetRefObj(const FString& uid)
	{
		UReferences* reference = _references.FindRef(T::StaticClass());
		if (reference == nullptr) {
			return nullptr;
		}

		return Cast<T>(reference->GetReference(uid));
	}

	template<class T>
	T* GetRefObj(const FName& uid)
	{
		UReferences* reference = _references.FindRef(T::StaticClass());
		if (reference == nullptr) {
			return nullptr;
		}

		return Cast<T>(reference->GetReference(uid));
	}

	template<class T>
	void QueryReference(const FString& fieldName, const FString& value, TFunction<void(T*)> querier)
	{
		UReferences* reference = _references.FindRef(T::StaticClass());
		if (reference == nullptr) {
			return;
		}

		reference->QueryReference<T>(fieldName, value, querier);
	}

	template<class T>
	void QueryReference(const FString& fieldName, int32 value, TFunction<void(T*)> querier)
	{
		UReferences* reference = _references.FindRef(T::StaticClass());
		if (reference == nullptr) {
			return;
		}

		reference->QueryReference<T>(fieldName, value, querier);
	}

	template<class T>
	void QueryReference(const FString& fieldName, FName value, TFunction<void(T*)> querier)
	{
		UReferences* reference = _references.FindRef(T::StaticClass());
		if (reference == nullptr) {
			return;
		}

		reference->QueryReference<T>(fieldName, value, querier);
	}

	template<class T, decltype(T::StaticClass())* = nullptr>
	FAnuTableRow* GetResourceRow(const FName& rowName)
	{
		if (rowName == NAME_None) {
			return nullptr;
		}

		auto dt = _resourceTables.FindRef(T::StaticClass());
		checkf(dt, TEXT("resource table not exist for class[%s], rowName[%s]"), *T::StaticClass()->GetName(), *rowName.ToString());
		return dt->template FindRow<FAnuTableRow>(rowName, "", false);
	}

	template<class T, decltype(T::StaticStruct())* = nullptr>
	T* GetResourceRow(const FName& rowName)
	{
		if (rowName == NAME_None) {
			return nullptr;
		}

		auto dt = _resourceTables.FindRef(T::StaticStruct());
		checkf(dt, TEXT("resource table not exist for struct[%s], rowName[%s]"), *T::StaticStruct()->GetName(), *rowName.ToString());
		return dt->template FindRow<T>(rowName, TEXT(""), false);
	}

	FAnuTableRow* GetResourceRow(TSubclassOf<UObject> clazz, const FName& rowName)
	{
		if (rowName == NAME_None) {
			return nullptr;
		}

		auto dt = _resourceTables.FindRef(clazz.Get());
		if (dt == nullptr) {
			return nullptr;
		}	
		return dt->FindRow<FAnuTableRow>(rowName, "", false);
	}

	template<class T>
	T* GetResource(const FName& rowName)
	{
		FAnuTableRow* resourceRow = GetResourceRow(T::StaticClass(), rowName);
		if (resourceRow == nullptr) {
			return nullptr;
		}
		return Cast<T>(resourceRow->LoadResource());
	}

	template<class T>
	TSubclassOf<T> GetAssetClass(const FName& rowName)
	{
		FAnuTableRow* resourceRow = GetResourceRow(T::StaticClass(), rowName);
		if (resourceRow == nullptr) {
			return nullptr;
		}
		return resourceRow->LoadClass();
	}

	template<class T>
	FString GetResourcePath(const FName& rowName)
	{
		FAnuTableRow* resourceRow = GetResourceRow(T::StaticClass(), rowName);
		if (resourceRow == nullptr) {
			return FAnuTableRow::EmptyPath;
		}
		return resourceRow->GetPath();
	}

	template<class T>
	const FSoftObjectPath& GetResourceRoute(const FName& rowName)
	{
		FAnuTableRow* resourceRow = GetResourceRow(T::StaticClass(), rowName);
		if (resourceRow == nullptr) {
			return FAnuTableRow::EmptyRoute;
		}
		return resourceRow->GetRoute();
	}

	void LoadResource(const TArray<FName>& iconUIDs, TArray<UTexture2D*>& output);
	void LoadResource(const TArray<FName>& iconUIDs, TArray<TSoftObjectPtr<UTexture2D>>& output);
	void CostPostProcessor(FDynamicCost& dst);
	void TagPostProcessor(const FString& contextString, const TMap<FName, int32>& tagWithValues, TMap<URefTag*, int32>& output);
	void RewardCommonPostProcessor(URefRewardBase* reference, ERewardType type, bool fromReference = true);
	URefSchedule* ParseDynamicSchedule(const FString& scheduleValue, const FString& context, bool useLocalTime);

	template<class T>
	UDataTable* GetResourceTable() {
		return _resourceTables.FindRef(T::StaticClass());
	}

	template <class T>
	void GetResourceTable(const TArray<FName>& modelUIDs, TArray<FAnuTableRow*>& output) {
		for (auto& uid : modelUIDs) {
			if (FAnuTableRow* resourceRow = GetResourceRow(T::StaticClass(), uid)) {
				output.Emplace(resourceRow);
			}
		}
	}

private:
	TMap<FName, TSharedPtr<class FJsonObject>> _questEventJsonCache;
	void FillQuestEvent(const TArray<TSharedPtr<class FJsonValue>>& eventArray, TArray<class URefQuestEvent*>& outArray);
public:
	TSharedPtr<class FJsonObject> LoadQuestEventJson(const FName& fileName, bool useCache = true);
	void FillQuestEvent(const FString& eventName, TSharedPtr<class FJsonObject> eventJson, TArray<class URefQuestEvent*>& outArray);

};
