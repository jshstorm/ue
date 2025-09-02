#include "ReferenceBuilder.h"
#include "Dom/JsonObject.h"

void UReferenceBuilder::RefTypeNameHandler(const FXmlNode* root)
{
	IterateNodes(root, [this](const FXmlNode* child) {
		_typeNames.Add(*child->GetAttribute("Name"), FCString::Atoi(*child->GetAttribute("ID")));
	});
}

void UReferenceBuilder::URefLegacyHandler(const FXmlNode* root)
{
	auto legacys = _references.FindRef(URefLegacy::StaticClass());
	IterateNodes(root, [this, legacys](const FXmlNode* child) {
		URefLegacy* reference = NewObject<URefLegacy>(this);
		reference->Parse(child);
		legacys->AddReference(reference);
	});
}

void UReferenceBuilder::PrintLegacyGuids(UClass* tgtRefClass)
{
	FString legacyGuids;

	auto valids = _references.FindRef(tgtRefClass);
	auto legacy = _references.FindRef(URefLegacy::StaticClass());
	auto invalids = legacy->GetValues();
	for (auto& it : *invalids) {
		int32 invalidGuid = it.Key;
		if (valids->GetReference(invalidGuid) == nullptr) {
			legacyGuids += FString::FromInt(invalidGuid);
			legacyGuids += ", ";
		}
	}

	UE_LOG(LogReference, Error, TEXT("[debug] legacy guids[%s]"), *legacyGuids);
}

