#include "ReferenceBuilder.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Misc/DefaultValueHelper.h"
#include "Misc/FileHelper.h"
#include "Kismet/KismetStringTableLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraShake.h"
#include "Engine/AssetManager.h"
#include "Serialization/JsonSerializer.h"
#include "GameFramework/Character.h"
#include "Particles/ParticleSystem.h"
#include "LevelDesign.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Sound/SoundWave.h"

#include "Internationalization/StringTableRegistry.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTable.h"

#if WITH_EDITOR
#include "UnrealED/Public/FileHelpers.h"
#endif

#include "ReferenceBuilder.ReferenceLogBuilder.inl"

namespace ReferenceBuilder::Details
{
	template<class TClass, decltype(TClass::StaticClass())* = nullptr>
	inline auto GetStaticClass()
	{
		return TClass::StaticClass();
	}

	template<class TStruct, decltype(TStruct::StaticStruct())* = nullptr>
	inline auto GetStaticClass()
	{
		return TStruct::StaticStruct();
	}
}

#define REGISTER_REF_HANDLERS(tableName, klass)  { \
	_refHandlers.Add(MakeTuple(tableName, FReferenceHandler::CreateUObject(this, &UReferenceBuilder::klass##Handler))); \
	_references.Add(klass::StaticClass(), NewObject<UReferences>(this));\
}

#define REGISTER_REF_JSON_HANDLERS(tableName, klass)  { \
	_refJsonHandlers.Add(MakeTuple(tableName, FJsonReferenceHandler::CreateUObject(this, &UReferenceBuilder::klass##Handler))); \
	_references.Add(klass::StaticClass(), NewObject<UReferences>(this));\
}

#define REGISTER_RESOURCE_HANDLERS(tableName, klass) {\
	ResourceHandler(tableName, ReferenceBuilder::Details::GetStaticClass<klass>()); \
}

#define REGISTER_REF_NO_UID_HANDLERS(tableName, klass)  { \
	_refHandlers.Add(MakeTuple(tableName, FReferenceHandler::CreateUObject(this, &UReferenceBuilder::klass##Handler))); \
	_referenceList.Add(klass::StaticClass(), NewObject<UReferenceList>(this));\
}

#define REGISTER_REF_NO_UID_HANDLERS_WITH_POSTHANDLER(tableName, klass)  { \
	_refHandlers.Add(MakeTuple(tableName, FReferenceHandler::CreateUObject(this, &UReferenceBuilder::klass##Handler))); \
	_referenceList.Add(klass::StaticClass(), NewObject<UReferenceList>(this));\
	_postProcessors.Emplace([this]() { UReferenceBuilder::klass##PostProcessor(); }); \
}

#define REGISTER_REF_CUSTOM_HANDLERS(tableName, func)  { \
	_refHandlers.Add(MakeTuple(tableName, FReferenceHandler::CreateUObject(this, &UReferenceBuilder::func))); \
}

#define REGISTER_REF_HANDLERS_WITH_POSTHANDER(tableName, klass)  { \
	_refHandlers.Emplace(MakeTuple(tableName, FReferenceHandler::CreateUObject(this, &UReferenceBuilder::klass##Handler))); \
	_refClasses.Emplace(tableName, klass::StaticClass()); \
	_references.Add(klass::StaticClass(), NewObject<UReferences>(this));\
	_postProcessors.Emplace([this]() { UReferenceBuilder::klass##PostProcessor(); }); \
}

#define REGISTER_REF_JSON_HANDLERS_WITH_POSTHANDER(tableName, klass)  { \
	_refJsonHandlers.Add(MakeTuple(tableName, FJsonReferenceHandler::CreateUObject(this, &UReferenceBuilder::klass##Handler))); \
	_refClasses.Emplace(tableName, klass::StaticClass()); \
	_references.Add(klass::StaticClass(), NewObject<UReferences>(this));\
	_postProcessors.Emplace([this]() { UReferenceBuilder::klass##PostProcessor(); }); \
}


FString UReferenceBuilder::GetJsonSrcDirectory()
{
	return FPaths::ProjectContentDir() + "Anu/JsonSrc/";
}

FString UReferenceBuilder::GetJsonIndexPath()
{
	return GetJsonSrcDirectory() + "index.json";
}

FString UReferenceBuilder::GetWorldLookupFilePath(const FString& worldName)
{
	FString contentDirectory = FPaths::ProjectContentDir();
	FString fileName = FString::Printf(TEXT("%s.xml"), *worldName);
	return FPaths::Combine(contentDirectory, TEXT("Anu"), TEXT("DataTable"), TEXT("References"), TEXT("WorldLookUp"), *fileName);
}

void UReferenceBuilder::LoadDialogStringTable()
{
	FName tableID{ AnuText::Get_DialogTableID() };
#if WITH_EDITOR
	IStringTableEngineBridge::FullyLoadStringTableAsset(tableID);
#endif
}

bool UReferenceBuilder::ClearDialogStringTable()
{
	FStringTablePtr StringTable = FStringTableRegistry::Get().FindMutableStringTable(AnuText::Get_DialogTableID());
	if (StringTable.IsValid() == false) {
		return false;
	}
	StringTable->ClearSourceStrings();
	return true;
}

void UReferenceBuilder::AddDialogString(const FString& strUID, const FString& strValue)
{
	FStringTableRegistry::Get().Internal_SetLocTableEntry(AnuText::Get_DialogTableID(), strUID, strValue);
}

bool UReferenceBuilder::AddDialogString(const FString& dlgUID, const FJsonObject* root)
{
	const TArray<TSharedPtr<FJsonValue>>* dlgSubs;
	if (root->TryGetArrayField("DialogSub", dlgSubs) == false) {
		return false;
	}

	int32 id = 0;
	FString strValue;
	for (auto& dlgSub : *dlgSubs) {
		const TSharedPtr<FJsonObject>* dlgSubObj = nullptr;
		if (dlgSub->TryGetObject(dlgSubObj) == false) {
			return false;
		}

		if ((*dlgSubObj)->TryGetStringField("String", strValue) == false) {
			return false;
		}

		FString strUID{ FString::Printf(TEXT("str.%s.%d"), *dlgUID, ++id) };
		UReferenceBuilder::AddDialogString(strUID, strValue);
	}
	return true;
}

bool UReferenceBuilder::CommitDialogStringTable()
{
#if WITH_EDITOR
	FStringTablePtr StringTable = FStringTableRegistry::Get().FindMutableStringTable(AnuText::Get_DialogTableID());
	if (StringTable.IsValid() == false) {
		return false;
	}

	auto stringTableAsset = StringTable->GetOwnerAsset();
	if (stringTableAsset == nullptr) {
		return false;
	}

	TArray<UPackage*> packages;
	packages.Add(stringTableAsset->GetOutermost());
	if (UEditorLoadingAndSavingUtils::SavePackagesWithDialog(packages, true) == false) {
		return false;
	}
#endif

	return true;
}

bool UReferenceBuilder::Initialize()
{
	FReferenceLogBuilder::Cleanup();

	UE_LOG(LogReference, Verbose, TEXT("UReferenceBuilder::Initialize"));
	auto started = FPlatformTime::Seconds();

#define REGISTER_ABSTRACT_TABLE(klass) \
	_references.Add(klass::StaticClass(), NewObject<UReferences>(this));
#define REGISTER_ABSTRACT_TABLE_WITH_POSTHANDLER(klass) { \
	_references.Add(klass::StaticClass(), NewObject<UReferences>(this)); \
	_postProcessors.Emplace([this]() { UReferenceBuilder::klass##PostProcessor(); }); \
}

	REGISTER_ABSTRACT_TABLE(URefObject);
	REGISTER_ABSTRACT_TABLE_WITH_POSTHANDLER(URefCharacter);

	InitializeDataTable();

	// resource
	REGISTER_REF_HANDLERS("ResourceCore", URefResourceCore);
	REGISTER_REF_JSON_HANDLERS("Contents", URefContentsSrc);

	// object
	REGISTER_REF_CUSTOM_HANDLERS("TypeIDName", RefTypeNameHandler);
	REGISTER_REF_HANDLERS("PC", URefPC);
	REGISTER_REF_HANDLERS("NPC", URefNPC);
	REGISTER_REF_HANDLERS("Monster", URefMonster);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Item", URefItem);
	REGISTER_REF_HANDLERS("Item_Equip", URefItemEquip);
	REGISTER_REF_HANDLERS("Item_Costume", URefItemCostume);
	REGISTER_REF_HANDLERS("Item_Emblem", URefItemEmblem);
	REGISTER_REF_HANDLERS("Item_Usable", URefItemUsable);
	REGISTER_REF_HANDLERS("Item_Quest", URefItemQuest);
	REGISTER_REF_HANDLERS("Item_Etc", URefItemEtc);
	REGISTER_REF_HANDLERS("LifeObject", URefLifeObject);
	REGISTER_REF_HANDLERS("Portal", URefPortal);
	REGISTER_REF_HANDLERS("Gimmick", URefGimmick);
	REGISTER_REF_NO_UID_HANDLERS_WITH_POSTHANDLER("CharacterStat", URefCharacterStat);

	// reward
	REGISTER_REF_NO_UID_HANDLERS("RewardStatic", URefRewardStatic);
	REGISTER_REF_NO_UID_HANDLERS("RewardRandom", URefRewardRandom);
	REGISTER_REF_NO_UID_HANDLERS("LifeReward", URefLifeReward);
	REGISTER_REF_NO_UID_HANDLERS("StageReward", URefStageReward);
	REGISTER_REF_NO_UID_HANDLERS("RewardSelectable", URefRewardSelectable);
	REGISTER_REF_NO_UID_HANDLERS("RewardPost", URefRewardPost);
	REGISTER_REF_NO_UID_HANDLERS("RewardPeriod", URefRewardPeriod);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Reward", URefReward);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("RankingReward", URefRankingReward);
	REGISTER_REF_NO_UID_HANDLERS("SubscriptionReward", URefSubscriptionReward);

	// class
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Class", URefClass);
	REGISTER_REF_NO_UID_HANDLERS("ClassLevel", URefClassLevel);
	REGISTER_REF_HANDLERS("ClassLicenseMastery", URefClassLicenseMastery);

	// common
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Region", URefRegion);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Currency", URefCurrency);
	REGISTER_REF_HANDLERS("Currency_Stress", URefCurrencyStress);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("World", URefWorld);
	REGISTER_REF_HANDLERS("Global", URefGlobal);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("LevelPC", URefLevelPC);
	REGISTER_REF_NO_UID_HANDLERS_WITH_POSTHANDLER("LevelNPC", URefLevelNPC);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Schedule", URefSchedule);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("PostTemplate", URefPostTemplate);
	REGISTER_REF_HANDLERS("NPCComment", URefNPCProfile);
	REGISTER_REF_HANDLERS("Stat", URefStat);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Ranking", URefRanking);

	// custom
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Body", URefBody);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("CustomDetail", URefCustomDetail);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("PaletteGroup", URefPaletteGroup);
	REGISTER_REF_NO_UID_HANDLERS("Palette", URefPalette);
	REGISTER_REF_HANDLERS("Color", URefColor);
	REGISTER_REF_NO_UID_HANDLERS("ColorPigment", URefColorPigment);

	REGISTER_REF_NO_UID_HANDLERS("SkillEffect", URefSkillEffect);

	// stage
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("StageGroup", URefStageGroup);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("StageInfo", URefStageInfo);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("StageContest", URefStageContest);
	REGISTER_REF_NO_UID_HANDLERS("ContestDonationGuide", URefContestDonationGuide);

	// chat sticker
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("ChatStickerGroup", URefChatStickerGroup);

	// quest
	REGISTER_REF_HANDLERS("QuestGroup", URefQuestGroup);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Quest", URefQuest);
	REGISTER_REF_NO_UID_HANDLERS("QuestSequence", URefQuestSequence);
	REGISTER_REF_HANDLERS("QuestEvent", URefQuestEvent);
	REGISTER_REF_NO_UID_HANDLERS("QuestActiveSchedule", URefQuestSchedule);
	REGISTER_REF_NO_UID_HANDLERS("QuestChallenge", URefQuestChallenge);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Title", URefTitle);
	REGISTER_REF_NO_UID_HANDLERS_WITH_POSTHANDLER("Reply", URefReply);
	REGISTER_REF_HANDLERS("Reaction", URefReaction);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("UserInteraction", URefUserInteraction);

	// quest - keyword
	REGISTER_REF_HANDLERS("Quest_Keyword", URefQuest);
	REGISTER_REF_NO_UID_HANDLERS("QuestSequence_Keyword", URefQuestSequence);

	// quest - arbeit
	REGISTER_REF_HANDLERS("ArbeitQuestGroup", URefArbeitQuestGroup);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("ArbeitReward", URefArbeitReward);
	REGISTER_REF_HANDLERS("Quest_Arbeit", URefQuestArbeit);
	REGISTER_REF_NO_UID_HANDLERS("QuestSequence_Arbeit", URefQuestSequenceArbeit);

	// quest - stamp tour
	REGISTER_REF_HANDLERS("QuestGroup_StampTour", URefStampTourQuestGroup);
	REGISTER_REF_HANDLERS("Quest_StampTour", URefQuest);
	REGISTER_REF_NO_UID_HANDLERS("QuestSequence_StampTour", URefQuestSequence);

	// quest - pass
	REGISTER_REF_HANDLERS("QuestGroup_Pass", URefPassQuestGroup);
	REGISTER_REF_HANDLERS("Quest_Pass", URefQuestPass);
	REGISTER_REF_NO_UID_HANDLERS("QuestSequence_Pass", URefQuestSequence);

	// quest - revenue
	REGISTER_REF_HANDLERS("Quest_Revenue", URefQuest);
	REGISTER_REF_NO_UID_HANDLERS("QuestSequence_Revenue", URefQuestSequence);

	// streaming
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Streaming", URefStreaming);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Interview", URefInterview);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("Inspecting", URefInspecting);
	REGISTER_REF_HANDLERS("TakePhoto", URefTakePhoto);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("ItemReview", URefItemReview);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("ItemTrade", URefItemTrade);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("StreamingNPC", URefStreamingNPC);

	// dialog
	REGISTER_REF_JSON_HANDLERS_WITH_POSTHANDER("Dialog", URefDialog);

	// shop
	REGISTER_REF_HANDLERS("ShopGroup", URefShopGroup);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("ShopItem", URefShopItem);
	REGISTER_REF_HANDLERS("ShopItem_Random", URefShopItemRandom);
	REGISTER_REF_HANDLERS("ShopItem_InApp", URefShopItemInApp);
	REGISTER_REF_HANDLERS("ShopItem_Mileage", URefShopItem);
	REGISTER_REF_NO_UID_HANDLERS("ShopCost", URefShopCost);
	REGISTER_REF_HANDLERS("ShopPool", URefShopPool);
	REGISTER_REF_HANDLERS("Payment", URefPayment);

	// equip
	REGISTER_REF_NO_UID_HANDLERS("EquipAttribute", URefEquipAttribute);
	REGISTER_REF_NO_UID_HANDLERS("EquipUpgrade", URefEquipUpgrade);
	REGISTER_REF_NO_UID_HANDLERS("EquipUpgradeEffect", URefEquipUpgradeEffect);
	REGISTER_REF_NO_UID_HANDLERS("EquipCarveEffect", URefEquipCarveEffect);
	REGISTER_REF_NO_UID_HANDLERS("EquipCraft", URefEquipCraft);
	REGISTER_REF_NO_UID_HANDLERS("EquipReforge", URefEquipReforge);
	REGISTER_REF_NO_UID_HANDLERS("EquipSetEffect", URefEquipSetEffect);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("EquipCollectionGroup", URefEquipCollectionGroup);
	REGISTER_REF_HANDLERS("EquipCollection", URefEquipCollection);
	REGISTER_REF_NO_UID_HANDLERS_WITH_POSTHANDLER("Emblem_Effect", URefEmblemEffect);

	// spectating
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("SkillPatronage", URefSkillPatronage);
	REGISTER_REF_NO_UID_HANDLERS("SkillPatronageTier", URefSkillPatronageTier);
	REGISTER_REF_NO_UID_HANDLERS("SkillPatronageCost", URefSkillPatronageCost);

	// fashion contents
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("FashionContentsGroup", URefFashionContentsGroup);
	REGISTER_REF_HANDLERS("FashionContentsScore", URefFashionContentsScore);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("FashionContentsStage", URefFashionContentsStage);
	REGISTER_REF_HANDLERS("FashionContentsNPC", URefFashionContentsNPC);
	REGISTER_REF_HANDLERS("FashionContentsFactors", URefFashionContentsFactor);

	// tag
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("TagGroup", URefTagGroup);
	REGISTER_REF_HANDLERS("Tag", URefTag);

	// skill tree
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("SkillTree", URefSkillTree);
	REGISTER_REF_NO_UID_HANDLERS_WITH_POSTHANDLER("SkillTreeStep", URefSkillTreeStep);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("SkillTreeSlot", URefSkillTreeSlot);
	REGISTER_REF_HANDLERS_WITH_POSTHANDER("SkillTreeLevel", URefSkillTreeLevel);


	// legacy
	//REGISTER_REF_HANDLERS("Legacy", URefLegacy);

	if (LoadFiles() == false) {
		return false;
	}

	// skill
	InitializeSkillDataTable();

	UE_LOG(LogReference, Verbose, TEXT("UReferenceBuilder::PostProcessing"));
	URefObjectPostProcessor();

	for (auto& processor : _postProcessors) {
		processor();
	}

	URefSkillPostProcessor();
	URefSkillTimelinePostProcessor();
	URefStatPostProcessor();

	FAnuResourceWorldListPostProcessor();

	auto end = FPlatformTime::Seconds();
	UE_LOG(LogReference, Verbose, TEXT("UReferenceBuilder::Initialize completed! takes [%.2f] sec"), end - started);


	//PrintLegacyGuids(URefShopItem::StaticClass());

	return !FReferenceLogBuilder::ContainsError();
}

void UReferenceBuilder::InitializeDataTable()
{
	LoadDialogStringTable();

#if WITH_EDITOR
	ClearDialogStringTable(); // for rebuild in tool mode
#endif

	REGISTER_RESOURCE_HANDLERS("ResourceIcon", UTexture2D);
	REGISTER_RESOURCE_HANDLERS("ResourceIcon", FAnuResourceIcon);
	REGISTER_RESOURCE_HANDLERS("ResourceCamera", ACameraActor);
	REGISTER_RESOURCE_HANDLERS("ResourceCameraShake", UMatineeCameraShake);
	REGISTER_RESOURCE_HANDLERS("ResourceWorld", UWorld);
	REGISTER_RESOURCE_HANDLERS("ResourceCharacter", ACharacter);
	REGISTER_RESOURCE_HANDLERS("ResourceMesh", UAnuMesh);
	REGISTER_RESOURCE_HANDLERS("ResourceParticle", UParticleSystem);
	REGISTER_RESOURCE_HANDLERS("ResourceLevelSequence", UAnuLevelSequence);
	REGISTER_RESOURCE_HANDLERS("ResourceLevelSequence_Stage", UAnuLevelSequenceClassPose);
	REGISTER_RESOURCE_HANDLERS("ResourceSnapshot", UAnuSnapshot);
	REGISTER_RESOURCE_HANDLERS("ResourceLivePageFeed", UAnuLivePageFeed);
	REGISTER_RESOURCE_HANDLERS("ResourceElectronicBoard", UAnuElectronicBoard);
	REGISTER_RESOURCE_HANDLERS("ResourceElectronicBoardNews", UAnuElectronicBoardNews);
	REGISTER_RESOURCE_HANDLERS("ResourceArea", UAnuArea);
	REGISTER_RESOURCE_HANDLERS("ResourcePushAlarm", UAnuPushAlarm);
	REGISTER_RESOURCE_HANDLERS("ResourceActor", AActor);
	REGISTER_RESOURCE_HANDLERS("ResourceRunnable", UAnuRunnable);
	REGISTER_RESOURCE_HANDLERS("ResourceSocialMotion", UAnuSocialMotion);
	REGISTER_RESOURCE_HANDLERS("ResourceAppearancePreset", UAnuAppearancePreset);
	REGISTER_RESOURCE_HANDLERS("ResourceLifeAction", UAnuResLifeAction);
	REGISTER_RESOURCE_HANDLERS("ResourceLevelDesign", ULevelDesign);
	REGISTER_RESOURCE_HANDLERS("ResourceOriginals", UAnuOriginals);
	REGISTER_RESOURCE_HANDLERS("ResourceBellSound", UAnuBellSound);
	REGISTER_RESOURCE_HANDLERS("ResourceProfileCard", UAnuProfileCard);
	REGISTER_RESOURCE_HANDLERS("ResourceNotice", UAnuNotice);
	REGISTER_RESOURCE_HANDLERS("ResourceBehaviorTree", UBehaviorTree);
	REGISTER_RESOURCE_HANDLERS("ResourceChatTag", UAnuChatTag);
	REGISTER_RESOURCE_HANDLERS("ResourceGallery", UAnuResGallery);
	REGISTER_RESOURCE_HANDLERS("ResourceMeshTransformation", UAnuMeshTransformation);
	REGISTER_RESOURCE_HANDLERS("ResourceChatSticker", UAnuChatSticker);
	REGISTER_RESOURCE_HANDLERS("ResourceFlipBook", UAnuFlipBook);
	REGISTER_RESOURCE_HANDLERS("ResourceSound", USoundWave);
	REGISTER_RESOURCE_HANDLERS("ResourceSignDesc", FAnuResourceSignatureDesc);
	REGISTER_RESOURCE_HANDLERS("ResourceWorldServerList", UAnuWorldServerList);
	REGISTER_RESOURCE_HANDLERS("ResourceClassLicense", URefClass);
	REGISTER_RESOURCE_HANDLERS("ResourceEventSchedule", UAnuEventSchedule);
}

void UReferenceBuilder::InitializeSkillDataTable()
{
	UE_LOG(LogReference, Verbose, TEXT("UReferenceBuilder::InitializeSkillDataTable> skill"));
	_references.Add(URefSkill::StaticClass(), NewObject<UReferences>(this));
	URefSkillHandler();

	UE_LOG(LogReference, Verbose, TEXT("UReferenceBuilder::InitializeSkillDataTable> skill timeline battle"));
	_references.Add(URefSkillTimeline::StaticClass(), NewObject<UReferences>(this));
	URefSkillTimelineHandler();

	UE_LOG(LogReference, Verbose, TEXT("UReferenceBuilder::InitializeSkillDataTable> skill object"));
	_references.Add(URefSkillObject::StaticClass(), NewObject<UReferences>(this));
	URefSkillObjectHandler();
}

void UReferenceBuilder::Finalize()
{
	_refHandlers.Empty();
	_postProcessors.Empty();

	_references.Empty();
	_globals.Empty();

	_charLevels.Empty();
	_worldLookupTable.Empty();
	_calendars.Empty();
	_shopCosts.Empty();
}

TSharedPtr<class FXmlFile> UReferenceBuilder::LoadTableFile(const FString& name)
{
	auto file = MakeShared<FXmlFile>(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Anu/DataTable/References"), name));
	return file;
}

TSharedPtr<FJsonObject> UReferenceBuilder::LoadJsonFile(const FString& path)
{
	FString jsonStr;
	if (FFileHelper::LoadFileToString(jsonStr, *path) == false) {
		return nullptr;
	}

	TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(jsonStr);
	TSharedPtr<FJsonObject> jsonObj;
	if (FJsonSerializer::Deserialize(jsonReader, jsonObj) == false || jsonObj.IsValid() == false) {
		return nullptr;
	}

	return jsonObj;
}

void UReferenceBuilder::GetJsonFilePaths(const FString& tableName, TArray<FString>& paths)
{
	const FString JsonReferencePath{ GetJsonSrcDirectory() };
	const FString tablePath{ JsonReferencePath + tableName };

#if WITH_EDITOR
	IFileManager::Get().FindFilesRecursive(paths, *tablePath, TEXT("*.json"), true, false);
#else
	auto jsonIndexFile = LoadJsonFile(GetJsonIndexPath());
	checkf(jsonIndexFile, TEXT("cannot load json index file in [%s]; may you have conflicted file in local workspace?"), *GetJsonIndexPath());

	const TArray<TSharedPtr<FJsonValue>>* fileNames;
	if (jsonIndexFile->TryGetArrayField(tableName, fileNames) == false) {
		UE_LOG(LogReference, Warning, TEXT("cannot find any file for Table[%s]; is your json index file old?"), *tableName);
		return;
	}
	
	for (auto& it : *fileNames) {
		paths.Emplace(FPaths::Combine(tablePath, it->AsString()));
	}
#endif
}

bool UReferenceBuilder::LoadFiles()
{
	UE_LOG(LogReference, Verbose, TEXT("UReferenceBuilder::LoadFiles"));

	// json references
#if WITH_EDITOR
	// create json index object
	TSharedPtr<FJsonObject> jsonIndex = MakeShareable(new FJsonObject);
	check(jsonIndex);
#endif
	for (auto& val : _refJsonHandlers) {
		const FString tableName{ val.Key };
		FJsonReferenceHandler* handler = &val.Value;
		UE_LOG(LogReference, Verbose, TEXT("loading json directory.. [%s]"), *tableName);

		TArray<FString> paths;
		GetJsonFilePaths(tableName, paths);

		TArray<TSharedPtr<FJsonValue>> pathValues;
		for (const auto& path : paths) {
			const FString fileName{ FPaths::GetBaseFilename(path) };
			auto json = LoadJsonFile(path);
			if (json == nullptr) {
				checkRefMsgfCont(Error, false, TEXT("[%s] json src file[%s] format is invalid; open file and check by text editor"), *tableName, *path);
				continue;
			}
			handler->Execute(fileName, json.Get());

#if WITH_EDITOR
			const FString JsonReferencePath{ GetJsonSrcDirectory() };
			const FString tablePath{ JsonReferencePath + tableName };

			pathValues.Add(MakeShareable(new FJsonValueString(path.RightChop(tablePath.Len() + 1))));
#endif
		}

#if WITH_EDITOR
		jsonIndex->SetArrayField(tableName, pathValues);
#endif
	}

	// xml references
	for (auto& val : _refHandlers) {
		FReferenceHandler* handler = &val.Value;
		FString fileName = val.Key + ".xml";
		UE_LOG(LogReference, Verbose, TEXT("loading file.. [%s]"), *fileName);

		auto xmlFile = LoadTableFile(fileName);
		FXmlNode* root = xmlFile->GetRootNode();
		if (root == nullptr) {
			continue;
		}
		handler->Execute(root);
		xmlFile.Reset();
		UE_LOG(LogReference, Verbose, TEXT("loading completed."));
	}

#if WITH_EDITOR
	FString jsonIndexStr;
	TSharedRef<TJsonWriter<>> jsonWriter = TJsonWriterFactory<>::Create(&jsonIndexStr);
	bool jsonIndexSerialized = FJsonSerializer::Serialize(jsonIndex.ToSharedRef(), jsonWriter);
	check(jsonIndexSerialized);
	bool jsonIndexSaved = FFileHelper::SaveStringToFile(jsonIndexStr, *GetJsonIndexPath());
	check(jsonIndexSaved);
#endif
	return true;
}

void UReferenceBuilder::IterateNodes(const FXmlNode* root, TFunction<void(const FXmlNode*)> handler)
{
	for (const FXmlNode* child = root->GetFirstChildNode(); child ; child = child->GetNextNode()) {
		handler(child);
	}
}

void UReferenceBuilder::LoadResource(const TArray<FName>& iconUIDs, TArray<UTexture2D*>& output)
{
	for (auto& it : iconUIDs) {
		if (it == NAME_None) {
			break;
		}

		UTexture2D* texture = GetResource<UTexture2D>(it);
		if (texture == nullptr) {
			continue;
		}
		output.Emplace(texture);
	}
}

void UReferenceBuilder::LoadResource(const TArray<FName>& iconUIDs, TArray<TSoftObjectPtr<UTexture2D>>& output)
{
	for (auto& it : iconUIDs) {
		if (it == NAME_None) {
			break;
		}

		auto& texture = GetResourceRoute<UTexture2D>(it);

		//UTexture2D* texture = GetResource<UTexture2D>(it);
		if (texture == nullptr) {
			continue;
		}
		output.Emplace(texture);
	}
}

void UReferenceBuilder::InitializeCostumeData()
{
	DoRefIterationJob<URefItemCostume>([this](URefItemCostume* costume){
		static auto GetGenderFilledKey = [](const FString& origin, EGender gender) {
			static TMap<EGender, FString> GenderStrs{
				{EGender::Female, "F_"}, {EGender::Male, "M_"}, {EGender::Invalid, ""}
			};
			auto it = GenderStrs.Find(gender);
			check(it);
			auto key = FText::FormatNamed(FText::FromString(origin), TEXT("gender"), FText::FromString(*it));
			return FText::FormatNamed(FText::FromString(origin), TEXT("gender"), FText::FromString(*it)).ToString();
		};

		FString rscKeyStr{ costume->Resource_Key.ToString() };
		{ // name
			costume->Name.Empty();
			FString origin{ FString::Printf(TEXT("str.name.objt.costume.%s"), *rscKeyStr) };
			FString femaleKey = GetGenderFilledKey(origin, EGender::Female);
			if (UKismetStringTableLibrary::IsRegisteredTableEntry(AnuText::Get_CommonTableID(), femaleKey) == false) {
				costume->Name.Emplace(AnuText::Get_CommonTable(GetGenderFilledKey(origin, EGender::Invalid)));
			}
			else {
				checkRefMsgf(Error, UKismetStringTableLibrary::IsRegisteredTableEntry(AnuText::Get_CommonTableID(), GetGenderFilledKey(origin, EGender::Male)), TEXT("[costume] Item_Costume[%s] bound with gender-specific [Name], but not registered male data"), *costume->UID.ToString());
				costume->Name.Emplace(AnuText::Get_CommonTable(femaleKey));
				costume->Name.Emplace(AnuText::Get_CommonTable(GetGenderFilledKey(origin, EGender::Male)));
			}
		}
		{ // desc
			costume->Desc.Empty();
			FString origin{ FString::Printf(TEXT("str.desc.objt.costume.%s"), *rscKeyStr) };
			FString femaleKey = GetGenderFilledKey(origin, EGender::Female);
			if (UKismetStringTableLibrary::IsRegisteredTableEntry(AnuText::Get_CommonTableID(), femaleKey) == false) {
				costume->Desc.Emplace(AnuText::Get_CommonTable(GetGenderFilledKey(origin, EGender::Invalid)));
			}
			else {
				checkRefMsgf(Error, UKismetStringTableLibrary::IsRegisteredTableEntry(AnuText::Get_CommonTableID(), GetGenderFilledKey(origin, EGender::Male)), TEXT("[costume] Item_Costume[%s] bound with gender-specific [Desc], but not registered male data"), *costume->UID.ToString());
				costume->Desc.Emplace(AnuText::Get_CommonTable(femaleKey));
				costume->Desc.Emplace(AnuText::Get_CommonTable(GetGenderFilledKey(origin, EGender::Male)));
			}
		}
		{ // icon
			costume->Icon.Empty();
			FString origin{ FString::Printf(TEXT("res.icon.costume.%s"), *rscKeyStr) };
			FString femaleKey = GetGenderFilledKey(origin, EGender::Female);
			if (GetResourceRow<UTexture2D>(*femaleKey) == nullptr) {
				costume->Icon.Emplace(*GetGenderFilledKey(origin, EGender::Invalid));
			}
			else {
				checkRefMsgf(Error, GetResourceRow<UTexture2D>(*GetGenderFilledKey(origin, EGender::Male)), TEXT("[costume] Item_Costume[%s] bound with gender-specific [Icon], but not registered male data"), *costume->UID.ToString());
				costume->Icon.Emplace(*femaleKey);
				costume->Icon.Emplace(*GetGenderFilledKey(origin, EGender::Male));
			}
		}
		{ // model
			costume->Model.Empty();
			FString origin{ FString::Printf(TEXT("res.mesh.costume.%s"), *rscKeyStr) };
			FString femaleKey = GetGenderFilledKey(origin, EGender::Female);
			if (GetResourceRow<UAnuMesh>(*femaleKey) == nullptr) {
				costume->Model.Emplace(*GetGenderFilledKey(origin, EGender::Invalid));
			}
			else {
				checkRefMsgf(Error, GetResourceRow<UAnuMesh>(*GetGenderFilledKey(origin, EGender::Male)), TEXT("[costume] Item_Costume[%s] bound with gender-specific [Model], but not registered male data"), *costume->UID.ToString());
				costume->Model.Emplace(*femaleKey);
				costume->Model.Emplace(*GetGenderFilledKey(origin, EGender::Male));
			}
		}

		if (costume->typeId.typeID3 == TID3_NPC_BODY) {
			costume->_effectBody = GetRefObj<URefBody>(costume->Effect_UID);
			checkRefMsgf(Error, costume->_effectBody, TEXT("[costume] Item_Costume[%s] bound with invalid Body[%s] in Effect_UID"), *costume->UID.ToString(), *costume->Effect_UID.ToString());
			costume->_effectBody->_skinCostume = costume;
		}
	});
}

void UReferenceBuilder::BuildSkillTimelineEffect(URefSkillTimeline* reference)
{
	reference->_refEffect = URefSkillEffect::effectsByName.FindRef(reference->Effect);
	if (reference->_refEffect == nullptr) {
		UE_LOG(LogReference, Error, TEXT("can not find effect Type [%s]"), *reference->Effect.ToString());
		return;
	}

	switch (reference->_refEffect->FunctionEffect)
	{
	case SkillEffect::TimeScale: {
		if (reference->Value.Num() > 0) {
			auto data = NewObject<URefSkillEffectTimeScale>(reference);
			data->timeScale = FCString::Atof(*reference->Value[0]);
			reference->_value = data;
		}
	}break;
	case SkillEffect::Damage: {
		if (reference->Value.Num() >= 2) {
			auto data = NewObject<URefSkillEffectDamage>(reference);
			data->value = FCString::Atoi64(*reference->Value[0]);
			data->poise = FCString::Atoi64(*reference->Value[1]);

			// optional
			for (int32 i = 2; i < reference->Value.Num(); ++i) {
				FString optionName;
				FString optionValue;
				reference->Value[i].Split(TEXT("|"), &optionName, &optionValue, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

				if (optionName.TrimStartAndEnd() == "ExtraBossDamage") {
					data->extraBossDamage = FCString::Atoi(*optionValue.TrimStartAndEnd());
				}
				else if (optionName.TrimStartAndEnd() == "PireceRate") {
					data->pireceRate = FCString::Atof(*optionValue.TrimStartAndEnd());
				}
			}

			reference->_value = data;
		}
	}break;
	case SkillEffect::Damage_Fixed:
	case SkillEffect::MaxHP_Fixed:
	case SkillEffect::HP: {
		if (reference->Value.Num() > 0) {
			auto data = NewObject<URefSkillEffectDamage>(reference);
			data->value = FCString::Atoi64(*reference->Value[0]);
			reference->_value = data;
		}
	}break;
	case SkillEffect::Stats: {
		if (reference->Value.Num() >= 2) {
			auto data = NewObject<URefSkillEffectAttribute>(reference);
			FString statTypeName;
			FString applyTypeName;
			reference->Value[0].Split(TEXT("_"), &statTypeName, &applyTypeName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

			data->statType = URefObject::GetStatEnum(*statTypeName);
			data->applyType = SkillUtil::GetApplyTypeByString(TCHAR_TO_UTF8(*applyTypeName));
			data->statValue = FCString::Atoi(*reference->Value[1]);
			reference->_value = data;
		}
	}break;
	case SkillEffect::Impulse_Airborne: {
		if (reference->Value.Num() > 1) {
			auto data = NewObject<URefSkillEffectAirborne>(reference);
			data->speed = FCString::Atoi(*reference->Value[0]);
			data->verticalSpeed = FCString::Atoi(*reference->Value[1]);
			reference->_value = data;
		}
	}break;
	case SkillEffect::Impulse_Pull: {
		if (reference->Value.Num() > 1) {
			auto data = NewObject<URefSkillEffectPull>(reference);
			data->distance = FCString::Atof(*reference->Value[0]);
			data->speed = FCString::Atof(*reference->Value[1]);
			reference->_value = data;
		}
	}break;
	case SkillEffect::Impulse_Push: {
		if (reference->Value.Num() > 1) {
			auto data = NewObject<URefSkillEffectPush>(reference);
			data->distance = FCString::Atof(*reference->Value[0]);
			data->speed = FCString::Atof(*reference->Value[1]);
			reference->_value = data;
		}
	}break;
	case SkillEffect::MakeObject: {
		if (reference->Value.Num() >= 1) {
			auto data = NewObject<URefSkillEffectMakeObject>(reference);
			data->skillObjectUID = *reference->Value[0].TrimStartAndEnd();
			data->refSkillObject = GetRefObj<URefSkillObject>(data->skillObjectUID);
			if (data->refSkillObject == nullptr) {
				UE_LOG(LogReference, Error, TEXT("can not find skill object [%s]"), *data->skillObjectUID.ToString());
			}
			reference->_value = data;
		}
	}break;
	case SkillEffect::SpawnParticle: {
		reference->_value = NewObject<URefSkillEffectSpawnParticle>(reference);
	}break;
	case SkillEffect::LockOn: {
		if (reference->Value.Num() >= 1) {
			auto data = NewObject<URefSkillEffectLockOn>(reference);
			data->rotationSpeed = FCString::Atoi(*reference->Value[0]);
			reference->_value = data;
		}
	}break;
	case SkillEffect::Bump: {
		if (reference->Value.Num() > 2) {
			auto data = NewObject<URefSkillEffectBump>(reference);
			data->direction = FCString::Atoi(*reference->Value[0]);
			data->distance = FCString::Atof(*reference->Value[1]);
			data->speed = FCString::Atof(*reference->Value[2]);
			data->radius = reference->Value.Num() > 3 ? FCString::Atof(*reference->Value[3]) : 0;
			data->anyTarget = reference->Value.Num() > 4 ? (bool)FCString::Atoi(*reference->Value[4]) : false;
			reference->_value = data;
		}
	}break;
	case SkillEffect::Move: {
		if (reference->Value.Num() > 2) {
			auto data = NewObject<URefSkillEffectMove>(reference);
			data->direction = FCString::Atoi(*reference->Value[0]);
			data->distance = FCString::Atof(*reference->Value[1]);
			data->speed = FCString::Atof(*reference->Value[2]);
			data->radius = 0;
			reference->_value = data;
		}
	}break;
	case SkillEffect::Warp: {
		if (reference->Value.Num() > 0) {
			auto data = NewObject<URefSkillEffectWarp>(reference);
			data->effectType = reference->Value[0];

			if (data->effectType == "World") {
				data->x = FCString::Atof(*reference->Value[1]);
				data->y = FCString::Atof(*reference->Value[2]);
				data->yaw = FCString::Atof(*reference->Value[3]);
			}

			reference->_value = data;
		}
	}break;
	case SkillEffect::State: {
		auto stateEft = NewObject<URefSkillEffectChangeState>(reference);
		uint8 blockState = SkillUtil::GetBlockState(reference->GetType());
		if (blockState != 0) {
			stateEft->states.Emplace(SkillUtil::EffectState::Block, blockState);
		}
		uint8 bodyState = SkillUtil::GetBodyState(reference->GetType());
		if (bodyState != 0) {
			stateEft->states.Emplace(SkillUtil::EffectState::Body, bodyState);
		}

		if (reference->Value.Num() > 0) {
			reference->_ignoreImmun = reference->Value[0] == "IgnoreImmun";
		}

		reference->_value = stateEft;
	}break;
	case SkillEffect::Dispel: {
		auto data = NewObject<URefSkillEffectDispel>(reference);
		if (reference->Value.Num() > 0) {
			data->matchType = SkillUtil::GetDispelMatchTypeByString(TCHAR_TO_UTF8(*reference->Value[0]));
		}
		if (reference->Value.Num() > 1) {
			switch (data->matchType)
			{
				case DispelMatchType::Group:
					data->targetSkillGroup = SkillUtil::GetEffectGroupByString(TCHAR_TO_UTF8(*reference->Value[1]));
					break;
				case DispelMatchType::Skill:
					FName skillUID = FName(*reference->Value[1]);
					auto refSkill = _references.FindRef(URefSkill::StaticClass())->GetReference(skillUID);
					data->targetSkillGUID = refSkill->GUID;
					break;
			}
		}
		reference->_value = data;
	}break;
	case SkillEffect::CoolDown: {
		if (reference->Value.Num() > 1) {
			auto data = NewObject<URefSkillEffectCoolDown>(reference);
			data->effectType = reference->Value[0];
			data->value = FCString::Atof(*reference->Value[1]);
			reference->_value = data;
		}
	}break;
	case SkillEffect::Score_Per: {
		auto data = NewObject<URefSkillEffectScore>(reference);
		data->TagType = GetRefObj<URefTag>(reference->Value[0]);
		checkRef(Error, data->TagType);
		data->value = FCString::Atoi(*reference->Value[1]);
		reference->_value = data;

	}break;
	case SkillEffect::CounterSkill: {
		if (reference->Value.Num() >= 4) {
			auto data = NewObject<URefSkillEffectCounterSkill>(reference);
			FString statTypeName;
			FString applyTypeName;
			reference->Value[0].Split(TEXT("_"), &statTypeName, &applyTypeName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

			data->statType = URefObject::GetStatEnum(*statTypeName);
			data->applyType = SkillUtil::GetApplyTypeByString(TCHAR_TO_UTF8(*applyTypeName));
			data->statValue = FCString::Atoi(*reference->Value[1]);

			data->successSkill = GetRefObj<URefSkill>(reference->Value[2]);
			data->failureSkill = GetRefObj<URefSkill>(reference->Value[3]);

			reference->_value = data;
		}
	}break;
	case SkillEffect::Immune: {
		auto data = NewObject<URefSkillEffectImmune>(reference);
		for (auto& one : reference->Value) {
			auto targetEffect = GetRefObj<URefSkillTimeline>(one);
			checkRef(Error, targetEffect);
			data->targets.Emplace(targetEffect);
		}
		reference->_value = data;
	}break;
	default: {
		UE_LOG(LogReference, Warning, TEXT("Skill timeline[%s] Effect[%s] is not define.  If u really want it?"), *reference->UID.ToString(), *reference->Effect.ToString());
	}break;
	}
}

TSharedPtr<class FJsonObject> UReferenceBuilder::LoadQuestEventJson(const FName& fileName, bool useCache /*= true*/)
{
	static FString QUEST_EVENT_JSON_DIR = FPaths::ProjectContentDir() + TEXT("Anu/QuestJsonSrc/");
	TSharedPtr<FJsonObject> eventJson = nullptr;
	if (useCache) {
		auto jsonIter = _questEventJsonCache.Find(fileName);
		if (jsonIter != nullptr) {
			eventJson = *jsonIter;
		}
	}

	if (eventJson == nullptr) {
		eventJson = LoadJsonFile(QUEST_EVENT_JSON_DIR + fileName.ToString() + ".json");
		if (eventJson == nullptr) {
			return nullptr;
		}

		if (useCache) {
			checkRefMsgf(Error, eventJson, TEXT("QuestSequence Event[%s] json file load fail"), *fileName.ToString());
			_questEventJsonCache.Emplace(fileName, eventJson);
		}
	}

	return eventJson;
}

void UReferenceBuilder::ResourceHandler(const FString& tableName, UStruct* clazz)
{
	static FString ResourceTableDir{ "/Game/Anu/DataTable/References/Resource/" };
	FString assetName{ "DT_" + tableName };
	FString assetFullPath{ ResourceTableDir + assetName + "." + assetName };
	UDataTable* dt = LoadObject<UDataTable>(this, *assetFullPath);
	checkRefMsgfRet(Error, dt, TEXT("resource table[%s] not exist in [%s]"), *tableName, *assetFullPath);
	_resourceTables.Emplace(clazz, dt);
}


bool UReferenceBuilder::GetRefGlobal(const FName& key, int32& value)
{
	FString* refValue = _globals.Find(key);
	if (refValue == nullptr) {		
		return false;
	}

	return FDefaultValueHelper::ParseInt(*refValue, value);
}

bool UReferenceBuilder::GetRefGlobal(const FName& key, float& value)
{
	FString* refValue = _globals.Find(key);
	if (refValue == nullptr) {
		return false;
	}

	return FDefaultValueHelper::ParseFloat(*refValue, value);
}

bool UReferenceBuilder::GetRefGlobal(const FName& key, FString& value)
{
	FString* refValue = _globals.Find(key);
	value = (refValue) ? *refValue : TEXT("None");
	return (refValue != nullptr);
}

URefLevelPC* UReferenceBuilder::GetRefLevelPCByExp(int32 exp)
{
	int32 count = _charLevels.Num();

	for (int32 i = count - 1; i >= 0; --i)
	{
		URefLevelPC* reference = _charLevels[i];
		if (reference->Exp <= exp) {
			return reference;
		}
	}

	return nullptr;
}

URefLevelPC* UReferenceBuilder::GetRefLevelPCByLevel(int32 level)
{
	int32 count = _charLevels.Num();

	for (int32 i = count - 1; i >= 0; --i)
	{
		URefLevelPC* reference = _charLevels[i];
		if (reference->Level == level) {
			return reference;
		}
	}

	return nullptr;
}

void UReferenceBuilder::GetQuestBySubGroup(const FName& subGroupKey, TArray<URefQuest*>& questList)
{
	auto it = _questBySubGroup.Find(subGroupKey);
	if (it == nullptr) {
		return;
	}

	questList.Append(*it);
}

void UReferenceBuilder::GetUIDs(TSubclassOf<URefBase> clazz, TArray<FName>& uids)
{
	if (UReferences* references = _references.FindRef(clazz)) {
		for (auto& item : *references->GetValues())	{
			uids.Emplace(item.Value->UID);
		}
	}
}

#if WITH_EDITOR
void UArchiveInstance::AddRefObject(UObject* object)
{
	_archive.references.Add(object);
}

void UArchiveInstance::Export(FName fileName, UClass* specifiedClass)
{
	UReferenceBuilder::SaveReference(fileName, _archive, specifiedClass);
}

bool UReferenceBuilder::ExportSpawner(TArray<ARefInteractor*> spawnActors)
{
	if (spawnActors.Num() <= 0) {
		FString msg = FString::Printf(TEXT("there is no spawners on this world"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(msg));
		return false;
	}

	for (auto& spawner : spawnActors) { spawner->GenerateCRC();  }
	spawnActors.Sort([](const ARefInteractor& a, const ARefInteractor& b) {
		return a.UID < b.UID;
	});

	FArchiveReferences archive;
	static auto GetGroundLocation = [](AActor* worldContextActor, const FVector& originLocationInWorld, bool& hit) ->FVector{
		FVector startLocation{ originLocationInWorld };
		startLocation.Z += 30.f;

		constexpr float DEFAULT_LINE_TRACE_BOTTOM_LIMIT{ -1000.f };
		FVector endLocation{ 0.f, 0.f, DEFAULT_LINE_TRACE_BOTTOM_LIMIT };
		endLocation += originLocationInWorld;

		static ETraceTypeQuery TRACE_TYPE_IKTRACE{ ETraceTypeQuery::TraceTypeQuery3 };
		FHitResult result;
		hit = UKismetSystemLibrary::LineTraceSingle(worldContextActor, startLocation, endLocation, TRACE_TYPE_IKTRACE, true, {}, EDrawDebugTrace::None, result, true);
		if (hit == false) {
			return originLocationInWorld;
		}
		return result.Location;
	};

	TSet<UClass*> customAdded;
	TSet<FName> spawnPinKeys;
	TArray<FName> invalidFK;
	archive.references.Reserve(spawnActors.Num());
	for(auto& spawner : spawnActors) {
		spawner->GUID = UCRC32::GetPtr()->Generate32(*spawner->UID);

		if (spawner->EnableExport() == false) {
			continue;
		}

		if (spawner->IsEditorOnly()) {
			continue;
		}

		if (customAdded.Find(spawner->GetClass()) == nullptr) {
			spawner->AddCustomBuilder(archive);
			customAdded.Emplace(spawner->GetClass());
		}

		FString error;
		bool fkValid = spawner->CheckFK(error, [&spawnPinKeys](ARefInteractor* interactor) {
			FName pinKey{interactor->GetPinKey()};
			if (pinKey.IsNone() == false && spawnPinKeys.Find(pinKey) != nullptr) {
				UE_LOG(LogReference, Error, TEXT("[exporter] actor[%s]  has duplicate pin[%s]"), *interactor->GetName(), *pinKey.ToString());
				return false;
			}
			return true;
		});

		if(fkValid == false) {
			invalidFK.Add(spawner->GetFName());
			UE_LOG(LogReference, Error, TEXT("%s"), *error);
			continue;
		}

		FName pinKey{spawner->GetPinKey()};
		if (pinKey.IsNone() == false) {
			spawnPinKeys.Add(pinKey);
		}

		spawner->Spawn_Rotation = spawner->GetActorRotation().Yaw;

		archive.references.Add(spawner);
	}
	
	if (invalidFK.Num() > 0) {
		FString msg = FString::Printf(TEXT("failed to build reference file"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(msg));

		FString failedStr = FString::Printf(TEXT("Check Fail Reasons in output log!\n< Invalid Actor List >\n"));
		for (int32 i = 0; i < invalidFK.Num(); ++i) {
			failedStr = FString::Printf(TEXT("%s [%s]\n"), *failedStr, *(invalidFK[i].ToString()));
		}
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(failedStr));
		return false;
	}

	UWorld* world = GEditor->GetEditorWorldContext().World();
	FString saving{ FString::Printf(TEXT("%s%s"), *UReferenceBuilder::SpawnPathPrefix, *(world->GetFName().ToString())) };
	UReferenceBuilder::SaveReference(FName(*saving), archive, ARefInteractor::StaticClass());

	FString msg{ FString::Printf(TEXT("exported reference file [%s]"), *saving) };
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(msg));
	return true;
}

void UReferenceBuilder::ExportWorldLookupTarget(UWorld* world, const TArray<AActor*>& targetActors)
{
	FString content{ "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<WorldLookUps>\n</WorldLookUps>" };

	auto file = MakeShared<FXmlFile>(content, EConstructMethod::ConstructFromBuffer);
	FXmlNode* rootNode = file->GetRootNode();

	struct FWorldLookUpData {
		FName UID;
		FName Type;
	};

	using LookUpTable = TMap<FName, TSet<FName>>; // type, <uids>
	static auto Add = [](const FWorldLookUpData& data, LookUpTable& uniqueChecker, TArray<FWorldLookUpData>& targets) {
		if (data.UID.IsNone()) {
			return;
		}

		auto& uids = uniqueChecker.FindOrAdd(data.Type);
		if (uids.Find(data.UID)) {
			return;
		}

		targets.Emplace(data);
		uids.Emplace(data.UID);
	};

	LookUpTable uniqueChecker;
	FWorldLookUpData data;
	TArray<FWorldLookUpData> targetDatas;
	for (auto& actor : targetActors) {
		if (auto spawner = Cast<ARefInteractor>(actor)) {
			data.Type = URefObject::NAME_Object;
			FName uid{ spawner->SpawnObject_UID };
			data.UID = uid;
			Add(data, uniqueChecker, targetDatas);

			if (uid != spawner->GetSpawnObjectUID()) {
				data.UID = spawner->GetSpawnObjectUID();
				Add(data, uniqueChecker, targetDatas);
			}
		}

		auto targetables = actor->GetComponentsByInterface(UWorldLookupTarget::StaticClass());
		for (auto& targetable : targetables) {
			data.Type = IWorldLookupTarget::Execute_GetLookupType(targetable);
			data.UID = IWorldLookupTarget::Execute_GetLookupUID(targetable);
			Add(data, uniqueChecker, targetDatas);
		}
	}

	targetDatas.Sort([](const FWorldLookUpData& lhs, const FWorldLookUpData& rhs) {
		return lhs.UID.ToString() < rhs.UID.ToString();
	});

	for (auto& targetData : targetDatas) {
		rootNode->AppendChildNode("WorldLookUp", TEXT(""));
		TArray<FXmlNode*> childrens = rootNode->GetChildrenNodes();
		FXmlNode* appended = childrens[childrens.Num() - 1];
		TArray<FXmlAttribute>& attrs = const_cast<TArray<FXmlAttribute>&>(appended->GetAttributes());
		attrs.Add(FXmlAttribute("UID", targetData.UID.ToString()));
		attrs.Add(FXmlAttribute("Type", targetData.Type.ToString()));
	}

	FString targetPath{ GetWorldLookupFilePath(world->GetFName().ToString()) };
	file->Save(targetPath);
}

void UReferenceBuilder::SaveReference(FName table, FArchiveReferences& archive, UClass* specifiedClass)
{
	static TSet<UClass*> NativeSpawnerClases {
		ARefInteractor::StaticClass(),
		ARefScheduleActorInteractor::StaticClass(),
		ARefNPCInteractor::StaticClass(),
		ARefMonInteractor::StaticClass(),
		ARefGimmickInteractor::StaticClass(),
		ARefClientInteractor::StaticClass(),
	};

	const FString xmlTemplate = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
	FString content = FString::Printf(TEXT("%s<%ss>\n</%ss>"), *xmlTemplate, *table.ToString(), *table.ToString());

	auto file = MakeShared<FXmlFile>(content, EConstructMethod::ConstructFromBuffer);
	FXmlNode* rootNode = file->GetRootNode();

	for (UObject* reference : archive.references) {
		rootNode->AppendChildNode(table.ToString(), TEXT(""));

		TArray<FXmlNode*> childrens = rootNode->GetChildrenNodes();
		FXmlNode* appended = childrens[childrens.Num() - 1];
		TArray<FXmlAttribute>& attrs = const_cast<TArray<FXmlAttribute>&>(appended->GetAttributes());

		// run property builder
		for (TFieldIterator<FProperty> it(reference->GetClass()); it; ++it) {
			FProperty* prop = *it;
			auto builder = archive.propertyBuilders.Find(prop->GetFName());
			if (builder) {
				(*builder)(attrs, reference);
				continue;
			}

			UClass* ownerClass = prop->GetOwnerClass();
			if (specifiedClass && ownerClass->IsChildOf(specifiedClass) == false) {
				continue;
			}

			if (NativeSpawnerClases.Find(ownerClass) == nullptr) {
				continue;
			}

			void* propValue = prop->ContainerPtrToValuePtr<void*>(reference);
			if (auto numProp = CastField<FNumericProperty>(prop))  {
				attrs.Add(FXmlAttribute(prop->GetName(), numProp->GetNumericPropertyValueToString(propValue)));
				continue;
			}
			else if (auto strProp = CastField<FStrProperty>(prop)) {
				attrs.Add(FXmlAttribute(prop->GetName(), *(FString*)propValue));
				continue;
			}
			else if (auto nameProp = CastField<FNameProperty>(prop)) {

				attrs.Add(FXmlAttribute(prop->GetName(), (*(FName*)propValue).ToString()));
				continue;
			}
		}

		// run additional builder
		for (auto& additionalBuilder : archive.additionalBuilders) {
			additionalBuilder.Value(attrs, reference);
		}
	}

	FString contentDirectory = FPaths::ProjectContentDir();
	FString fileName = FString::Printf(TEXT("%s.xml"), *table.ToString());
	FString targetPath = FPaths::Combine(contentDirectory, TEXT("ExportedReferences"), TEXT("Spawner"), *fileName);
	file->Save(targetPath);
}

void UReferenceBuilder::ExecuteSkillTimelineHandler(const FXmlNode* root)
{
	URefSkillTimelineHandler();
}

void UReferenceBuilder::ExecuteSkillTimelinePostProcessor()
{
	URefSkillTimelinePostProcessor();
}
#endif // #if WITH_EDITOR

void UReferenceBuilder::AddDebugReference(URefBase* reference, TSet<UClass*>&& additionalCacheClasses)
{
	auto references = _references.FindRef(reference->GetClass());
	references->AddReference(reference);

	for (auto& cls : additionalCacheClasses) {
		auto clsCache = _references.FindRef(cls);
		clsCache->AddReference(reference);
	}
}


int64 UReferenceBuilder::GetNPCLevelStartExpOffset(int32 lv)
{
	lv = FMath::Clamp(lv, 0, _npcLvTable.Num() - 1);
	return _npcLvTable[lv];
}


void UReferenceBuilder::GetReply(const FName& replyUID, TArray<URefReply*>& out)
{
	if (auto replyIter = _replys.Find(replyUID)) {
		out.Append(*replyIter);
	}
}

int32 UReferenceBuilder::GetGuid(const FName& tableName, const FName& uid) const
{
	auto it = _refClasses.Find(tableName);
	if (it == nullptr) {
		return INDEX_NONE;
	}

	auto references = _references.Find(*it);
	if (references == nullptr) {
		return INDEX_NONE;
	}

	auto refByUID = (*references)->GetReference(uid);
	return refByUID ? refByUID->GUID : INDEX_NONE;
}

URefRegion* UReferenceBuilder::GetTargetRegion(const FName& expectedType, const FName& uid) const
{
	auto it = _worldLookupTable.Find(expectedType);
	return it ? it->FindRef(uid) : nullptr;
}

URefClass* UReferenceBuilder::GetClassByTid2(const FName& tid2) const
{
	auto it = _classByTID2.Find(tid2);
	return it ? *it : nullptr;
}

const TArray<URefShopCost*>* UReferenceBuilder::GetShopCosts(const FName& shopCostUID) const
{
	return _shopCosts.Find(shopCostUID);
}

void UReferenceBuilder::ReserveQuestEvent(const FName& uid, const FName& type, const FString& typeValue)
{
	URefQuestEvent* evt = NewObject<URefQuestEvent>(this);
	evt->UID = uid;
	evt->GUID = UCRC32::GetPtr()->Generate32(evt->UID);
	evt->Type = type;
	evt->Type_Value = typeValue;
	auto references = _references.FindRef(URefQuestEvent::StaticClass());
	references->AddReference(evt);
}