void UReferenceBuilder::URefPCHandler(const FXmlNode* root)
{
	auto pcs = _references.FindRef(URefPC::StaticClass());
	auto chars = _references.FindRef(URefCharacter::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, pcs, chars, objs](const FXmlNode* child) {
		URefPC* reference = NewObject<URefPC>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		pcs->AddReference(reference);
		chars->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefNPCHandler(const FXmlNode* root)
{
	auto npcs = _references.FindRef(URefNPC::StaticClass());
	auto chars = _references.FindRef(URefCharacter::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, npcs, chars, objs](const FXmlNode* child) {
		URefNPC* reference = NewObject<URefNPC>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		npcs->AddReference(reference);
		chars->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefMonsterHandler(const FXmlNode* root)
{
	auto monsters = _references.FindRef(URefMonster::StaticClass());
	auto chars = _references.FindRef(URefCharacter::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, monsters, chars, objs](const FXmlNode* child) {
		URefMonster* reference = NewObject<URefMonster>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		monsters->AddReference(reference);
		chars->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefItemHandler(const FXmlNode* root)
{
	auto items = _references.FindRef(URefItem::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, items, objs](const FXmlNode* child) {
		URefItem* reference = NewObject<URefItem>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		items->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefItemEquipHandler(const FXmlNode* root)
{
	auto equips = _references.FindRef(URefItemEquip::StaticClass());
	auto items = _references.FindRef(URefItem::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, equips, items, objs](const FXmlNode* child) {
		URefItemEquip* reference = NewObject<URefItemEquip>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		equips->AddReference(reference);
		items->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefItemCostumeHandler(const FXmlNode* root)
{
	auto costumes = _references.FindRef(URefItemCostume::StaticClass());
	auto items = _references.FindRef(URefItem::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, costumes, items, objs](const FXmlNode* child) {
		URefItemCostume* reference = NewObject<URefItemCostume>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		costumes->AddReference(reference);
		items->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefItemEmblemHandler(const FXmlNode* root)
{
	auto emblems = _references.FindRef(URefItemEmblem::StaticClass());
	auto items = _references.FindRef(URefItem::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, emblems, items, objs](const FXmlNode* child) {
		URefItemEmblem* reference = NewObject<URefItemEmblem>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		emblems->AddReference(reference);
		items->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefItemUsableHandler(const FXmlNode* root)
{
	auto& dyeings = _references.FindOrAdd(URefItemDyeing::StaticClass());
	if (dyeings == nullptr) {
		dyeings = NewObject<UReferences>(this);
	}

	auto usables = _references.FindRef(URefItemUsable::StaticClass());
	auto items = _references.FindRef(URefItem::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, dyeings, usables, items, objs](const FXmlNode* child) {
		URefItemUsable* reference = nullptr;
		FName tid3 = *child->GetAttribute("TID_3");
		if (tid3 == URefItemDyeing::TID3) {
			reference = NewObject<URefItemDyeing>(this);
			reference->ParseTypeID(child, _typeNames);
			reference->Parse(child);

			dyeings->AddReference(reference);
		}
		else {
			reference = NewObject<URefItemUsable>(this);
			reference->ParseTypeID(child, _typeNames);
			reference->Parse(child);
		}

		usables->AddReference(reference);
		items->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefItemQuestHandler(const FXmlNode* root)
{
	auto questItems = _references.FindRef(URefItemQuest::StaticClass());
	auto items = _references.FindRef(URefItem::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, questItems, items, objs](const FXmlNode* child) {
		URefItemQuest* reference = NewObject<URefItemQuest>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		questItems->AddReference(reference);
		items->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefItemEtcHandler(const FXmlNode* root)
{
	auto etcs = _references.FindRef(URefItemEtc::StaticClass());
	auto items = _references.FindRef(URefItem::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, etcs, items, objs](const FXmlNode* child) {
		URefItemEtc* reference = NewObject<URefItemEtc>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		etcs->AddReference(reference);
		items->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefLifeObjectHandler(const FXmlNode* root)
{
	auto lifeObjs = _references.FindRef(URefLifeObject::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, lifeObjs, objs](const FXmlNode* child) {
		URefLifeObject* reference = NewObject<URefLifeObject>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		lifeObjs->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefPortalHandler(const FXmlNode* root)
{
	auto portals = _references.FindRef(URefPortal::StaticClass());
	auto lifeObjs = _references.FindRef(URefLifeObject::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [this, portals, lifeObjs, objs](const FXmlNode* child) {
		URefPortal* reference = NewObject<URefPortal>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		portals->AddReference(reference);
		lifeObjs->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefGimmickHandler(const FXmlNode* root)
{
	auto gimmicks = _references.FindRef(URefGimmick::StaticClass());
	auto objs = _references.FindRef(URefObject::StaticClass());

	IterateNodes(root, [&](const FXmlNode* child) {
		auto reference = NewObject<URefGimmick>(this);
		reference->ParseTypeID(child, _typeNames);
		reference->Parse(child);

		gimmicks->AddReference(reference);
		objs->AddReference(reference);
	});
}

void UReferenceBuilder::URefCharacterStatHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefCharacterStat::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefCharacterStat>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefClassHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefClass::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefClass>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefRegionHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefRegion::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child){
		auto reference = NewObject<URefRegion>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSkillHandler()
{
	FString tablePath{ DATATABLE_SKILL_DIR + TEXT("RefSkill.RefSkill") };
	UDataTable* table = LoadObject<UDataTable>(this, *tablePath);
	TArray<FRefSkillDataBase*> rows;
	table->GetAllRows("", rows);

	auto references = _references.FindRef(URefSkill::StaticClass());
	auto& ref = _refGroups.FindOrAdd(URefSkill::StaticClass());

	for (FRefSkillDataBase* row : rows) {
		auto reference = NewObject<URefSkill>(this);
		reference->Parse(row->StaticStruct(), row);
		reference->GUID = UCRC32::GetPtr()->Generate32(reference->UID);
		references->AddReference(reference);

		auto& groupData = ref.FindOrAdd(reference->Skill_Group.SkillGroupValue);
		groupData.Emplace(reference);
	}

	for (auto& pair : *references->GetValues()) {
		auto baseSkillRef = Cast<URefSkill>(pair.Value);
		TArray<FString> strValues;
		URefBase::GetTrimedStringArray(baseSkillRef->Display_Tag.ToString(), strValues);
		for (auto& value : strValues) {
			value.TrimStartAndEndInline();
			if (value != "None") {
				baseSkillRef->_displayTags.Emplace(AnuText::Get_CommonTable(value));
			}
		}

		if (baseSkillRef->InputType != ESkillInputType::Basic) {
			continue;
		}

		FString baseUID = baseSkillRef->UID.ToString();
		int32 comboIndex = FCString::Atoi(*baseUID.Right(1));
		baseUID.RemoveAt(baseUID.Len() - 1);

		while (true) {
			FString nextUID = baseUID;
			nextUID.AppendInt(++comboIndex);
			if (auto nextSkillRef = Cast<URefSkill>(references->GetReference(nextUID))) {
				nextSkillRef->BaseSkill = baseSkillRef;
				baseSkillRef->ComboSkills.Emplace(nextSkillRef);
			}
			else break;
		}
	}
}

void UReferenceBuilder::URefSkillObjectHandler()
{
	FString tablePath{ DATATABLE_SKILL_DIR + TEXT("RefSkillObject.RefSkillObject") };
	UDataTable* table = LoadObject<UDataTable>(this, *tablePath);
	checkRefMsgfRet(Error, table, TEXT("cannot find RefSkillObject table; path[%s]"), *tablePath);

	TArray<FRefSkillObjectDataBase*> rows;
	table->GetAllRows("", rows);

	auto references = _references.FindRef(URefSkillObject::StaticClass());
	checkRefMsgfRet(Error, references, TEXT("skill object type cache failed"));

	auto objs = _references.FindRef(URefObject::StaticClass());
	checkRefMsgfRet(Error, objs, TEXT("object type cache failed"));

	for (FRefSkillObjectDataBase* row : rows) {
		auto reference = NewObject<URefSkillObject>(this);
		reference->Parse(row->StaticStruct(), row);
		reference->ResolveTypeID(_typeNames);
		reference->GUID = UCRC32::GetPtr()->Generate32(reference->UID);
		references->AddReference(reference);
		objs->AddReference(reference);
	}
}

void UReferenceBuilder::URefSkillEffectHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefSkillEffect::StaticClass());
	URefSkillEffect::effectsByName.Empty();
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSkillEffect>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSkillTimelineHandler()
{
	FString tablePath{ DATATABLE_SKILL_DIR + TEXT("RefSkillTimeline.RefSkillTimeline") };
	UDataTable* table = LoadObject<UDataTable>(this, *tablePath);
	auto references = _references.FindRef(URefSkillTimeline::StaticClass());
	references->Reset();

	auto sklRefs = _references.FindRef(URefSkill::StaticClass());
	for (auto& pair : *sklRefs->GetValues()) {
		auto refSkill = Cast<URefSkill>(pair.Value);
		refSkill->ClearTimelines();
	}

	TArray<FRefSkillTimelineDataBase*> rows;
	table->GetAllRows("", rows);

	for (FRefSkillTimelineDataBase* row : rows) {
		auto reference = NewObject<URefSkillTimeline>(this);
		reference->Parse(row->StaticStruct(), row);
		reference->GUID = UCRC32::GetPtr()->Generate32(reference->UID);
		references->AddReference(reference);
		URefSkill* skill = Cast<URefSkill>(sklRefs->GetReference(row->Skill_UID));
		reference->_refSkill = skill;
		if (skill == nullptr) {
			continue; // for tool
		}

		skill->AddTimeline(reference);

		if (skill->Duration == 0 || skill->Duration < row->Duration_Absolute) {
			skill->Duration = row->Duration_Absolute;
		}
	}

	DoRefIterationJob<URefSkill>([](URefSkill* reference) {
		reference->Init();
	});
}

void UReferenceBuilder::URefStageGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefStageGroup::StaticClass());
	IterateNodes(root, [references, this](const FXmlNode* child) {
		auto reference = NewObject<URefStageGroup>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefStageInfoHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefStageInfo::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefStageInfo>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefStageContestHandler(const FXmlNode* root)
{
	URefStageContest::map.Empty();

	auto references = _references.FindRef(URefStageContest::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefStageContest>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefContestDonationGuideHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefContestDonationGuide::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefContestDonationGuide>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefCurrencyHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefCurrency::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child){
		auto reference = NewObject<URefCurrency>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefCurrencyStressHandler(const FXmlNode* root)
{
	auto currencies = _references.FindRef(URefCurrency::StaticClass());
	auto references = _references.FindRef(URefCurrencyStress::StaticClass());
	IterateNodes(root, [this, currencies, references](const FXmlNode* child) {
		auto reference = NewObject<URefCurrencyStress>(this);
		reference->Parse(child);
		references->AddReference(reference);
		currencies->AddReference(reference);
	});
}

void UReferenceBuilder::URefShopGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefShopGroup::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefShopGroup>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefShopItemHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefShopItem::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefShopItem>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefShopItemInAppHandler(const FXmlNode* root)
{
	auto shopItems = _references.FindRef(URefShopItem::StaticClass());
	auto references = _references.FindRef(URefShopItemInApp::StaticClass());
	IterateNodes(root, [this, references, shopItems](const FXmlNode* child) {
		auto reference = NewObject<URefShopItemInApp>(this);
		reference->Parse(child);
		references->AddReference(reference);

		shopItems->AddReference(reference);
	});
}

void UReferenceBuilder::URefShopItemRandomHandler(const FXmlNode* root)
{
	auto shopItems = _references.FindRef(URefShopItem::StaticClass());
	auto references = _references.FindRef(URefShopItemRandom::StaticClass());
	IterateNodes(root, [this, references, shopItems](const FXmlNode* child) {
		auto reference = NewObject<URefShopItemRandom>(this);
		reference->Parse(child);
		references->AddReference(reference);

		shopItems->AddReference(reference);
	});
}

void UReferenceBuilder::URefShopCostHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefShopCost::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefShopCost>(this);
		reference->Parse(child);
		references->AddReference(reference);

		auto& shopCosts = _shopCosts.FindOrAdd(reference->Cost_UID);
		shopCosts.Emplace(reference);
	});
}

void UReferenceBuilder::URefShopPoolHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefShopPool::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefShopPool>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefPaymentHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefPayment::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefPayment>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefWorldHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefWorld::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefWorld>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefGlobalHandler(const FXmlNode* root)
{
	IterateNodes(root, [this](const FXmlNode* child) {
		URefGlobal* reference = URefGlobal::StaticParse(child);
		_globals.Emplace(*reference->Key, reference->Value);
	});
}

void UReferenceBuilder::URefClassLevelHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefClassLevel::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefClassLevel>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}


void UReferenceBuilder::URefClassLicenseMasteryHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefClassLicenseMastery::StaticClass());
	IterateNodes(root, [references, this](const FXmlNode* child) {
		auto reference = NewObject<URefClassLicenseMastery>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefLevelPCHandler(const FXmlNode* root)
{
	IterateNodes(root, [this](const FXmlNode* child) {
		URefLevelPC* reference = NewObject<URefLevelPC>(this);
		reference->Parse(child);
		checkRefRet(Error, reference->Currency_MAX.Num() == reference->Currency_MAX_Value.Num());

		for (int32 i = 0; i < reference->Currency_MAX.Num(); ++i) {
			reference->_statCurrencyValues.Add(URefObject::GetStatEnum(reference->Currency_MAX[i]), reference->Currency_MAX_Value[i]);
		}

		_charLevels.Add(reference);
	});
}

void UReferenceBuilder::URefLevelNPCHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefLevelNPC::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefLevelNPC>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefChatStickerGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefChatStickerGroup::StaticClass());
	auto& groupRef = _refGroups.FindOrAdd(URefChatStickerGroup::StaticClass());

	IterateNodes(root, [references, this, &groupRef](const FXmlNode* child) {
		auto reference = NewObject<URefChatStickerGroup>(this);
		reference->Parse(child);
		references->AddReference(reference);
		auto& groupData = groupRef.FindOrAdd(reference->UID);
		groupData.Emplace(reference);
	});
}

void UReferenceBuilder::URefResourceCoreHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefResourceCore::StaticClass());

	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefResourceCore* reference = NewObject<URefResourceCore>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefFashionContentsScoreHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefFashionContentsScore::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefFashionContentsScore* reference = NewObject<URefFashionContentsScore>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefFashionContentsFactorHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefFashionContentsFactor::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefFashionContentsFactor* reference = NewObject<URefFashionContentsFactor>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefFashionContentsGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefFashionContentsGroup::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefFashionContentsGroup* reference = NewObject<URefFashionContentsGroup>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefFashionContentsStageHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefFashionContentsStage::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefFashionContentsStage* reference = NewObject<URefFashionContentsStage>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefFashionContentsNPCHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefFashionContentsNPC::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefFashionContentsNPC* reference = NewObject<URefFashionContentsNPC>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefTagGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefTagGroup::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefTagGroup* reference = NewObject<URefTagGroup>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefTagHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefTag::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefTag* reference = NewObject<URefTag>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefRewardStaticHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefRewardStatic::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefRewardStatic* reference = NewObject<URefRewardStatic>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefRewardRandomHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefRewardRandom::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefRewardRandom* reference = NewObject<URefRewardRandom>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefLifeRewardHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefLifeReward::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefLifeReward* reference = NewObject<URefLifeReward>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefRewardSelectableHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefRewardSelectable::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefRewardSelectable* reference = NewObject<URefRewardSelectable>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefRewardPostHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefRewardPost::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefRewardPost* reference = NewObject<URefRewardPost>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefRewardPeriodHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefRewardPeriod::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefRewardPeriod* reference = NewObject<URefRewardPeriod>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefRewardHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefReward::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child){
		URefReward* reference = NewObject<URefReward>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefDialogHandler(const FString& uid, const FJsonObject* root)
{
	auto references = _references.FindRef(URefDialog::StaticClass());

	if (root->TryGetField(URefDialog::RootFieldName).IsValid() == false) {
		for (auto& it : root->Values) {
			FString childUID{ FString::Printf(TEXT("%s.%s"), *uid, *it.Key) };
			if (auto& childObj = it.Value->AsObject()) {
				URefDialogHandler(childUID, childObj.Get());
			}
		}
		return;
	}

	URefDialog* reference = CreateReference<URefDialog>(this, uid);
	reference->Parse(root);
#if WITH_EDITOR
	AddDialogString(uid, root);
#endif
	references->AddReference(reference);
}

void UReferenceBuilder::URefBodyHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefBody::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefBody* reference = NewObject<URefBody>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefCustomDetailHandler(const FXmlNode* root)
{
	auto objs = _references.FindRef(URefCustomDetail::StaticClass());
	IterateNodes(root, [this, objs](const FXmlNode* child) {
		URefCustomDetail* newReference = NewObject<URefCustomDetail>(this);
		newReference->Parse(child);
		objs->AddReference(newReference);
	});
}

void UReferenceBuilder::URefPaletteGroupHandler(const FXmlNode* root)
{
	auto objs = _references.FindRef(URefPaletteGroup::StaticClass());
	IterateNodes(root, [this, objs](const FXmlNode* child) {
		URefPaletteGroup* newReference = NewObject<URefPaletteGroup>(this);
		newReference->Parse(child);
		objs->AddReference(newReference);
	});
}

void UReferenceBuilder::URefPaletteHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefPalette::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefPalette>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefColorHandler(const FXmlNode* root)
{
	auto objs = _references.FindRef(URefColor::StaticClass());
	IterateNodes(root, [this, objs](const FXmlNode* child) {
		URefColor* newReference = NewObject<URefColor>(this);
		newReference->Parse(child);
		objs->AddReference(newReference);
	});
}

void UReferenceBuilder::URefColorPigmentHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefColorPigment::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefColorPigment>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefArbeitQuestGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefArbeitQuestGroup::StaticClass());
	auto parents = _references.FindRef(URefQuestGroup::StaticClass());

	IterateNodes(root, [this, references, parents](const FXmlNode* child) {
		auto reference = NewObject<URefArbeitQuestGroup>(this);
		reference->Parse(child);
		references->AddReference(reference);
		parents->AddReference(reference);
	});
}

void UReferenceBuilder::URefStampTourQuestGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefStampTourQuestGroup::StaticClass());
	auto parents = _references.FindRef(URefQuestGroup::StaticClass());

	IterateNodes(root, [this, references, parents](const FXmlNode* child) {
		auto reference = NewObject<URefStampTourQuestGroup>(this);
		reference->Parse(child);
		references->AddReference(reference);
		parents->AddReference(reference);
	});
}

void UReferenceBuilder::URefPassQuestGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefPassQuestGroup::StaticClass());
	auto parents = _references.FindRef(URefQuestGroup::StaticClass());

	IterateNodes(root, [this, references, parents](const FXmlNode* child) {
		auto reference = NewObject<URefPassQuestGroup>(this);
		reference->Parse(child);
		references->AddReference(reference);
		parents->AddReference(reference);
	});
}

void UReferenceBuilder::URefQuestPassHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefQuestPass::StaticClass());
	auto parents = _references.FindRef(URefQuest::StaticClass());

	IterateNodes(root, [this, references, parents](const FXmlNode* child) {
		auto reference = NewObject<URefQuestPass>(this);
		reference->Parse(child);
		references->AddReference(reference);
		parents->AddReference(reference);
	});
}

void UReferenceBuilder::URefArbeitRewardHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefArbeitReward::StaticClass());

	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefArbeitReward>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefQuestArbeitHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefQuestArbeit::StaticClass());
	auto parents = _references.FindRef(URefQuest::StaticClass());

	IterateNodes(root, [this, references, parents](const FXmlNode* child) {
		auto reference = NewObject<URefQuestArbeit>(this);
		reference->Parse(child);
		references->AddReference(reference);
		parents->AddReference(reference);
	});
}

void UReferenceBuilder::URefQuestSequenceArbeitHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefQuestSequenceArbeit::StaticClass());
	auto parents = _referenceList.FindRef(URefQuestSequence::StaticClass());

	IterateNodes(root, [this, references, parents](const FXmlNode* child) {
		auto reference = NewObject<URefQuestSequenceArbeit>(this);
		reference->Parse(child);
		references->AddReference(reference);
		parents->AddReference(reference);
	});
}

void UReferenceBuilder::URefQuestGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefQuestGroup::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child)
		{
			auto reference = NewObject<URefQuestGroup>(this);
			reference->Parse(child);
			references->AddReference(reference);
		});
}

void UReferenceBuilder::URefQuestHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefQuest::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child)
		{
			auto reference = NewObject<URefQuest>(this);
			reference->Parse(child);
			references->AddReference(reference);
		});
}

void UReferenceBuilder::URefQuestSequenceHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefQuestSequence::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefQuestSequence>(this);
		reference->Parse(child);
		references->AddReference(reference);
		});
}

void UReferenceBuilder::URefQuestEventHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefQuestEvent::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefQuestEvent>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefQuestScheduleHandler(const FXmlNode * root)
{
	auto references = _referenceList.FindRef(URefQuestSchedule::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefQuestSchedule>(this);
		reference->Parse(child);

		if (reference->_schedule->GetNextEndDate() < reference->_schedule->GetNow()) {
			return;  // discard too old schedule
		}

		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefQuestChallengeHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefQuestChallenge::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefQuestChallenge>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefTitleHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefTitle::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefTitle>();
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefEquipAttributeHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefEquipAttribute::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEquipAttribute>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefEquipUpgradeHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefEquipUpgrade::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEquipUpgrade>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefEquipUpgradeEffectHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefEquipUpgradeEffect::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEquipUpgradeEffect>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefEquipCarveEffectHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefEquipCarveEffect::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEquipCarveEffect>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefEquipCraftHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefEquipCraft::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEquipCraft>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefEquipReforgeHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefEquipReforge::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEquipReforge>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefEquipSetEffectHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefEquipSetEffect::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEquipSetEffect>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefEquipCollectionGroupHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefEquipCollectionGroup::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEquipCollectionGroup>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefEquipCollectionHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefEquipCollection::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEquipCollection>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSkillPatronageHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefSkillPatronage::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSkillPatronage>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSkillPatronageTierHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefSkillPatronageTier::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSkillPatronageTier>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSkillPatronageCostHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefSkillPatronageCost::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSkillPatronageCost>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefScheduleHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefSchedule::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSchedule>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefPostTemplateHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefPostTemplate::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefPostTemplate>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefNPCProfileHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefNPCProfile::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefNPCProfile>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefStatHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefStat::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefStat>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefRankingHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefRanking::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefRanking>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefReplyHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefReply::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefReply>(this);
		reference->Parse(child);

		auto& replys = _replys.FindOrAdd(reference->Reply_UID);
		replys.Emplace(reference);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefReactionHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefReaction::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefReaction>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefStreamingHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefStreaming::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefStreaming>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefInterviewHandler(const FXmlNode* root)
{
	auto strms = _references.FindRef(URefStreaming::StaticClass());
	auto references = _references.FindRef(URefInterview::StaticClass());
	IterateNodes(root, [this, strms, references](const FXmlNode* child) {
		auto reference = NewObject<URefInterview>(this);
		reference->Parse(child);
		references->AddReference(reference);
		strms->AddReference(reference);
	});
}

void UReferenceBuilder::URefInspectingHandler(const FXmlNode* root)
{
	auto strms = _references.FindRef(URefStreaming::StaticClass());
	auto references = _references.FindRef(URefInspecting::StaticClass());
	IterateNodes(root, [this, strms, references](const FXmlNode* child) {
		auto reference = NewObject<URefInspecting>(this);
		reference->Parse(child);
		references->AddReference(reference);
		strms->AddReference(reference);
	});
}

void UReferenceBuilder::URefTakePhotoHandler(const FXmlNode* root)
{
	auto strms = _references.FindRef(URefStreaming::StaticClass());
	auto references = _references.FindRef(URefTakePhoto::StaticClass());
	IterateNodes(root, [this, strms, references](const FXmlNode* child) {
		auto reference = NewObject<URefTakePhoto>(this);
		reference->Parse(child);
		references->AddReference(reference);
		strms->AddReference(reference);
	});
}

void UReferenceBuilder::URefItemReviewHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefItemReview::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefItemReview>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefItemTradeHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefItemTrade::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefItemTrade>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefRankingRewardHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefRankingReward::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefRankingReward>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSubscriptionRewardHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefSubscriptionReward::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSubscriptionReward>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefStageRewardHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefStageReward::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		URefStageReward* reference = NewObject<URefStageReward>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefUserInteractionHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefUserInteraction::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefUserInteraction>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefContentsSrcHandler(const FString& uid, const FJsonObject* root)
{
	auto references = _references.FindRef(URefContentsSrc::StaticClass());
	URefContentsSrc* reference = CreateReference<URefContentsSrc>(this, uid);

	reference->Key = *root->GetStringField("Key");
	for (auto& eventFiled : root->GetArrayField("Event")) {
		URefQuestEvent* questEvent = NewObject<URefQuestEvent>(this);
		auto typeObj = eventFiled->AsObject();
		questEvent->Type = *typeObj->GetStringField("Type");
		questEvent->Type_Value = typeObj->GetStringField("Type_Value");
		reference->Events.Emplace(questEvent);
	}
	reference->Parse(root);
	references->AddReference(reference);
}

void UReferenceBuilder::URefStreamingNPCHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefStreamingNPC::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefStreamingNPC>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSkillTreeHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefSkillTree::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSkillTree>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSkillTreeStepHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefSkillTreeStep::StaticClass());
		IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSkillTreeStep>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSkillTreeSlotHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefSkillTreeSlot::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSkillTreeSlot>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}

void UReferenceBuilder::URefSkillTreeLevelHandler(const FXmlNode* root)
{
	auto references = _references.FindRef(URefSkillTreeLevel::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefSkillTreeLevel>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}
void UReferenceBuilder::URefEmblemEffectHandler(const FXmlNode* root)
{
	auto references = _referenceList.FindRef(URefEmblemEffect::StaticClass());
	IterateNodes(root, [this, references](const FXmlNode* child) {
		auto reference = NewObject<URefEmblemEffect>(this);
		reference->Parse(child);
		references->AddReference(reference);
	});
}