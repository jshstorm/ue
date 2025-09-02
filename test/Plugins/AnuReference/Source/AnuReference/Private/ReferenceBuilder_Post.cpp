#include "ReferenceBuilder.h"
#include "Kismet/KismetTextLibrary.h"
#include "Dom/JsonObject.h"
#include "GameFramework/Character.h"

void UReferenceBuilder::CostPostProcessor(FDynamicCost& dst)
{
	if (dst._typeName.IsNone()) {
		return;
	}

	dst._target = Cast<URefBase>(GetRefObj(dst.UID, dst._type));
	checkRefMsgfRet(Error, dst.UID.IsNone() || dst._target, TEXT("invalid Cost_UID[%s] for Cost_Type[%s]"), *dst.UID.ToString(), *dst._type->GetName());

	dst._discountSchedule = GetRefObj<URefSchedule>(dst.Discount_Schedule);
	checkRefMsgfRet(Error, dst.Discount_Schedule.IsNone() || dst._discountSchedule, TEXT("invalid Schedule[%s] bound with ShopCost"), *dst.Discount_Schedule.ToString());
}

void UReferenceBuilder::URefShopItemPostProcessor()
{
	DoRefIterationJobNoUID<URefShopCost>([this](URefShopCost* shopCost) {
		CostPostProcessor(shopCost->_impl);
	});

	// auto-build item sell data
	static FName AutoBuiltGoldRewardUID{ "gold" };
	DoRefIterationJob<URefItem>([this](URefItem* item) {
		if (item->Price == 0) {
			return; // unable to sell
		}

		if (GetRefObj<URefShopItem>(item->UID)) {
			return; // special selling item; no support auto built
		}

		DoRefIterationJob<URefShopGroup>([this, item](URefShopGroup* shop) {
			if (shop->SubType != "Sell") {
				return;
			}
			URefShopItem* autoBuilt = NewObject<URefShopItem>(this);
			autoBuilt->UID = item->UID;
			autoBuilt->GUID = item->GUID;
			autoBuilt->ShopGroup = shop->UID;
			autoBuilt->Target_Type = URefCurrency::TableName;
			autoBuilt->Target_UID = AutoBuiltGoldRewardUID;
			autoBuilt->Amount = item->Price;
			autoBuilt->Category = item->typeNames[1];
			FDynamicCost cost;
			cost._typeName = "Item";
			cost._type = URefItem::StaticClass();
			cost.UID = item->UID;
			cost.SetOriginAmount(1);
			autoBuilt->_cost.Emplace(MoveTemp(cost));

			auto shopItems = _references.FindRef(URefShopItem::StaticClass());
			shopItems->AddReference(autoBuilt);
		});
	});

	DoRefIterationJob<URefShopItem>([this](URefShopItem* reference) {
		reference->_shopGroup = GetRefObj<URefShopGroup>(reference->ShopGroup);
		checkRefMsgfRet(Error, reference->_shopGroup, TEXT("ShopItem[%s] bound with invalid ShopGroup[%s]"), *reference->UID.ToString(), *reference->ShopGroup.ToString());
		reference->_shopGroup->_packages.Emplace(reference);

		static TMap<FName, UClass*> targetTypes{
			{"Item", URefItem::StaticClass()},
			{"Package", URefReward::StaticClass()},
			{"RandomBox", URefReward::StaticClass()},
			{URefCurrency::TableName, URefCurrency::StaticClass()},
			{"Body", URefBody::StaticClass()},
			{"Height", URefBody::StaticClass()},
			{URefQuestGroup::AttendanceGroup, URefQuest::StaticClass()},
			{URefShopItem::SubscriptionType, URefReward::StaticClass()},
			{"PC", URefPC::StaticClass()},
		};

		if (reference->Target_Type.IsNone() == false) {
			reference->_type = targetTypes.FindRef(reference->Target_Type);
			checkRefMsgfRet(Error, reference->_type, TEXT("ShopItem[%s] bound with invalid Target_Type[%s]"), *reference->UID.ToString(), *reference->Target_Type.ToString());
			reference->_target = Cast<URefBase>(GetRefObj(reference->Target_UID, reference->_type));
			checkRefMsgfRet(Error, reference->_target, TEXT("ShopItem[%s] bound with invalid Target_UID[%s]; check Target_Type[%s]"), *reference->UID.ToString(), *reference->Target_UID.ToString(), *reference->Target_Type.ToString());
		}

		reference->_schedule = ParseDynamicSchedule(reference->Schedule, "ShopItem_Schedule", true);
		checkRefMsgfRet(Error, FName(reference->Schedule).IsNone() || reference->_schedule, TEXT("ShopItem[%s] bound with invalid Schedule[%s]"), *reference->UID.ToString(), *reference->Schedule);
		
		reference->_purchaseResetSchedule = ParseDynamicSchedule(reference->Purchase_Reset_Schedule, "ShopItem_PurchaseResetSchedule", true);
		checkRefMsgfRet(Error, FName(reference->Purchase_Reset_Schedule).IsNone() || reference->_purchaseResetSchedule, TEXT("ShopItem[%s] bound with invalid Schedule[%s]"), *reference->UID.ToString(), *reference->Purchase_Reset_Schedule);

		for (auto& applyTag : reference->ApplyTags) {
			TArray<FString> strValues;
			URefBase::GetTrimedStringArray(applyTag, strValues);
			checkRefMsgfRet(Error, strValues.Num() == 2, TEXT("ShopItem[%s] bound with invalid Apply_Tag[%s]"), *reference->UID.ToString(), *applyTag);

			URefTag* tag = GetRefObj<URefTag>(strValues[0]);
			checkRefMsgfRet(Error, tag, TEXT("ShopItem[%s] bound with invalid Apply_Tag[%s]. check Tag table"), *reference->UID.ToString(), *strValues[0]);
			int32 value = FCString::Atoi(*strValues[1]);

			reference->_applyTags.Emplace(tag, value);
		}

		for (auto& it : reference->Display_Tag) {
			if (it.IsNone()) {
				break;
			}
			URefTag* tag = GetRefObj<URefTag>(it);
			checkRefMsgfRet(Error, tag, TEXT("ShopItem[%s] bound with invalid Display_Tag[%s]"), *reference->UID.ToString(), *it.ToString());
			reference->_displayTags.Emplace(tag);
		}

		// redirect shop cost
		if (reference->GetCostType() == "ShopCost") {
			const FDynamicCost& redirect{ reference->_cost[0] };
			auto shopCosts = _shopCosts.Find(redirect.UID);
			checkRefMsgfRet(Error, shopCosts, TEXT("ShopItem[%s] bound with invalid ShopCost[%s]"), *reference->UID.ToString(), *redirect.UID.ToString());
			for (auto& shopCostRef : *shopCosts) {
				FDynamicCost redirected{ shopCostRef->_impl };
				redirected.SetOriginAmount(redirected.GetOriginAmount() * redirect.GetOriginAmount());
				
				for (auto& replacableUID : shopCostRef->Replace_ShopCost) {
					auto& replacableShopCosts = _shopCosts.FindOrAdd(replacableUID);
					for (auto& replacableShopCost : replacableShopCosts) {
						redirected._replacables.Emplace(replacableShopCost->_impl);
					}
				}

				reference->_cost.Emplace(MoveTemp(redirected));
			}
			reference->_cost.RemoveAt(0);

			if (reference->_cost.Num() == 1) {
				reference->Pre_Discount_Value = reference->_cost[0].GetOriginAmount();
			}
		}

		for (auto& cost : reference->_cost) {
			CostPostProcessor(cost);

			if (URefPayment* payment = Cast<URefPayment>(cost._target); payment && reference->_registeredInApp) {
				payment->_targetShopItem = reference;
			}
			else if (URefItemUsable* coupon = Cast<URefItemUsable>(cost._target); coupon && coupon->typeId.typeID3 == TID3_COUPON) {
				coupon->_targetShops.AddUnique(reference->_shopGroup);
			}

			for (auto& replacable : cost._replacables) {
				if (URefItemUsable* coupon = Cast<URefItemUsable>(replacable._target); coupon && coupon->typeId.typeID3 == TID3_COUPON) {
					coupon->_targetShops.AddUnique(reference->_shopGroup);
				}
			}
		}
	});

	DoRefIterationJob<URefShopItemRandom>([this](URefShopItemRandom* shopItemRandom) {
		shopItemRandom->_defaultAvailable = false;
		shopItemRandom->_shopGroup->_isRuntimeBuildShop = true;

		shopItemRandom->_pool = GetRefObj<URefShopPool>(shopItemRandom->ShopPool_UID);
		checkRefMsgfRet(Error, shopItemRandom->_pool, TEXT("[shop] ShopItemRandom[%s] bound with invalid ShopPool_UID[%s]"), *shopItemRandom->UID.ToString(), *shopItemRandom->ShopPool_UID.ToString());
	});

	DoRefIterationJob<URefShopPool>([this](URefShopPool* shopPool) {
		shopPool->_schedule = GetRefObj<URefSchedule>(shopPool->Schedule);
		checkRefMsgfRet(Error, shopPool->_schedule, TEXT("[shop] ShopPool[%s] bound with invalid Schedule[%s]"), *shopPool->UID.ToString(), *shopPool->Schedule.ToString());
	});

	DoRefIterationJob<URefShopGroup>([this](URefShopGroup* shop) {
		shop->_bonusReward = GetRefObj<URefReward>(shop->Bonus_Reward);
		checkRefMsgfRet(Error, shop->Bonus_Reward.IsNone() || shop->_bonusReward, TEXT("[shop] ShopGroup[%s] bound with invalid Bonus_Reward[%s]"), *shop->UID.ToString(), *shop->Bonus_Reward.ToString());
	});
}

URefSchedule* UReferenceBuilder::ParseDynamicSchedule(const FString& scheduleValue, const FString& context, bool useLocalTime)
{
	if (scheduleValue.Find("|") == INDEX_NONE) {
		return GetRefObj<URefSchedule>(scheduleValue);
	}

	URefSchedule* newSchedule = NewObject<URefSchedule>(this);
	bool res = newSchedule->PeriodCondition.Parse("Period", scheduleValue);
	checkf(res, TEXT("dynamic schedule[%s] has not-a-date time value; from[%s]"), *scheduleValue, *context);
	newSchedule->Use_Local_Time = useLocalTime;
	return newSchedule;
}

void UReferenceBuilder::URefPostTemplatePostProcessor()
{
	DoRefIterationJob<URefPostTemplate>([this](URefPostTemplate* reference) {
		if (reference->Icon.IsNone() == false) {
			reference->_iconTexture = GetResourceRoute<UTexture2D>(reference->Icon);
		}
	});
}

void UReferenceBuilder::URefObjectPostProcessor()
{
	InitializeCostumeData();

	DoRefIterationJob<URefObject>([this](URefObject* reference) {
#if WITH_EDITOR
		UE_LOG(LogReference, Verbose, TEXT("loading refobject uid [%s]"), *reference->UID.ToString());
#endif
		reference->_displayInfo._name.Append(reference->Name);
		reference->_displayInfo._desc.Append(reference->Desc);
		LoadResource(reference->Icon, reference->_displayInfo._icon);
		LoadResource(reference->Icon_2, reference->_icon2ObjectPtrs);
		GetResourceTable<ACharacter>(reference->Model, reference->_models);
	});

	DoRefIterationJob<URefCharacter>([this](URefCharacter* reference) {
		for (auto& groupName : reference->GroupName) {
			TArray<URefSkill*> skills;
			GetGroupReference<URefSkill>(groupName, skills);
			for (auto skill : skills) {
				reference->_builtInSkills.Emplace(skill);
			}
		}
	});

	DoRefIterationJob<URefSkillObject>([this](URefSkillObject* reference) {
		TArray<URefSkill*> skills;
		GetGroupReference<URefSkill>(reference->UID, skills);
		for (auto skill : skills) {
			reference->_builtInSkills.Emplace(skill);
		}
	});

	DoRefIterationJob<URefNPC>([this](URefNPC* reference) {
		for (auto& it : reference->Shop_UID) {
			if (it.IsNone()) {
				break;
			}

			URefShopGroup* shop = GetRefObj<URefShopGroup>(it);
			checkRefMsgfCont(Error, shop, TEXT("[npc] invalid ShopGroup[%s] bound with NPC[%s]"), *it.ToString(), *reference->UID.ToString());
			shop->_ownerNPC = reference;
			reference->_shops.Emplace(shop);
		}
		static FName NAME_Shop{ "Shop" };
		if (reference->_shops.Num() != 0 && reference->Menu.Find(NAME_Shop) == INDEX_NONE) {
			reference->Menu.Emplace(NAME_Shop);
		}

		reference->_schedule = GetRefObj<URefSchedule>(reference->Spawn_Schedule);

		reference->_unlockQuest = GetRefObj<URefQuest>(reference->Unlock_Condition);
		checkRefMsgf(Error, reference->Unlock_Condition == NAME_None || reference->_unlockQuest, TEXT("NPC[%s] bound with invalid Unlock_Condition[%s]. check NPC/Quest table"), *reference->UID.ToString(), *reference->Unlock_Condition.ToString());

		ReserveQuestEvent(reference->_moveEvtUID, "Move_Closest", reference->UID.ToString());
		ReserveQuestEvent(reference->_menuEvtUID, "Open_UI_NPC", reference->UID.ToString());
		ReserveQuestEvent(reference->_marketEvtUID, "Open_UI_NPC_Market", reference->UID.ToString());
	});

	DoRefIterationJob<URefLifeObject>([this](URefLifeObject* reference) {
		reference->_class = GetRefObj<URefClass>(reference->Class_UID);
		reference->_costCurrency = GetRefObj<URefCurrency>(reference->Currency_UID);

		// condition notify
		if (reference->_class) {
			reference->_class->_displayInfo._icon.Empty();
			reference->_class->_displayInfo._name.Empty();
			reference->_class->_displayInfo._icon.Append(reference->_displayInfo._icon);
			reference->_class->_displayInfo._name.Emplace(reference->_class->GetRawName());
			_classByTID2.Emplace(reference->typeNames[1], reference->_class);
		}

		if (_lifeObjByTID.Find(reference->typeId.typeID) == nullptr) {
			_lifeObjByTID.Emplace(reference->typeId.typeID, reference);
		}
	});

	TSet<int32> duplicateChecker;
	UDataTable* table = GetResourceTable<UAnuBellSound>();
	table->ForeachRow<FAnuResourceBellSound>("", [&duplicateChecker](const FName& rowName, const FAnuResourceBellSound& value) {
		checkRefMsgf(Error, duplicateChecker.Find(value.GUID) == nullptr, TEXT("DT_ResourceBellSound has duplicate GUID[%d]"), value.GUID);
		checkRefMsgf(Error, value.GUID <= MAX_BELL_SOUND_COUNT, TEXT("DT_ResourceBellSound has invalid GUID[%d]; exceed MAX_BELL_SOUND_COUNT[%d]"), value.GUID, MAX_BELL_SOUND_COUNT);
		duplicateChecker.Emplace(value.GUID);
	});
}

void UReferenceBuilder::URefCharacterPostProcessor()
{
}

void UReferenceBuilder::TagPostProcessor(const FString& contextString, const TMap<FName, int32>& tagWithValues, TMap<URefTag*, int32>& output)
{
	for (auto& it : tagWithValues) {
		FName tagUID{ it.Key };
		if (tagUID.IsNone()) {
			break;
		}
		URefTag* tag = GetRefObj<URefTag>(tagUID);
		checkRefMsgfRet(Error, tag, TEXT("[tag] [%s] bound with invalid Tag[%s]"), *contextString, *tagUID.ToString());
		checkRefMsgfRet(Error, output.Find(tag) == nullptr, TEXT("[tag] [%s] has duplicate Tag[%s]"), *contextString, *tagUID.ToString());
		output.Emplace(tag, it.Value);
	}
}

void UReferenceBuilder::URefItemPostProcessor()
{
	DoRefIterationJobNoUID<URefEquipAttribute>([this](URefEquipAttribute* reference) {
		URefItemEquip* refItem = GetRefObj<URefItemEquip>(reference->Item_UID);
		checkRefMsgfRet(Error, refItem, TEXT("[equip attribute] invalid data for item[%s]; not registered in Item_Equip"), *reference->Item_UID.ToString());

		auto& attribute = refItem->_attributes.FindOrAdd(reference->Attribute.Apply_Type);
		attribute.Emplace(reference);
		reference->StatInfoData = { reference->Attribute.Apply_Effect, URefObject::GetStatEnum(reference->Attribute.Apply_Effect), (StatValue)reference->Attribute.Value, reference->Attribute.Value_Type };
		const EStatTypes statType = reference->StatInfoData.StatType;
		if (statType != EStatTypes::None && statType != EStatTypes::ElementalDamage) {
			refItem->StatMinSum += reference->MinValue;
			refItem->StatMaxSum += reference->MaxValue;
		}
	});

	TMap<FName, uint8> setCounts;
	DoRefIterationJob<URefItem>([this, &setCounts](URefItem* reference) {
		GetResourceTable<UAnuMesh>(reference->Model, reference->_models);

		TagPostProcessor(reference->UID.ToString(), reference->_tagWithValues, reference->_tags);

		auto& setCount = setCounts.FindOrAdd(reference->SetGroup);
		setCount++;
	});

	DoRefIterationJobNoUID<URefEquipSetEffect>([this, &setCounts](URefEquipSetEffect* reference) {
		reference->StatInfoData = { reference->Attribute.Apply_Effect, URefObject::GetStatEnum(reference->Attribute.Apply_Effect), (StatValue)reference->Attribute.Value, reference->Attribute.Value_Type };
		if (reference->EquipCount == 0) {
			reference->EquipCount = setCounts.FindRef(reference->SetGroup);
		}

		if (reference->Attribute.Apply_Type == "Tag") {
			URefTag* tag = GetRefObj<URefTag>(reference->Attribute.Apply_Effect);
			checkRefMsgf(Error, tag, TEXT("EquipSetEffect[%s] bound with invalid Tag[%s]"), *reference->SetGroup.ToString(), *reference->Attribute.Apply_Effect.ToString());
			reference->Tag = tag;
		}
	});

	DoRefIterationJob<URefItemEquip>([this](URefItemEquip* reference) {
		if (reference->typeId.IsWeapon()) {
			reference->_class = GetRefObj<URefClass>(reference->Target_Class);
			checkRefMsgf(Error, reference->_class, TEXT("Item_Equip[%s] bound with invalid Target_Class[%s]"), *reference->UID.ToString(), *reference->Target_Class.ToString());
		}
	});

	auto BuildEquipMolding = [this](FString tableName, URefEquipMolding* reference){
		reference->_itemEquip = GetRefObj<URefItemEquip>(reference->ItemEquip_UID);
		checkRefMsgf(Error, reference->_itemEquip, TEXT("[%s] Item_Equip[%s] bound with invalid equip table item"), *tableName, *reference->ItemEquip_UID.ToString());

		reference->_currency = GetRefObj<URefCurrency>(reference->Currency_UID);
		checkRefMsgf(Error, reference->Currency_UID == NAME_None || reference->_currency, TEXT("[%s] Item_Equip[%s] bound with invalid Currency[%s]"), *tableName, *reference->ItemEquip_UID.ToString(), *reference->Currency_UID.ToString());

		uint8 count = reference->Material_Item_UID.Num();
		for (uint8 i = 0; i < count; ++i) {
			FName itemUID{ reference->Material_Item_UID[i] };
			URefItem* refItem = GetRefObj<URefItem>(itemUID);
			checkRefMsgf(Error, itemUID == NAME_None || refItem, TEXT("[%s] Item_Equip[%s] bound with invalid Item UID[%s]"), *tableName, *reference->ItemEquip_UID.ToString(), *itemUID.ToString());
			if (refItem) {
				reference->_materials.Emplace(MakeTuple(refItem, reference->Material_Item_Count[i]));
			}
		}
	};

	DoRefIterationJobNoUID<URefEquipCraft>([this, BuildEquipMolding](URefEquipCraft* reference) {
		BuildEquipMolding(TEXT("EquipCraft"), reference);
		reference->_itemEquip->_craft = reference;
	});

	DoRefIterationJobNoUID<URefEquipReforge>([this, BuildEquipMolding](URefEquipReforge* reference) {
		BuildEquipMolding(TEXT("EquipReforge"), reference);
		reference->_itemEquip->_reforge = reference;
	});

	DoRefIterationJob<URefItemEmblem>([this](URefItemEmblem* emblem) {
		emblem->_schedule = ParseDynamicSchedule(emblem->Schedule, "ItemEmblem", true);
		checkRefMsgf(Error, emblem->Schedule.Compare("None") == 0 || emblem->_schedule, TEXT("[emblem] Item_Emblem[%s] bound with invalid Schedule[%s]"), *emblem->UID.ToString(), *emblem->Schedule);
	});

	DoRefIterationJob<URefItemUsable>([this](URefItemUsable* usable) {
		if (usable->typeId.IsUsableCurrency()) {
			usable->_targetCurrency = GetRefObj<URefCurrency>(usable->Effect_UID);
			checkRefMsgf(Error, usable->_targetCurrency, TEXT("[usable] Item_Usable[%s] is currency type, but bound with invalid Currency[%s] in Effect_UID"), *usable->UID.ToString(), *usable->Effect_UID.ToString());
		}
		else if (usable->typeId.IsUsableReward()) {
			usable->_targetReward = GetRefObj<URefReward>(usable->Effect_UID);
			checkRefMsgf(Error, usable->_targetReward, TEXT("[usable] Item_Usable[%s] is reward type, but bound with invalid Reward[%s] in Effect_UID"), *usable->UID.ToString(), *usable->Effect_UID.ToString());
		}
		else if (usable->typeId.IsUsableClassExp()) {
			usable->_targetClass = GetRefObj<URefClass>(usable->Effect_UID);
			checkRefMsgf(Error, usable->_targetClass, TEXT("[usable] Item_Usable[%s] is class exp type, but bound with invalid Class[%s] in Effect_UID"), *usable->UID.ToString(), *usable->Effect_UID.ToString());
		}
	});

	DoRefIterationJob<URefItemDyeing>([this](URefItemDyeing* dyeing) {
		if (dyeing->_dyeingType == "Fixed") {
			dyeing->_color = GetRefObj<URefColor>(dyeing->Effect_UID);
			checkRefMsgf(Error, dyeing->_color, TEXT("[dyeing] Item_Usable[%s] is fixed dyeing item, but bound with invalid Color[%s] in Effect_UID"), *dyeing->UID.ToString(), *dyeing->Effect_UID.ToString());
			checkRefMsgf(Error, dyeing->Effect_Value > 0, TEXT("[dyeing] FixedDyeingItem[%s] bound with invalid Effect_Value[%d]; it must be positive number for consuming"), *dyeing->UID.ToString(), dyeing->Effect_Value);

			dyeing->_fixedCost.Parse("Item", dyeing->UID.ToString(), FString::FromInt(dyeing->Effect_Value));
			CostPostProcessor(dyeing->_fixedCost);
		}
		else if (dyeing->_dyeingType == "Random") {
			dyeing->_paletteGroup = GetRefObj<URefPaletteGroup>(dyeing->Effect_UID);
			checkRefMsgf(Error, dyeing->_paletteGroup, TEXT("[dyeing] Item_Usable[%s] is random dyeing item, but bound with invalid PaletteGroup[%s] in Effect_UID"), *dyeing->UID.ToString(), *dyeing->Effect_UID.ToString());
		}
		else if (dyeing->_dyeingType == "ColorPigment") {
			dyeing->_color = GetRefObj<URefColor>(dyeing->Effect_UID);
			checkRefMsgf(Error, dyeing->_color, TEXT("[dyeing] Item_Usable[%s] is fixed dyeing item, but bound with invalid Color[%s] in Effect_UID"), *dyeing->UID.ToString(), *dyeing->Effect_UID.ToString());
			checkRefMsgf(Error, dyeing->Effect_Value > 0, TEXT("[dyeing] FixedDyeingItem[%s] bound with invalid Effect_Value[%d]; it must be positive number for consuming"), *dyeing->UID.ToString(), dyeing->Effect_Value);

			dyeing->_fixedCost.Parse("Item", dyeing->UID.ToString(), FString::FromInt(dyeing->Effect_Value));
			CostPostProcessor(dyeing->_fixedCost);

			dyeing->_color->_pigment = dyeing;
		}
	});
}

void UReferenceBuilder::URefSkillPostProcessor()
{
	UReferences* classes = _references.FindRef(URefClass::StaticClass());
	DoRefIterationJob<URefSkill>([this, classes](URefSkill* reference) {
		reference->_refClass = Cast<URefClass>(classes->GetReference(reference->Skill_Group.SkillGroupValue));
	});

	DoRefIterationJob<URefSkillObject>([this](URefSkillObject* reference) {
		reference->refSkill = GetRefObj<URefSkill>(reference->Skill_UID);

		if (reference->typeId.IsProjectile()) {
			reference->parameters = NewObject<USkillObjParameters_Projectile>(reference);
		}
		else if (reference->typeId.IsOrb()) {
			reference->parameters = NewObject<USkillObjParameters_Orb>(reference);
		}
		else {
			reference->parameters = NewObject<USkillObjParameters>(reference);
		}

		reference->parameters->Parse(reference->Values);
	});

	DoRefIterationJobNoUID<URefSkillEffect>([this](URefSkillEffect* reference) {
		TArray<FString> uids;
		reference->Icon.ToString().ParseIntoArray(uids, TEXT("|"), true);
		for (auto& uid : uids) {
			uid.TrimStartAndEndInline();
			FName nuid = FName(uid);
			if (nuid == NAME_None) {
				checkRefMsgf(Error, uids.Num() == 1, TEXT("[SkillEffect] Format error. 'None' icon cannot be array."));
				break;
			}

			FAnuResourceIcon* row = GetResourceRow<FAnuResourceIcon>(nuid);
			checkRefMsgfRet(Error, row, TEXT("[SkillEffect] Icon '%s' cannot be found in DT_ResourceIcon table."), *uid);

			reference->Asset_Icon.Emplace(row->Texture);
		}
	});
}

void UReferenceBuilder::URefLevelPCPostProcessor()
{
	_charLevels.Sort([](const URefLevelPC& left, const URefLevelPC& right) -> bool {
		return left.Exp < right.Exp;
	});
}

void UReferenceBuilder::URefLevelNPCPostProcessor()
{
	DoRefIterationJobNoUID<URefLevelNPC>([this](URefLevelNPC* reference) {
		_npcLvTable.Emplace(reference->Exp);
	});

	_npcLvTable.Emplace(0); // 현재 level을 바로 인덱스로 사용하기 위해 0레벨 시작 exp 더미로 하나 더 넣음
	_npcLvTable.Sort([](const int64& lhs, const int64& rhs) {
		return lhs < rhs;
	});
}

void UReferenceBuilder::URefClassPostProcessor()
{
	URefClass::s_OpenPreQuests.Empty();
	DoRefIterationJob<URefClass>([this](URefClass* reference) {
		reference->_iconTexture = GetResourceRoute<UTexture2D>(reference->Icon);

		if (reference->Default_CameraProperty.Compare(TEXT("None")) != 0) {
			URefResourceCore* core = GetRefObj<URefResourceCore>(reference->Default_CameraProperty);
			UClass* cameraAssetClass = LoadObject<UClass>(this, *core->Route);
			reference->_cameraProperty = NewObject<UObject>(this, cameraAssetClass);
		}

		for (auto& it : reference->OpenQuest_UID) {
			for (auto& uid : it.Value) {
				URefQuest* refQuest = GetRefObj<URefQuest>(uid);
				checkRefMsgfRet(Error, refQuest, TEXT("Class[%s] Type[%d] bound with invalid Quest[%s]. check Quest table"), *reference->UID.ToString(), it.Key, *uid.ToString());
				reference->GetRTLicense(it.Key)._openRefQuest.Emplace(refQuest);
				if (URefQuest* preQuest = GetRefObj<URefQuest>(refQuest->PreCondition)) {
					URefClass::s_OpenPreQuests.Emplace(preQuest->UID, reference);
				}
			}
		}
		
		for (auto& it : reference->SpecialStat) {
			if (it.IsNone()) { break; }
			URefStat* stat = GetRefObj<URefStat>(it);
			checkRefMsgfRet(Error, stat, TEXT("Class[%s] bound with invalid Stat[%s]. check Stat table"), *reference->UID.ToString(), *it.ToString());
			reference->_specialStats.Emplace(stat);
		}
	});

	DoRefIterationJobNoUID<URefClassLevel>([this](URefClassLevel* reference) {
		auto refClass = GetRefObj<URefClass>(reference->Class_UID);
		checkRefMsgfRet(Error, refClass, TEXT("not find class [%s]"), *reference->Class_UID.ToString());
		reference->_class = refClass;
		refClass->_classLevels.Emplace(reference->Level, reference);
		for (auto& sklUID : reference->Open_Skill) {
			if (sklUID == NAME_None) {
				break;
			}
			URefSkill* refSkill = GetRefObj<URefSkill>(sklUID);
			checkRefMsgfRet(Error, refSkill, TEXT("ClassLevel[%s:%d] bound with invalid OpenSkill[%s]. check Skill table"), *reference->Class_UID.ToString(), reference->Level, *sklUID.ToString());
			refClass->_lvSkills.Emplace(refSkill);
		}
	});

	DoRefIterationJob<URefClassLicenseMastery>([this](URefClassLicenseMastery* reference) {
		reference->_refClass = GetRefObj<URefClass>(reference->Class_UID);
		checkRefMsgfRet(Error, reference->_refClass, TEXT("not find class mastery[%s] License[%s]"), *reference->UID.ToString(), *reference->Class_UID.ToString());
		reference->_refClass->GetRTLicense(reference->License_Type)._masteries.Emplace(reference);

		if (auto it = reference->_conditions.FindByPredicate([this](const FConditionRule& rule) { return GetRefObj<URefObject>(rule.Values[0]) != nullptr; })) {
			URefObject* object = GetRefObj<URefObject>(it->Values[0]);
			reference->_refObject = object;
			FString hintKey;
			FFormatNamedArguments fmtArgs;
			if (reference->Desc_Detail_1.IsEmpty() == false) {
				fmtArgs.Emplace(TEXT("0"), FText::FormatNamed(reference->Desc_Detail_1, "ObjectName", object->GetName()));
				hintKey += TEXT("{0}\n");
			}
			if (reference->Desc_Detail_2.IsEmpty() == false) {
				fmtArgs.Emplace(TEXT("1"), FText::FormatNamed(reference->Desc_Detail_2, "ObjectName", object->GetName()));
				hintKey += TEXT("{1}\n");
			}
			if (reference->Desc_Detail_2.IsEmpty() == false) {
				fmtArgs.Emplace(TEXT("2"), FText::FormatNamed(reference->Desc_Detail_3, "ObjectName", object->GetName()));
				hintKey += TEXT("{2}");
			}

			reference->_hintDesc = FText::Format(FText::FromString(hintKey), fmtArgs);
		}
	});
}

void UReferenceBuilder::URefWorldPostProcessor()
{
	DoRefIterationJob<URefRegion>([this](URefRegion* reference) {
		URefWorld* refWorld = GetRefObj<URefWorld>(reference->WorldUID);
		checkRefRet(Error, refWorld);
		refWorld->_regions.Emplace(reference);
		reference->_world = refWorld;
	});

	DoRefIterationJob<URefWorld>([this](URefWorld* reference) {
		FString	path{ GetResourcePath<UWorld>(reference->Model) };
		FString lefts, rights;
		path.Split(TEXT("."), &lefts, &rights);
		reference->_modelName = *lefts;

		reference->MoveSequence.Remove(NAME_None);

		for (auto& region : reference->_regions) {
			FString levelName{ region->SpawnPath.Right(region->SpawnPath.Len() - UReferenceBuilder::SpawnPathPrefix.Len()) };

			auto lookupFile = MakeShared<FXmlFile>();
			if (lookupFile->LoadFile(GetWorldLookupFilePath(levelName))) {
				FXmlNode* root = lookupFile->GetRootNode();
				IterateNodes(root, [this, reference](const FXmlNode* child) {
					FName lookupType{ *child->GetAttribute("Type") };
					FName uid{ *child->GetAttribute("UID") };

					auto& lookups = _worldLookupTable.FindOrAdd(lookupType);
					lookups.Emplace(uid, reference->_regions[0]);
				});
			}
		}
	});
}

void UReferenceBuilder::URefRegionPostProcessor()
{
	DoRefIterationJob<URefRegion>([this](URefRegion* reference) {
		reference->_ruleQuest = GetRefObj<URefQuest>(reference->Rule_Quest);
		checkRefMsgfRet(Error, reference->Rule_Quest == NAME_None || reference->_ruleQuest, TEXT("region[%s] bound with invalid rule quest[%s]"), *reference->UID.ToString(), *reference->Rule_Quest.ToString());

		reference->_unlockQuest = GetRefObj<URefQuest>(reference->Unlock_Condition);
		checkRefMsgfRet(Error, reference->Unlock_Condition == NAME_None || reference->_unlockQuest, TEXT("region[%s] bound with invalid unlock quest[%s]"), *reference->UID.ToString(), *reference->Unlock_Condition.ToString());

		FString	path{ GetResourcePath<UWorld>(reference->Model) };
		FString lefts, rights;
		path.Split(TEXT("."), &lefts, &rights);
		reference->_modelName = *lefts;
	});
}
void UReferenceBuilder::FillQuestEvent(const TArray<TSharedPtr<FJsonValue>>& eventArray, TArray<URefQuestEvent*>& outArray)
{
	for (auto& it : eventArray) {
		TSharedPtr<FJsonObject> typeObj = it->AsObject();
		checkRefMsgfRet(Error, typeObj, TEXT("QuestEvent json format invalid"));

		FString presetFileName;
		if (typeObj->TryGetStringField("Preset_File", presetFileName)) {
			TSharedPtr<FJsonObject> presetJson = LoadQuestEventJson(*presetFileName);
			FillQuestEvent(presetJson->GetArrayField(typeObj->GetStringField("Preset_Name")), outArray);
			continue;
		}

		FString str;
		if (typeObj->TryGetStringField("Type", str) == false) {
			continue;
		}

		FName type{ *str };
		if (type.IsNone()) {
			continue;
		}

		URefQuestEvent* questEvent = NewObject<URefQuestEvent>(this);
		questEvent->Type = type;
		questEvent->Type_Value = typeObj->GetStringField("Type_Value");
		if (typeObj->TryGetStringField("Region", str)) {
			questEvent->Region = *str;
		}
	
		if (typeObj->HasField("Condition")) {
			const auto& conditions = typeObj->GetArrayField("Condition");
			for (auto& conditionIter : conditions) {
				FConditionRule::Parse(questEvent->_condition, conditionIter->AsString());
			}
		}

		if (typeObj->HasField("Fail_Event")) {
			const auto& failEvents = typeObj->GetArrayField("Fail_Event");
			FillQuestEvent(failEvents, questEvent->_failEvents);
		}

		outArray.Emplace(questEvent);
	}
}

void UReferenceBuilder::FillQuestEvent(const FString& eventName, TSharedPtr<FJsonObject> eventJson, TArray<URefQuestEvent*>& outArray)
{
	const TArray<TSharedPtr<FJsonValue>>* arrayField;
	if (eventJson->TryGetArrayField(eventName, arrayField) == false) {
		return;
	}

	FillQuestEvent(*arrayField, outArray);
}

void UReferenceBuilder::URefQuestPostProcessor()
{
	auto CacheFriend = [this](URefReward* reward, URefQuest* questRef) {
		if (reward == nullptr) {
			return;
		}

		reward->Visit(ERewardObjectType::Friend, [questRef](URefRewardBase* rwd, int32 amount) {
			if (auto npc = Cast<URefNPC>(rwd->_character)) {
				npc->_friendOpenQuest = questRef;
			}
		});
	};

	DoRefIterationJob<URefQuest>([this, &CacheFriend](URefQuest* quest) {
		quest->_displayInfo._icon.Emplace(GetResourceRoute<UTexture2D>(quest->Icon));
		quest->_displayInfo._name.Emplace(quest->Name);

		URefQuestGroup* questGroup = GetRefObj<URefQuestGroup>(quest->QuestGroup);
		checkRefMsgfRet(Error, questGroup, TEXT("Not exist QuestGroup[%s]. Check Quest table."), *quest->QuestGroup.ToString());
		quest->_groupType = questGroup->_type;

		questGroup->Quests.Emplace(quest);
		quest->_group = questGroup;

		quest->_prevQuest = GetRefObj<URefQuest>(quest->PreCondition);
		if (quest->_prevQuest) {
			quest->_prevQuest->_nextQuests.Emplace(quest);
		}

		quest->_resetSchedule = GetRefObj<URefSchedule>(quest->Reset_Schedule);
		checkRefMsgfRet(Error, quest->Reset_Schedule.IsNone() || quest->_resetSchedule, TEXT("Quest[%s] bound with invalid Reset_Schedule[%s]"), *quest->UID.ToString(), *quest->Reset_Schedule.ToString());

		for (auto& it : quest->Reward) {
			if (it.IsNone()) {
				break;
			}
			URefReward* reward = GetRefObj<URefReward>(it);
			checkRefMsgfRet(Error, reward, TEXT("quest[%s] bound with invalid reward[%s]. check Reward column"), *quest->UID.ToString(), *it.ToString());
			quest->_rewards.Emplace(reward);
			CacheFriend(reward, quest);
		}

		for (auto& it : quest->Reward_Bonus) {
			if (it.IsNone()) {
				break;
			}
			URefReward* reward = GetRefObj<URefReward>(it);
			checkRefMsgfRet(Error, reward, TEXT("quest[%s] bound with invalid manual reward[%s]. check Reward_Bonus column"), *quest->UID.ToString(), *it.ToString());
			quest->_manualRewards.Emplace(reward);
			CacheFriend(reward, quest);
		}

		FName classUID{ *quest->Class_Exp[0] };
		if (classUID.IsNone() == false) {
			checkRefMsgfRet(Error, quest->Class_Exp.Num() == 2, TEXT("quest[%s] Class_Exp format is invalid; expected> class.uid | exp"), *quest->UID.ToString());
			quest->_rewardClass = GetRefObj<URefClass>(classUID);
			checkRefMsgfRet(Error, quest->_rewardClass, TEXT("quest[%s] bound with invalid class[%s]"), *quest->UID.ToString(), *classUID.ToString());
			quest->_rewardClassExp = FCString::Atoi(*quest->Class_Exp[1]);
			checkRefMsgfRet(Error, quest->_rewardClassExp > 0, TEXT("quest[%s] bound with invalid Class_Exp; exp[%s] must be positive"), *quest->Class_Exp[1]);
		}

		auto& questList = _questBySubGroup.FindOrAdd(quest->SubGroup);
		questList.Emplace(quest);

		if (quest->_groupType == EQuestGroupType::Keyword) {
			for (auto& keywordCategory : quest->_descKeys) {
				if (keywordCategory.IsNone()) {
					continue;
				}

				{ // caching in category
					auto& keywords = URefTitle::KEYWORDS_BY_CATEGORY.FindOrAdd(keywordCategory);
					keywords.Emplace(quest);
				}				
				{ // caching in all
					auto& keywords = URefTitle::KEYWORDS_BY_CATEGORY.FindOrAdd("str.kwd.1.all");
					keywords.Emplace(quest);
				}
			}
		}
	});

	DoRefIterationJobNoUID<URefQuestSequence>([this](URefQuestSequence* questSequence) {
		URefQuest* quest = GetRefObj<URefQuest>(questSequence->Quest_UID);
		checkRefMsgfRet(Error, quest, TEXT("QuestSequence table error: not exist quest[%s]"), *questSequence->Quest_UID.ToString());
		questSequence->_quest = quest;
		quest->_sequences.Emplace(questSequence);

		for (auto& rewardUID : questSequence->Reward) {
			if (rewardUID.IsNone()) {
				break;
			}
			
			URefReward* reward = GetRefObj<URefReward>(rewardUID);
			checkRefMsgfRet(Error, reward, TEXT("QuestSequence[%s] bound with invalid Reward[%s]"), *questSequence->GetDebugString(), *rewardUID.ToString());
			questSequence->_reward.Emplace(reward);
		}

		for (auto& rewardUID : questSequence->Reward_Bonus) {
			if (rewardUID.IsNone()) {
				break;
			}

			URefReward* reward = GetRefObj<URefReward>(rewardUID);
			checkRefMsgfRet(Error, reward, TEXT("QuestSequence[%s] bound with invalid Reward_Bonus[%s]"), *questSequence->GetDebugString(), *rewardUID.ToString());
			questSequence->_manualRewards.Emplace(reward);
		}

		// host
		for (auto& it : questSequence->Host_Object) {
			if (it.IsNone()) {
				break;
			}

			URefObject* hostObj = GetRefObj<URefObject>(it);
			checkRefMsgfRet(Error, hostObj, TEXT("Not exist Host_Object[%s] of quest[%s:%d]. Check Object/QuestSequence table."), *it.ToString(), *questSequence->Quest_UID.ToString(), questSequence->Order);
			questSequence->_hostObjects.Emplace(hostObj);
			hostObj->_listenQuests.AddUnique(questSequence);

			if (hostObj->typeId.IsNPC()) {
				quest->_hostObject.Emplace(URefNPC::StaticClass(), hostObj);
			}
			else if (hostObj->typeId.IsLifeObject()) {
				quest->_hostObject.Emplace(URefLifeObject::StaticClass(), hostObj);
			}
			else if (hostObj->typeId.IsItem()) {
				quest->_hostObject.Emplace(URefItem::StaticClass(), hostObj);
			}
			else if (hostObj->typeId.IsMonster()) {
				quest->_hostObject.Emplace(URefMonster::StaticClass(), hostObj);
			}
		}

		PostQuestCondition(questSequence);
	});

	DoRefIterationJob<URefQuest>([this](URefQuest* quest) {
		checkRefMsgfRet(Error, quest->_sequences.Num() != 0, TEXT("[quest] Quest[%s] has any QuestSequence"), *quest->UID.ToString());
		quest->_sequences.Sort([this](const URefQuestSequence& lhs, const URefQuestSequence& rhs) {
			checkRefMsgf(Error, lhs.Order != rhs.Order, TEXT("[quest] sequence order duplicate: quest[%s], order[%d]"), *lhs.Quest_UID.ToString(), lhs.Order);
			return lhs.Order < rhs.Order;
		});

		checkRefMsgfRet(Error, quest->_group != nullptr, TEXT("[quest] Quest[%s] bound with invalid QuestGroup[%s]"), *quest->UID.ToString(), *quest->QuestGroup.ToString());
		FName eventFileName{ *FString::Printf(TEXT("%s/%s"), *quest->_group->Type.ToString(), *quest->UID.ToString()) };
		TSharedPtr<FJsonObject> eventJson = LoadQuestEventJson(eventFileName);
		if (eventJson == nullptr) {
			UE_LOG(LogReference, Verbose, TEXT("[quest] quest[%s] has no event file[%s]"), *quest->UID.ToString(), *eventFileName.ToString());
			return;
		}

		eventJson->TryGetBoolField("Skip_Complete_UI", quest->_skipCompleteUI);

		FillQuestEvent("Accept_Event", eventJson, quest->_acceptEvents);
		FillQuestEvent("Reward_Event", eventJson, quest->_rewardEvents);
		FillQuestEvent("Cookie_Event", eventJson, quest->_cookieEvents);

		for (auto& sequence : quest->_sequences) {
			FString sequenceKey{ FString::Printf(TEXT("Seq_%d"), sequence->Order) };
			const TSharedPtr<FJsonObject>* seqObjPtr;
			if (eventJson->TryGetObjectField(sequenceKey, seqObjPtr) == false) {
				UE_LOG(LogReference, Warning, TEXT("[quest] quest event[%s] has no key[%s]"), *eventFileName.ToString(), *sequenceKey);
				continue;
			}

			TSharedPtr<FJsonObject> seqObj{ *seqObjPtr };
			seqObj->TryGetBoolField("AutoPlay", sequence->_autoRunProgressEvent);
			FillQuestEvent("Progress_Event", seqObj, sequence->_progressEvents);
			FillQuestEvent("Complete_Event", seqObj, sequence->_completeEvents);
			FillQuestEvent("Situation_Event", seqObj, sequence->_situationEvents);
		}
		});

	DoRefIterationJobNoUID<URefQuestChallenge>([this](URefQuestChallenge* challenge) {
		URefQuest* challengeQuest = GetRefObj<URefQuest>(challenge->Quest_UID);
		checkRefMsgfRet(Error, challengeQuest, TEXT("QuestChallenge bound with invalid Quest[%s]"), *challenge->Quest_UID.ToString());
		challengeQuest->_challenges.Emplace(challenge);
		challenge->_quest = challengeQuest;
		challengeQuest->Repeat = 0; // override repeat count; challenge can be deactivate only by schedule

		for (auto& it : challenge->Host_Object) {
			if (it.IsNone()) {
				break;
			}
			URefObject* host = GetRefObj<URefObject>(it);
			checkRefMsgfRet(Error, host, TEXT("QuestChallenge[%s] bound wigh invalid Host_Object[%s]"), *challenge->GetDebugString(), *it.ToString());
			challenge->_hostObjects.Emplace(host);
		}

		challenge->_reward = GetRefObj<URefReward>(challenge->Reward);
		checkRefMsgfRet(Error, challenge->Reward.IsNone() || challenge->_reward, TEXT("QuestChallenge[%s] bound with invalid Reward[%s]"), *challenge->GetDebugString(), *challenge->Reward.ToString());

		for (auto& it : challenge->Ranking_Reward) {
			if (it.IsNone()) {
				break;
			}

			URefRankingReward* rwd = GetRefObj<URefRankingReward>(it);
			checkRefMsgfRet(Error, rwd, TEXT("QuestChallenge[%s] bound with invalid Ranking_Reward[%s]"), *challenge->GetDebugString(), *it.ToString());
			challenge->_rankingRewards.Emplace(rwd);
		}
	});

	_questEventJsonCache.Empty(); // free json objects

	DoRefIterationJobNoUID<URefQuestSchedule>([this](URefQuestSchedule* questSchedule) {
		URefQuestGroup* group = GetRefObj<URefQuestGroup>(questSchedule->QuestGroup_UID);
		checkRefMsgfRet(Error, group, TEXT("QuestAcitveSchedule bound with invalid QuestGroup[%s]"), *questSchedule->QuestGroup_UID.ToString());
		group->_appendSchedules.Emplace(questSchedule);
	});

	DoRefIterationJob<URefQuestGroup>([this](URefQuestGroup* questGroup) {
		for (auto& scheduleUID : questGroup->Active_Schedule) {
			auto refSchedule = GetRefObj<URefSchedule>(scheduleUID);
			checkRefMsgfRet(Error, scheduleUID.IsNone() || refSchedule, TEXT("[quest] QuestGroup[%s] bound with invalid Active_Schedule[%s]"), *questGroup->UID.ToString(), *scheduleUID.ToString());
			if (refSchedule) {
				questGroup->_activeSchedules.Emplace(refSchedule);
			}
		}

		if (questGroup->Quests.Num() != 0) {
			questGroup->Quests.Sort([](const URefQuest& lhs, const URefQuest& rhs) {
				return lhs.Order < rhs.Order;
			});
			questGroup->Quests.Last()->_lastInGroup = true;
		}

		if (questGroup->_type == EQuestGroupType::Arbeit || questGroup->_type == EQuestGroupType::MassiveArbeit) {
			URefQuestEvent* completeUIOpen{ GetRefObj<URefQuestEvent>(FName("open.ui.quest.complete")) };
			checkRefMsgfRet(Error, completeUIOpen, TEXT("QuestEvent[open.ui.quest.complete] must be registered in QuestEvent table for arbeit"));
			for (auto& arbeit : questGroup->Quests) {
				arbeit->_rewardEvents.Emplace(completeUIOpen);
			}
		}
		else if (questGroup->_type == EQuestGroupType::Challenge) {
			for (auto& quest : questGroup->Quests) {
				checkRefMsgfRet(Error, quest->_challenges.Num() != 0, TEXT("quest[%s] is challenge, but has no QuestChallenge"), *quest->UID.ToString());
				quest->_challenges.Sort([](const URefQuestChallenge& lhs, const URefQuestChallenge& rhs) {
					return lhs.Order < rhs.Order;
				});

				ReserveQuestEvent(URefQuestEvent::GetChallengeCeremonyEvtUID(quest), "Run_Challenge_Ceremony", quest->UID.ToString());
			}
		}

		for (auto& it : questGroup->Reward) {
			if (it.IsNone()) {
				break;
			}
			URefReward* reward = GetRefObj<URefReward>(it);
			checkRefMsgfRet(Error, reward, TEXT("quest group[%s] bound with invalid manual reward[%s]. check Reward column"), *questGroup->UID.ToString(), *it.ToString());
			questGroup->_rewards.Emplace(reward);
		}

		Algo::Sort(questGroup->_appendSchedules, [](URefQuestSchedule* lhs, URefQuestSchedule* rhs) {
			const FDateTime& lStartDate = lhs->_schedule->GetNextStartDateInUtc();
			const FDateTime& rStartDate = rhs->_schedule->GetNextStartDateInUtc();
			return lStartDate < rStartDate;
		});
	});

	DoRefIterationJob<URefQuestPass>([this](URefQuestPass* pass) {
		auto freeRewardSequence = pass->_sequences.FindByPredicate([](URefQuestSequence* sequence) {
			return sequence->_reward.Num() != 0;
		});
		pass->_premiumOnly = (freeRewardSequence == nullptr);

		pass->_missionGroup = GetRefObj<URefPassQuestGroup>(pass->MissionGroup_UID);
		checkRefMsgfRet(Error, pass->MissionGroup_UID.IsNone() || (pass->_missionGroup && pass->_missionGroup->_type == EQuestGroupType::PassMission), TEXT("Quest_Pass[%s] bound with invalid MissionGroup_UID[%s]"), *pass->UID.ToString(), *pass->MissionGroup_UID.ToString());

		for (auto& shopItemUID : pass->ShopItem_Reroll) {
			if (shopItemUID.IsNone()) {
				break;
			}
			URefShopItem* shopItem = GetRefObj<URefShopItem>(shopItemUID);
			checkRefMsgfRet(Error, shopItem, TEXT("Quest_Pass[%s] bound with invalid ShopItem[%s]; check ShopItem_Reroll"), *pass->UID.ToString(), *shopItemUID.ToString());
			pass->_rerollShopItems.Emplace(shopItem);
		}
	});
}

void UReferenceBuilder::PostQuestCondition(URefQuestSequence* questSeq)
{
	// condition
	static auto SetConditionTargetObjectMono = [](UReferenceBuilder* builder, URefQuestSequence* seq, URefObject* targetObject) {
		seq->_conditionTarget = targetObject;
		seq->_displayInfo = seq->_conditionTarget->_displayInfo;
		if (targetObject->typeId.IsItem() == false) {
			seq->_displayInfo._icon.Empty(); // icon can be shown only when target object is item
		}
	};
	static auto SetConditionTargetObject = [](UReferenceBuilder* builder, URefQuestSequence* seq) {
		FName targetUID{ seq->GetTargetObject() };
		if (targetUID.IsNone() == false) {
			URefObject* targetObject = builder->GetRefObj<URefObject>(targetUID);
			checkRefMsgfRet(Error, targetObject, TEXT("quest sequence[%s] bound with invalid Condition_Ref[Object | %s]; invalid object"), *seq->GetDebugString(), *targetUID.ToString());
			SetConditionTargetObjectMono(builder, seq, targetObject);
		}
		else if (auto objTypeRule = seq->GetRule("RefObjectType")) {
			if (objTypeRule->Values.Num() > 1) {
				FName tid1 = *objTypeRule->Values[0];
				if (tid1 == URefLifeObject::NAME_TID_1) {
					TypeID targetTID;
					targetTID.typeID1 = objTypeRule->Values.IsValidIndex(0) ? builder->_typeNames.FindRef(*objTypeRule->Values[0]) : 0;
					targetTID.typeID2 = objTypeRule->Values.IsValidIndex(1) ? builder->_typeNames.FindRef(*objTypeRule->Values[1]) : 0;
					targetTID.typeID3 = objTypeRule->Values.IsValidIndex(2) ? builder->_typeNames.FindRef(*objTypeRule->Values[2]) : 0;
					targetTID.typeID4 = objTypeRule->Values.IsValidIndex(3) ? builder->_typeNames.FindRef(*objTypeRule->Values[3]) : 0;
					
					if (auto it = builder->_lifeObjByTID.Find(targetTID.typeID)) {
						URefLifeObject* representative = *it;
						SetConditionTargetObjectMono(builder, seq, representative);
						seq->_displayInfo._name.Empty();
						seq->_displayInfo._name.Emplace(representative->Action_Name);
					}
					else if (auto targetClass = builder->GetClassByTid2(*objTypeRule->Values[1])) {
						seq->_conditionTarget = targetClass;
						seq->_displayInfo._name.Emplace(targetClass->GetRawName());
					}
				}
			}
		}
	};
	static auto SetConditionTargetCurrency = [](UReferenceBuilder* builder, URefQuestSequence* seq) {
		FName targetUID{ seq->GetTargetCurrency() };
		if (targetUID.IsNone() == false) {
			seq->_conditionTarget = builder->GetRefObj<URefCurrency>(targetUID);
			checkRefMsgfRet(Error, seq->_conditionTarget, TEXT("quest sequence[%s] bound with invalid Condition_Ref[RefCurrency | %s]; invalid currency"), *seq->GetDebugString(), *targetUID.ToString());
			seq->_displayInfo._name.Append(seq->_conditionTarget->_displayInfo._name);
		}
	};
	static auto SetConditionTargetUserInteraction = [](UReferenceBuilder* builder, URefQuestSequence* seq) {
		if (auto interactionChecker = seq->GetRule("RefUserInteraction")) {
			const FString& targetUID = interactionChecker->Values[0];
			URefUserInteraction* targetInteraction = builder->GetRefObj<URefUserInteraction>(targetUID);
			checkRefMsgfRet(Error, targetInteraction, TEXT("quest sequence[%s] bound with invalid Condition_Ref[RefUserInteraction | %s]; invalid user interaction"), *seq->GetDebugString(), *targetUID);
			seq->_conditionTarget = targetInteraction;
			seq->_displayInfo._name.Emplace(targetInteraction->Name);
		}
		FName targetUID{ seq->GetTargetCurrency() };
		if (targetUID.IsNone() == false) {
			seq->_conditionTarget = builder->GetRefObj<URefCurrency>(targetUID);
			checkRefMsgfRet(Error, seq->_conditionTarget, TEXT("quest sequence[%s] bound with invalid Condition_Ref[RefCurrency | %s]; invalid currency"), *seq->GetDebugString(), *targetUID.ToString());
			seq->_displayInfo._name.Append(seq->_conditionTarget->_displayInfo._name);
		}
	};

	static TMap<FName, TFunction<void(UReferenceBuilder*, URefQuestSequence*)>> ConditionPostHandler{
		{ "Complete_Quest", [](UReferenceBuilder* builder, URefQuestSequence* seq) {
			FName targetQuestUID{ seq->GetTargetQuest() };
			if (targetQuestUID.IsNone() == false) {
				URefQuest* thisQuest = seq->_quest;
				URefQuest* targetQuest = builder->GetRefObj<URefQuest>(targetQuestUID);
				checkRefMsgfRet(Error, targetQuest, TEXT("quest sequence[%s] Condition[Complete_Quest] bound with invalid Condition_Ref[RefQuest | %s]; not registered in Quest"), *seq->GetDebugString(), *targetQuestUID.ToString());
				seq->_quest->_listenQuests.Emplace(targetQuest);

				if (URefQuestGroup::CompleteUITypes.Find(thisQuest->_groupType)
					&& thisQuest->_sequences[thisQuest->_sequences.Num() - 1] == seq
					&& URefQuestGroup::CompleteUITypes.Find(targetQuest->_groupType)) {
					targetQuest->_skipCompleteUI = true;
					UE_LOG(LogReference, Verbose, TEXT("[quest] quest[%s] skip complete ui; because parent quest[%s] has higher priority when complete-ui"), *targetQuest->UID.ToString(), *thisQuest->UID.ToString());
				}
			}
		}},
		{ "Get_Item", SetConditionTargetObject},
		{ "Get_Currency", SetConditionTargetCurrency},
		{ "Act_LifeObject", SetConditionTargetObject},
		{ "Act_Streaming", [](UReferenceBuilder* builder, URefQuestSequence* seq) {
			seq->_conditionTarget = seq;
			seq->_displayInfo._name.Emplace(AnuText::Get_CommonTable("str.stream.type.LiveStream"));
			seq->_displayInfo._icon.Emplace(builder->GetResourceRoute<UTexture2D>("res.icon.stream.LiveStream"));
		}},
		{ "Act_Interview", [](UReferenceBuilder* builder, URefQuestSequence* seq) {
			seq->_conditionTarget = seq;
			seq->_displayInfo._name.Emplace(AnuText::Get_CommonTable("str.stream.type.Interview"));
			seq->_displayInfo._icon.Emplace(builder->GetResourceRoute<UTexture2D>("res.icon.stream.Interview"));
		}},
		{ "Act_Inspecting", [](UReferenceBuilder* builder, URefQuestSequence* seq) {
			seq->_conditionTarget = seq;
			seq->_displayInfo._name.Emplace(AnuText::Get_CommonTable("str.stream.type.Inspecting"));
			seq->_displayInfo._icon.Emplace(builder->GetResourceRoute<UTexture2D>("res.icon.stream.Inspecting"));
		}},
		{ "Act_TakePhoto", [](UReferenceBuilder* builder, URefQuestSequence* seq) {
			seq->_conditionTarget = seq;
			seq->_displayInfo._name.Emplace(AnuText::Get_CommonTable("str.stream.type.TakePhoto"));
			seq->_displayInfo._icon.Emplace(builder->GetResourceRoute<UTexture2D>("res.icon.stream.TakePhoto"));
		}},
		{ "Act_ItemReview", [](UReferenceBuilder* builder, URefQuestSequence* seq) {
			seq->_conditionTarget = seq;
			seq->_displayInfo._name.Emplace(AnuText::Get_CommonTable("str.platform.category.item.review"));
			seq->_displayInfo._icon.Emplace(builder->GetResourceRoute<UTexture2D>("res.icon.item.review"));
		}},
		{ "Act_ItemTrade", [](UReferenceBuilder* builder, URefQuestSequence* seq) {
			seq->_conditionTarget = seq;
			seq->_displayInfo._name.Emplace(AnuText::Get_CommonTable("str.platform.category.item.trade"));
			seq->_displayInfo._icon.Emplace(builder->GetResourceRoute<UTexture2D>("res.icon.item.trade"));
		}},
		{ "Act_UserInteraction", SetConditionTargetUserInteraction },
	};

	if (auto it = ConditionPostHandler.Find(questSeq->Condition)) {
		(*it)(this, questSeq);
	}

	// condition ref
	static TMap<FName, TFunction<void(UReferenceBuilder*, URefQuestSequence*, FConditionRule&)>> ConditionRefPostHandler{
		{ "Schedule", [](UReferenceBuilder* builder, URefQuestSequence* seq, FConditionRule& checker) {
			seq->_conditionSchedule = builder->GetRefObj<URefSchedule>(checker.Values[0]);
			checkRefMsgfRet(Error, seq->_conditionSchedule, TEXT("Schedule checker bound with invalid Schedule[%s] in QuestSequence[%s]"), *checker.Values[0], *seq->GetDebugString());
		}},
	};
	for (auto& checker : questSeq->_completeConditions) {
		if (auto it = ConditionRefPostHandler.Find(checker.Rule)) {
			(*it)(this, questSeq, checker);
		}
	}
}

void UReferenceBuilder::URefTitlePostProcessor()
{
	DoRefIterationJob<URefTitle>([this](URefTitle* title) {
		for (auto& it : title->Keywords) {
			URefQuest* quest = GetRefObj<URefQuest>(it);
			checkRefMsgfRet(Error, quest, TEXT("Title: Keyword quest[%s] not exist. Check Title/Quest table"), *it.ToString());
			title->_keywords.Emplace(quest);
			quest->_titles.Emplace(title);
		}
	});
}

void UReferenceBuilder::RewardCommonPostProcessor(URefRewardBase* reference, ERewardType type, bool fromReference)
{
	check(reference);
	FName rewardItemUID{ reference->Reward_Item_UID };
	reference->_rewardObjType = URefReward::GetType(reference->Reward_Type);
	reference->_item = GetRefObj<URefItem>(rewardItemUID);
	reference->_character = GetRefObj<URefCharacter>(rewardItemUID);
	reference->_statType = URefObject::GetStatEnum(reference->Reward_Item_UID);
	if (reference->_statType != EStatTypes::None) {
		reference->_rewardObjectName = URefObject::GetStatText(reference->_statType);
	}

	if (auto currency = GetRefObj<URefCurrency>(rewardItemUID)) {
		reference->_currency = currency;
		reference->_rewardObjectName = currency->Name;

		if (currency == UReferenceBuilder::PopularityCurrency) {
			reference->_rewardObjType = ERewardObjectType::Popularity;
		}
	}
	else if (auto title = GetRefObj<URefTitle>(rewardItemUID)) {
		reference->_title = title;
		reference->_implGuid = title->GUID;
		reference->_rewardObjectName = title->GetRawName();
	}
	else if (auto palette = GetRefObj<URefPaletteGroup>(rewardItemUID)) {
		reference->_palette = palette;
		reference->_implGuid = palette->GUID;
		reference->_rewardObjectName = palette->Name;
	}
	else if (reference->_rewardObjType == ERewardObjectType::ClassLicense) {
		TArray<FString> values;
		URefBase::GetTrimedStringArray(rewardItemUID.ToString(), values);
		URefClass* refClass = GetRefObj<URefClass>(values[0]);
		checkf(refClass, TEXT("reward[%s] bound with invalid ClassUID[%s]"), *reference->Reward_Item_UID.ToString(), *reference->Reward_UID.ToString());
		reference->_class = refClass;
		reference->LicenseType = URefClass::GetLicenceType(values[1]);
		checkf(reference->LicenseType != EClassLicense::None, TEXT("reward[%s] bound with invalid ClassLicense Type[%s]"), *reference->Reward_Item_UID.ToString(), *reference->Reward_UID.ToString());
		if (UDataTable* table = GetResourceTable<URefClass>()) {
			table->ForeachRow<FAnuResourceClassLicense>(TEXT(""), [reference](const FName& rowName, const FAnuResourceClassLicense& value){
				if (value.Type == reference->LicenseType) {
					reference->_rewardObjectName = value.Name;
				}
			});
		}
	}
	else if (auto chatSticker = GetRefObj<URefChatStickerGroup>(rewardItemUID)) {
		reference->_chatSticker = chatSticker;
		reference->_implGuid = chatSticker->GUID;
		reference->_rewardObjectName = chatSticker->Name;
	}
	else if (auto resRow = GetResourceRow(UAnuSocialMotion::StaticClass(), rewardItemUID)) {
		FAnuResourceSocialMotion* res = static_cast<FAnuResourceSocialMotion*>(resRow);
		reference->_implGuid = res->GUID;
		reference->_rewardObjectName = res->Name;
	}
	else if (auto tag = GetRefObj<URefTag>(rewardItemUID)) {
		reference->_tag = tag;
		reference->_implGuid = tag->GUID;
		reference->_rewardObjectName = tag->Name;
	}

	if (fromReference) {
		reference->_reward = GetRefObj<URefReward>(reference->Reward_UID);
	}

	checkf(reference->_reward, TEXT("reward[%s] bound with invalid Reward[%s]"), *reference->Reward_Item_UID.ToString(), *reference->Reward_UID.ToString());

	reference->_reward->RewardItems.Emplace(reference);

	URefReward* rewardRef = reference->_reward;
	checkRefMsgfRet(Error, rewardRef->RewardType == type || rewardRef->RewardType == ERewardType::Invalid,
		TEXT("Reward[%s] child reward registered in another type table; check Reward table[%s, %s]"),
		*reference->Reward_UID.ToString(),
		*rewardRef->GetEnumValueString("RewardType", static_cast<int64>(type)),
		*rewardRef->GetEnumValueString("RewardType", static_cast<int64>(rewardRef->RewardType)));
	reference->_reward->RewardType = type;

	for (auto& tagUID : reference->Display_Tag) {
		if (tagUID.IsNone()) {
			break;
		}
		URefTag* tag = GetRefObj<URefTag>(tagUID);
		checkRefMsgfRet(Error, tag, TEXT("Reward[%s] bound with invalid Display_Tag[%s]"), *reference->Reward_Item_UID.ToString(), *tagUID.ToString());
		reference->_displayTags.Emplace(tag);
	}
}

void UReferenceBuilder::URefRewardPostProcessor()
{
	FName popularityUID{ "popularity" };
	UReferenceBuilder::PopularityCurrency = GetRefObj<URefCurrency>(popularityUID);
	checkRefMsgfRet(Error, UReferenceBuilder::PopularityCurrency, TEXT("Currency[%s] not exist. it's critical..!"), *popularityUID.ToString());

	DoRefIterationJobNoUID<URefRewardStatic>([this](URefRewardStatic* reference) {
		RewardCommonPostProcessor(reference, ERewardType::Static);
	});

	DoRefIterationJobNoUID<URefRewardRandom>([this](URefRewardRandom* reference) {
		RewardCommonPostProcessor(reference, ERewardType::Random);
		check(reference->_reward);
		reference->_reward->_sumOfProbability += reference->Prob;
	});

	DoRefIterationJobNoUID<URefRewardSelectable>([this](URefRewardSelectable* reference) {
		RewardCommonPostProcessor(reference, ERewardType::Selectable);
	});

	DoRefIterationJobNoUID<URefRewardPost>([this](URefRewardPost* reference) {
		RewardCommonPostProcessor(reference, ERewardType::PostBox);
	});

	DoRefIterationJobNoUID<URefRewardPeriod>([this](URefRewardPeriod* reference) {
		RewardCommonPostProcessor(reference, ERewardType::Period);
		
		static FName TriggerKey{"Calendar"};
		if (TriggerKey == *reference->Source_Contents[0]) {
			TArray<FString> schedules;
			schedules.Append(reference->Source_Contents);
			schedules.RemoveAt(0);
			checkRefMsgfRet(Error, schedules.Num() != 0, TEXT("[calendar] RewardPeriod[%s] bound with invalid calendar"), *reference->Reward_Item_UID.ToString());

			if (FPeriodCondition::GetPeriodType(*schedules[0]) != FPeriodCondition::Type::None) {
				URefSchedule* dynamicSchedule = NewObject<URefSchedule>(reference);
				FName periodType= *schedules[0];
				FString periodValue;
				for (int32 i = 1; i < schedules.Num(); i++) {
					periodValue += FString::Printf(TEXT("%s%s"), *schedules[i], i != schedules.Num() - 1 ? TEXT(" | ") : TEXT(""));
				}
				bool res = dynamicSchedule->PeriodCondition.Parse(periodType, periodValue);
				checkf(res, TEXT("[reward] RewardPeriod[%s] has invalid dynamic schedule data; check Source_Contents"), *reference->Reward_UID.ToString());
				reference->_calendar.Emplace(dynamicSchedule);
			}
			else { // bound with schedule references
				for (auto& scheduleIt : schedules) {
					URefSchedule* schedule = GetRefObj<URefSchedule>(scheduleIt);
					checkRefMsgfRet(Error, schedule, TEXT("[calendar] RewardPeriod[%s] bound with invalid calendar; not exist Schedule[%s]/Period_Type"), *reference->Reward_Item_UID.ToString(), *scheduleIt);
					reference->_calendar.Emplace(schedule);
				}
			}
		}
	});

	DoRefIterationJob<URefReward>([this](URefReward* reference) {
		reference->_displayInfo._name.Emplace(reference->Name);
		reference->_displayInfo._desc.Emplace(reference->Desc);
		reference->_displayInfo._icon.Emplace(GetResourceRoute<UTexture2D>(reference->Icon));

		if (reference->RewardType == ERewardType::Selectable) {
			TSet<FName> selectGroups;
			for (auto& it : reference->RewardItems) {
				URefRewardSelectable* selectable = Cast<URefRewardSelectable>(it.Reward);
				checkRefMsgfRet(Error, selectable, TEXT("Reward[%s] child reward registered duplicately in each another tables"), *reference->UID.ToString());
				selectGroups.Emplace(selectable->Select_Group);
			}
			reference->_rewardCount = selectGroups.Num();
		}
		else if (reference->RewardType == ERewardType::Period) {
			for (auto& it : reference->RewardItems) {
				SAFE_CAST(it.Reward, URefRewardPeriod, rewardPeriod);
				if (rewardPeriod->_calendar.Num() != 0) {
					_calendars.Emplace(rewardPeriod);
				}
			}
		}
		else {
			reference->_rewardCount = reference->RewardItems.Num();
		}

		if (auto clRwd = reference->GetReward(ERewardObjectType::ClassLicense)) {
			URefClass* refClass = clRwd->_class;
			auto& statRwd = refClass->GetRTLicense(clRwd->LicenseType)._statRwd;
			checkRefMsgf(Warning, statRwd.Num() == 0, TEXT("Reward[%s] has Class[%s]; but license Type[%d] stats are filled from other reason.."), *reference->UID.ToString(), *refClass->UID.ToString(), clRwd->LicenseType);
			statRwd.Empty();
			reference->Visit(ERewardObjectType::Stat, [&statRwd](URefRewardBase* rwd, int32 amount) {
				FStatInfoData statData;
				statData.StatString = rwd->Reward_Item_UID;
				statData.StatType = rwd->_statType;
				statData.Value = rwd->Amount;
				statData.ApplyType = EStatApplyType::Add;
				statRwd.Emplace(MoveTemp(statData));
			});
		}
	});

	DoRefIterationJobNoUID<URefSubscriptionReward>([this](URefSubscriptionReward* reference) {
		URefReward* benefitRwd = GetRefObj<URefReward>(reference->Reward_UID);
		checkRefMsgf(Error, benefitRwd, TEXT("SubscriptionReward[%s] bound with invalid Reward"), *reference->Reward_UID.ToString());
		benefitRwd->_subsRwd.Emplace(reference);

		reference->_bonusReward = GetRefObj<URefReward>(reference->Bonus_Reward);
		checkRefMsgf(Error, reference->_bonusReward, TEXT("SubscriptionReward[%s] bound with invalid Bonus_Reward[%s]"), *reference->Reward_UID.ToString(), *reference->Bonus_Reward.ToString());
	});
}

void UReferenceBuilder::URefChatStickerGroupPostProcessor()
{
	DoRefIterationJob<URefChatStickerGroup>([this](URefChatStickerGroup* group) {
		group->ImageTexture = GetResourceRoute<UTexture2D>(group->Icon);
	});
}

void UReferenceBuilder::URefStageGroupPostProcessor()
{
	int32 index = 0;
	DoRefIterationJob<URefStageGroup>([this, &index](URefStageGroup* reference) {
		reference->Index = index++;
	});
}

void UReferenceBuilder::URefStageInfoPostProcessor()
{
	DoRefIterationJob<URefStageInfo>([this](URefStageInfo* reference) {
		reference->_region = GetRefObj<URefRegion>(reference->Region);
		URefStageGroup* regStageGroup = GetRefObj<URefStageGroup>(reference->StageGroup_UID);
		checkRefMsgfRet(Error, regStageGroup, TEXT("StageInfo invalid: stage[%s]-> StageGroup[%s] not exist"), *reference->UID.ToString(), *reference->StageGroup_UID.ToString());
		regStageGroup->_stages.Emplace(reference);
		reference->Asset_Icon = GetResourceRoute<UTexture2D>(reference->Icon);
		reference->_refStageGroup = regStageGroup;

		for (auto& it : reference->Display_Reward) {
			if (it.IsNone()) {
				break;
			}

			URefBase* target = GetRefObj<URefItem>(it);
			if (target == nullptr) {
				target = GetRefObj<URefCurrency>(it);
			}
			checkRefMsgfRet(Error, target, TEXT("StageInfo invalid: stage[%s]-> Display_Item[%s] not exist; check Item/Currency"), *reference->UID.ToString(), *it.ToString());
			reference->_displayRewards.Emplace(target);
		}

		reference->_unlockCondition = GetRefObj<URefStageInfo>(reference->Unlock_Condition);
		reference->_costCurrency.Reserve(reference->Currency.Num());
		for (auto& pair : reference->Currency) {
			URefCurrency* currency = GetRefObj<URefCurrency>(pair.Key);
			reference->_costCurrency.Emplace(currency, pair.Value);
		}
		reference->_resetSchedule = GetRefObj<URefSchedule>(reference->ResetSchedule);
		checkRefMsgfRet(Error, reference->ResetSchedule.IsNone() || reference->_resetSchedule, TEXT("StageInfo invalid: stage[%s]-> ResetSchedule[%s] not exist"), *reference->UID.ToString(), *reference->ResetSchedule.ToString());

		reference->_openSchedule = GetRefObj<URefSchedule>(reference->OpenSchedule);
		checkRefMsgfRet(Error, reference->OpenSchedule.IsNone() || reference->_openSchedule, TEXT("StageInfo invalid: stage[%s]-> OpenSchedule[%s] not exist"), *reference->UID.ToString(), *reference->OpenSchedule.ToString());

		DoRefIterationJobNoUID<URefStageReward>([reference](URefStageReward* refReward) {
			if (refReward->StageReward_UID == reference->StageReward_UID) {
				reference->_rewards.Emplace(refReward);
			}
		});

		for (auto& it : reference->Redirect_Stage) {
			URefStageInfo* stageInfo = GetRefObj<URefStageInfo>(it);
			checkRefMsgfRet(Error, stageInfo || it.IsNone(), TEXT("StageInfo invalid: stage[%s]-> Redirect_Stage[%s] not exist"), *reference->UID.ToString(), *it.ToString());
			if (stageInfo) {
				reference->_redirectStage.Emplace(stageInfo);
			}
		}
	});

	TMap<FName, TMap<EStageDifficulty, TArray<URefStageGroup*>>> GroupRegions;
	DoRefIterationJob<URefStageGroup>([this, &GroupRegions](URefStageGroup* reference) {
		for (auto& questUID : reference->Keywords) {
			if (URefQuest* refQuest = GetRefObj<URefQuest>(questUID)) {
				reference->_keywords.Emplace(refQuest);
			}
		}

		reference->_regionText = AnuText::Get_CommonTable(FString::Printf(TEXT("str.name.%s"), *reference->Region.ToString()));
		reference->_regionDescText = AnuText::Get_CommonTable(FString::Printf(TEXT("str.desc.%s"), *reference->Region.ToString()));

		reference->_stages.Sort([](const URefStageInfo& lhs, const URefStageInfo& rhs) { return lhs.Order < rhs.Order; });
		if (reference->Difficulty != EStageDifficulty::None) {
			auto& region = GroupRegions.FindOrAdd(reference->Region);
			auto& groups = region.FindOrAdd(reference->Difficulty);
			groups.Emplace(reference);
		}

		if (reference->Episode_UID.IsNone() == false) {
			reference->_episode = GetRefObj<URefQuestGroup>(reference->Episode_UID);
			checkRefMsgfRet(Error, reference->_episode, TEXT("[episode] StageGroup[%s] bound with invalid Episode_UID[%s]; check QuestGroup table"), *reference->UID.ToString(), *reference->Episode_UID.ToString());
		}

		if (reference->Type > EStageType::Normal) {
			switch (reference->Type) {
			case EStageType::Raid:
				reference->_permissions.Append({ EPermissionType::MMO_Stage_Raid_Observation });
				break;
			case EStageType::Fashion:
				reference->_permissions.Append({ EPermissionType::MMO_Stage_Raid_Observation });
				break;
			case EStageType::Game:
				reference->_permissions.Append({ EPermissionType::MMO_Games });
				break;
			case EStageType::Multi:
				reference->_permissions.Append({ EPermissionType::MMO_Stage });
				break;
			case EStageType::Tower:
				reference->_permissions.Append({ EPermissionType::Tower });
				break;
			};
		}
		
	});

	for (auto& it : GroupRegions) {
		for (auto& groups : it.Value) {
			groups.Value.Sort([](const URefStageGroup& lhs, const URefStageGroup& rhs) { return lhs.Order < rhs.Order; });
			int32 index = 0;
			for (auto& refGroup : groups.Value) {
				for (auto& refStage : refGroup->_stages) {
					refStage->Display_Order = (++index);
				}
			}
		}
	}
}

void UReferenceBuilder::URefStageContestPostProcessor()
{
	DoRefIterationJob<URefStageContest>([this](URefStageContest* reference) {

		for (auto uid : reference->StageGroupUID) {
			auto refGroup = GetRefObj<URefStageGroup>(uid);
			check(refGroup != nullptr);
			for (auto refStage : refGroup->_stages) {
				reference->_stages.Emplace(refStage);
				refStage->_refStageContest = reference;

				checkRefMsgfRet(Error, refStage->_region, TEXT("StageInfo[%s] bound with invalid Region[%s]"), *refStage->UID.ToString(), *refStage->Region.ToString());
				refStage->_region->_contest = reference;

				for (auto one : refStage->_redirectStage) {
					one->_refStageContest = reference;
					one->_region->_contest = reference;
				}
			}
		}
		reference->_closeSchedule = GetRefObj<URefSchedule>(reference->CloseScheduleUID);
		checkRefMsgfRet(Error, reference->_closeSchedule || reference->CloseScheduleUID.IsNone(), TEXT("StageContest[%s] bound with invalid Close Schedule[%s]"), *reference->UID.ToString(), *reference->CloseScheduleUID.ToString());

		reference->_openSchedule = GetRefObj<URefSchedule>(reference->OpenScheduleUID);
		checkRefMsgfRet(Error, reference->_openSchedule || reference->OpenScheduleUID.IsNone(), TEXT("StageContest[%s] bound with invalid Open Schedule[%s]"), *reference->UID.ToString(), *reference->OpenScheduleUID.ToString());

		// reward
		DoRefIterationJob<URefRankingReward>([this, reference](URefRankingReward* reward) {
			if (reference->RankingRewardName.IsEqual(reward->Ranking_Name)) {
				reference->_rewards.Emplace(reward);
			}
		});

		reference->_rewards.Sort([](const URefRankingReward& lhs, const URefRankingReward& rhs) {
			return lhs._rangeValues[0] < rhs._rangeValues[0];
		});
	});
}

void UReferenceBuilder::URefDialogPostProcessor()
{
#if WITH_EDITOR
	CommitDialogStringTable();
#endif

	// Dialog
	DoRefIterationJob<URefDialog>([this](URefDialog* reference) {
		for (auto& speech : reference->_speeches) {
			speech->_dlg = reference;

			speech->_character = GetRefObj<URefCharacter>(speech->Character_UID);
			checkRefMsgfRet(Error, speech->_character, TEXT("DialogSub[%s] bound with invalid character[%s]"), *speech->GetDebugString(), *speech->Character_UID.ToString());
			speech->_isPCSpeech = speech->_character->typeId.IsPC();
			reference->_participants.Emplace(speech->_character);

			if (speech->Emotion.Num() == 1 && speech->Emotion[0] == NAME_None) {
				speech->Emotion.Empty();
			}

			// check choice speech
			checkRefMsgf(Error, speech->IsChoicable() == false || speech->Choice_ID.Find(speech->ID) == INDEX_NONE, TEXT("DialogSub[%s] has invalid Choice_ID[%d] which is same with ID; may occur circular flow error"), *speech->GetDebugString(), speech->ID);
			checkRefMsgf(Error, speech->IsChoicable() == false || speech->Choice_ID.Find(speech->Go_ID) == INDEX_NONE, TEXT("DialogSub[%s] has invalid Choice_ID[%d] which is same with Go_ID; may occur circular flow error"), *speech->GetDebugString(), speech->Go_ID);
			checkRefMsgf(Error, speech->Go_ID != speech->ID, TEXT("DialogSub[%s] has invalid Go_ID[%d] which is same with ID; may occur circular flow error"), *speech->GetDebugString(), speech->ID);
		}

		reference->_schedule = GetRefObj<URefSchedule>(reference->Schedule);

		checkRefMsgf(Error, reference->_speeches.Num() != 0, TEXT("Dialog[%s] has any DialogSub. check DialogSub table"), *reference->UID.ToString());
		reference->_speeches.Sort([](const URefDialogSub& lhs, const URefDialogSub& rhs) {
			checkRefMsgf(Error, lhs.ID != rhs.ID, TEXT("Dialog[%s] has duplicated ordered sub dialogs"), *lhs.GetDebugString());
			return lhs.ID < rhs.ID;
		});

		URefCharacter* character = GetRefObj<URefCharacter>(reference->Owner_Character_UID);
		checkRefMsgfRet(Error, character, TEXT("Dialog[%s].Owner_Character_UID[%s] invalid. check Dialog/Character table"), *reference->UID.ToString(), *reference->Owner_Character_UID.ToString());
		reference->_owner = character;
		auto& dialogs = character->_dialogbyTriggerType.FindOrAdd(reference->Trigger_Type);
		dialogs.Emplace(reference);

		// item trade dialog automation
		if (reference->Trigger_Type == URefItemTrade::AutomationTrigger_Success) {
			FString winnerCondition{ FString::Printf(TEXT("IsWinner | %s | 1"), *reference->Owner_Character_UID.ToString()) };
			for (auto& dlgSub : reference->_speeches) {
				FConditionRule::Parse(dlgSub->_playCondition, winnerCondition);
			}
		}
		else if (reference->Trigger_Type == URefItemTrade::AutomationTrigger_Fail) {
			FString loserCondition{ FString::Printf(TEXT("IsWinner | %s | 0"), *reference->Owner_Character_UID.ToString()) };
			for (auto& dlgSub : reference->_speeches) {
				FConditionRule::Parse(dlgSub->_playCondition, loserCondition);
			}
		}
	});
}

void UReferenceBuilder::URefCustomDetailPostProcessor()
{
	URefCustomDetail* detailBasic = nullptr;
	auto& groupRef = _refGroups.FindOrAdd(URefCustomDetail::StaticClass());
	DoRefIterationJob<URefCustomDetail>([this, &detailBasic, &groupRef](URefCustomDetail* reference) {
		for (auto& value : reference->CustomValue) {
			reference->_numValue.Emplace(FCString::Atoi(*value));
		}

		for (auto& partName : reference->Parts) {
			if (partName.IsNone()) {
				detailBasic = reference;
				return;
			}
			auto& groupData = groupRef.FindOrAdd(partName);
			groupData.Emplace(reference);
		}
		});

	if (detailBasic != nullptr) {
		for (auto& itr : groupRef) {
			auto& groupData = groupRef.FindOrAdd(itr.Key);
			groupData.Emplace(detailBasic);
		}
	}
}

void UReferenceBuilder::URefSkillTimelinePostProcessor()
{
	DoRefIterationJob<URefSkillTimeline>([this](URefSkillTimeline* reference) {
		BuildSkillTimelineEffect(reference);
	});

	DoRefIterationJob<URefSkill>([this](URefSkill* reference) {
		reference->DamageCount = 0;
		for (auto& pair : reference->_refTimelines) {
			for (auto one : pair.Value._timelines) {
				if (one->_refEffect == nullptr) {
					continue;
				}

				if (one->_refEffect->Effect == SkillEffect::Damage) {
					reference->DamageCount++;
				}
			}
			break;
		}
	});
}

void UReferenceBuilder::URefCurrencyPostProcessor()
{
	TMap<URefSchedule*, TArray<URefCurrency*>> groupBySchedule;
	DoRefIterationJob<URefCurrency>([this, &groupBySchedule](URefCurrency* reference) {
		reference->_displayInfo._name.Emplace(reference->Name);
		reference->_displayInfo._desc.Emplace(reference->Desc);
		reference->_displayInfo._icon.Emplace(GetResourceRoute<UTexture2D>(reference->Icon));

		reference->_advertisementShopItem = GetRefObj<URefShopItem>(reference->Advertisement_Shop_UID);
		checkRefMsgfRet(Error, reference->_advertisementShopItem || reference->Advertisement_Shop_UID.IsNone(), TEXT("currency[%s] bound with invalid Advertisement_Shop_UID[%s]"), *reference->UID.ToString(), *reference->Advertisement_Shop_UID.ToString());
		if (reference->_advertisementShopItem) {
			checkRefMsgfRet(Error, reference->_advertisementShopItem->Target_UID == reference->UID, TEXT("currency[%s] Advertisement_Shop_UID[%s] item must to be same currency"), *reference->UID.ToString(), *reference->Advertisement_Shop_UID.ToString());
		}

		reference->_displaySchedule = GetRefObj<URefSchedule>(reference->Display_Schedule);
		checkRefMsgfRet(Error, reference->Display_Schedule.IsNone() || reference->_displaySchedule, TEXT("currency[%s] bound with invalid Display_Schedule[%s]"), *reference->UID.ToString(), *reference->Display_Schedule.ToString());

		if (reference->_displaySchedule) {
			auto& group = groupBySchedule.FindOrAdd(reference->_displaySchedule);
			group.Emplace(reference);
		}
	});

	DoRefIterationJob<URefCurrency>([&groupBySchedule](URefCurrency* currency) {
		if (currency->_displaySchedule == nullptr) {
			return;
		}
		auto& group = groupBySchedule.FindOrAdd(currency->_displaySchedule);
		currency->_groupBySchedule.Append(group);
		currency->_groupBySchedule.Remove(currency);
	});
}

void UReferenceBuilder::URefReplyPostProcessor()
{
	for (auto& replyIter : _replys) {
		replyIter.Value.Sort([](const URefReply& lhs, const URefReply& rhs) {
			return lhs.Order < rhs.Order;
			});

		for (auto& it : replyIter.Value) {
			URefObject* host = GetRefObj<URefObject>(it->Host);
			checkRefMsgfRet(Error, host, TEXT("Reply[%s] host[%s] not exist. Check Reply/Object table"), *it->UID.ToString(), *it->Host.ToString());
			it->_host = host;
		}
	}
}

void UReferenceBuilder::URefStreamingPostProcessor()
{
	DoRefIterationJob<URefStreaming>([this](URefStreaming* reference) {
		reference->_schedule = GetRefObj<URefSchedule>(reference->Schedule);
		checkRefMsgfRet(Error, reference->Schedule.IsNone() || reference->_schedule, TEXT("Streaming[%s] bound with invalid Schedule[%s]"), *reference->UID.ToString(), *reference->Schedule.ToString());

		for (auto& it : reference->_openCondition) {
			if (it.Rule == FConditionRule::NAME_QuestProgress) {
				checkf(it.Values.Num() >= 2, TEXT("Streaming[%s] bound invalid checker: QuestProgress | quest.uid | quest.state | {seq.order}"), *reference->UID.ToString());
				URefQuest* listenQuest = GetRefObj<URefQuest>(it.Values[0]);
				checkRefMsgfRet(Error, listenQuest, TEXT("Streaming[%s] bound with invalid Quest[%s] in Open_Condition(QuestProgress)"), *reference->UID.ToString(), *it.Values[0]);
				reference->_listenQuests.Emplace(listenQuest);
			}
		}

		for (auto& it : reference->Display_Tag) {
			if (it.IsNone()) {
				break;
			}

			URefTag* tag = GetRefObj<URefTag>(it);
			checkRefMsgfRet(Error, tag, TEXT("Streaming[%s] bound with invalid Tag[%s] in Display_Tag"), *reference->UID.ToString(), *it.ToString());
			reference->_displayTags.Emplace(tag);
		}
	});
}

void UReferenceBuilder::URefInterviewPostProcessor()
{
	// reserve inspecting accept common event
	URefQuestEvent* acceptEvent = NewObject<URefQuestEvent>(this);
	acceptEvent->UID = "evt.interview.accept";
	acceptEvent->GUID = UCRC32::GetPtr()->Generate32(acceptEvent->UID);
	acceptEvent->Type = "Run_Interview";
	acceptEvent->Type_Value = "None";
	auto references = _references.FindRef(URefQuestEvent::StaticClass());
	references->AddReference(acceptEvent);

	DoRefIterationJob<URefInterview>([this](URefInterview* reference) {
		reference->_dlg = GetRefObj<URefDialog>(reference->Type_Value);
		checkRefMsgfRet(Error, reference->_dlg, TEXT("Interview[%s] bound with invalid Type_Value[%s]; it must be valid Dialog uid"), *reference->UID.ToString(), *reference->Type_Value);

		reference->_acceptDlg = GetRefObj<URefDialog>(reference->Accept_Dialog);
		checkRefMsgfRet(Error, reference->Accept_Dialog.IsNone() || reference->_acceptDlg, TEXT("Interview[%s] bound with invalid Accept_Dialog[%s]"), *reference->UID.ToString(), *reference->Accept_Dialog.ToString());

		reference->_target = GetRefObj<URefCharacter>(reference->_dlg->Owner_Character_UID);
		checkRefMsgfRet(Error, reference->_target, TEXT("Interview[%s] bound with invalid dialog; Owner_Character_UID[%s] in Dialog[%s] must be valid Character uid"), *reference->UID.ToString(), *reference->_dlg->Owner_Character_UID.ToString(), *reference->Type_Value);
	});
}

void UReferenceBuilder::URefInspectingPostProcessor()
{
	// reserve inspecting accept common event
	URefQuestEvent* acceptEvent = NewObject<URefQuestEvent>(this);
	acceptEvent->UID = "evt.inspecting.accept";
	acceptEvent->GUID = UCRC32::GetPtr()->Generate32(acceptEvent->UID);
	acceptEvent->Type = "Run_Inspecting";
	acceptEvent->Type_Value = "None";
	auto references = _references.FindRef(URefQuestEvent::StaticClass());
	references->AddReference(acceptEvent);

	DoRefIterationJob<URefInspecting>([this](URefInspecting* reference) {
		reference->_acceptDlg = GetRefObj<URefDialog>(reference->Accept_Dialog);
		checkRefMsgfRet(Error, reference->Accept_Dialog.IsNone() || reference->_acceptDlg, TEXT("Inspecting[%s] bound with invalid Accept_Dialog[%s]"), *reference->UID.ToString(), *reference->Accept_Dialog.ToString());
		
		URefDialog* targetDlg = GetRefObj<URefDialog>(reference->Type_Value);
		checkRefMsgfRet(Error, targetDlg, TEXT("Inspecting[%s] bound with invalid Type_Value[%s]; not exist Dialog"), *reference->UID.ToString(), *reference->Type_Value);
		targetDlg->Type = URefInspecting::TypeName;
	});
}

void UReferenceBuilder::URefItemReviewPostProcessor()
{
	// reserve review accept common event
	ReserveQuestEvent("evt.review.accept", "Run_ItemReview", "None");

	DoRefIterationJob<URefItemReview>([this](URefItemReview* reference) {
		reference->_dlg = GetRefObj<URefDialog>(reference->Dialog);
		checkRefMsgfRet(Error, reference->_dlg, TEXT("ItemReview[%s] bound with invalid dialog[%s]"), *reference->UID.ToString(), *reference->Dialog);

		reference->_item = GetRefObj<URefItem>(reference->UID);
		checkRefMsgfRet(Error, reference->_item, TEXT("ItemReview[%s] bound with invalid item"), *reference->UID.ToString());
		reference->_item->_review = reference;

		for (auto& it : reference->Display_Tag) {
			if (it.IsNone()) {
				break;
			}

			URefTag* tag = GetRefObj<URefTag>(it);
			checkRefMsgfRet(Error, tag, TEXT("ItemReview[%s] bound with invalid Tag[%s] in Display_Tag"), *reference->UID.ToString(), *it.ToString());
			reference->_displayTags.Emplace(tag);
		}
	});
}

void UReferenceBuilder::URefItemTradePostProcessor()
{
	// reserve review accept common event
	URefQuestEvent* acceptEvent = NewObject<URefQuestEvent>(this);
	acceptEvent->UID = "evt.trade.accept";
	acceptEvent->GUID = UCRC32::GetPtr()->Generate32(acceptEvent->UID);
	acceptEvent->Type = "Run_ItemTrade";
	acceptEvent->Type_Value = "None";
	auto references = _references.FindRef(URefQuestEvent::StaticClass());
	references->AddReference(acceptEvent);

	DoRefIterationJob<URefItemTrade>([this](URefItemTrade* reference) {
		for (auto& dlgUID : reference->Dialog) {
			if (dlgUID.IsNone()) {
				break;
			}
			URefDialog* dlg = GetRefObj<URefDialog>(dlgUID);
			checkRefMsgfRet(Error, dlg, TEXT("ItemTrade[%s] bound with invalid dialog[%s]"), *reference->UID.ToString(), *dlgUID.ToString());
			reference->_dlgs.Emplace(dlg);
		}
		checkRefMsgfRet(Error, reference->_dlgs.Num() != 0, TEXT("ItemTrade[%s] must bound with valid dialog; check Dialog column"), *reference->UID.ToString());

		reference->_item = GetRefObj<URefItem>(reference->UID);
		checkRefMsgfRet(Error, reference->_item, TEXT("ItemTrade[%s] bound with invalid item"), *reference->UID.ToString());

		for (auto& rewardUID : reference->Reward) {
			if (rewardUID.IsNone()) {
				break;
			}
			auto reward = GetRefObj<URefReward>(rewardUID);
			checkRefMsgfRet(Error, reward, TEXT("ItemTrade[%s] bound with invalid reward[%s]"), *reference->UID.ToString(), *rewardUID.ToString());
			checkf(reward->RewardType == ERewardType::Selectable, TEXT("ItemTrade[%s] must be bound with RewardSelectable; check reward[%s]"), *reference->UID.ToString(), *rewardUID.ToString());
			reference->_rewards.Emplace(reward);
		}

		for (auto& it : reference->Display_Tag) {
			if (it.IsNone()) {
				break;
			}

			URefTag* tag = GetRefObj<URefTag>(it);
			checkRefMsgfRet(Error, tag, TEXT("ItemTrade[%s] bound with invalid Tag[%s] in Display_Tag"), *reference->UID.ToString(), *it.ToString());
			reference->_displayTags.Emplace(tag);
		}
	});
}

void UReferenceBuilder::URefStreamingNPCPostProcessor()
{
	DoRefIterationJob<URefStreamingNPC>([this](URefStreamingNPC* strmNpc) {
		strmNpc->_npc = GetRefObj<URefCharacter>(strmNpc->UID);
		checkRefMsgfRet(Error, strmNpc->_npc, TEXT("StreamingNPC[%s] bound with invalid Character"), *strmNpc->UID.ToString());
	});
}

void UReferenceBuilder::URefRankingRewardPostProcessor()
{
	DoRefIterationJob<URefRankingReward>([this](URefRankingReward* reference) {
		reference->_reward = GetRefObj<URefReward>(reference->Reward);
		checkRefMsgfRet(Error, reference->_reward, TEXT("RankingReward[%s] bound with invalid reward[%s]"), *reference->UID.ToString(), *reference->Reward.ToString());
		reference->_reward->_rankingReward = reference;
	});
}

void UReferenceBuilder::URefUserInteractionPostProcessor()
{
	DoRefIterationJob<URefUserInteraction>([this](URefUserInteraction* reference) {
		if (reference->Icon.IsNone() == false) {
			reference->_iconTexture = GetResourceRoute<UTexture2D>(reference->Icon);
			checkRefMsgfRet(Error, !reference->_iconTexture.IsNull(), TEXT("user interaction[%s] bound with invalid icon[%s]"), *reference->UID.ToString(), *reference->Icon.ToString());
		}

		reference->_resetSchedule = GetRefObj<URefSchedule>(reference->Reset_Schedule);
		checkRefMsgfRet(Error, reference->Reset_Schedule.IsNone() || reference->_resetSchedule, TEXT("UserInteraction[%s] bound with invalid Reset_Schedule[%s]"), *reference->UID.ToString(), *reference->Reset_Schedule.ToString());
	});
}

void UReferenceBuilder::URefBodyPostProcessor()
{
	DoRefIterationJob<URefBody>([this](URefBody* reference) {
		LoadResource(reference->Icon, reference->_displayInfo._icon);

		TArray<FAnuTableRow*> output;
		GetResourceTable<UAnuMesh>(reference->Model, output);
		reference->_meshes.AddZeroed(reference->Model.Num());
		for (int32 i = reference->Model.Num() - 1; i >= 0; --i) {
			if (output.IsValidIndex(i)) {
				reference->_meshes[i] = static_cast<FAnuResourceModelMesh*>(output[i]);
			}
		}

		checkRefMsgfRet(Error, reference->_parts != EBodyParts::Frog || reference->_meshes.Num() > 0, TEXT("frog type Body[%s] must be bound with valid DT_ResourceMesh"), *reference->UID.ToString());
		checkRefMsgfRet(Error, reference->_parts != EBodyParts::Npc || reference->_meshes.Num() > 0, TEXT("npc type Body[%s] must be bound with valid DT_ResourceMesh"), *reference->UID.ToString());

		if (reference->CustomDetail_UID.IsNone() == false) {
			reference->_refCustomDefault = GetRefObj<URefCustomDetail>(reference->CustomDetail_UID);
			checkRefMsgfRet(Error, reference->_refCustomDefault, TEXT("do not exist default ref color[%s]"), *reference->CustomDetail_UID.ToString());
		}

		reference->_defaultColor = GetRefObj<URefColor>(reference->Default_Color_UID);
		checkRefMsgfRet(Error, reference->Default_Color_UID.IsNone() || reference->_defaultColor, TEXT("Body[%s] bound invalid Default_Color_UID[%s]; check Color"), *reference->UID.ToString(), *reference->Default_Color_UID.ToString());

		reference->_targetCharacter = GetRefObj<URefCharacter>(reference->Target_Character);
		checkRefMsgfRet(Error, reference->Target_Character.IsNone() || reference->_targetCharacter, TEXT("Body[%s] bound invalid Target_Character[%s]"), *reference->UID.ToString(), *reference->Target_Character.ToString());
	});
}

void UReferenceBuilder::URefSkillPatronagePostProcessor()
{
	TMap<ESkillPatronageTarget, TArray<URefSkillPatronageTier*>> targetTiers;
	DoRefIterationJobNoUID<URefSkillPatronageTier>([&targetTiers](URefSkillPatronageTier* tier) {
		TArray<URefSkillPatronageTier*>& list = targetTiers.FindOrAdd(tier->GetTarget());
		if (list.Num() < tier->Tier) {
			list.SetNum(tier->Tier);
		}

		int32 idx = tier->Tier - 1;
		checkRefMsgf(Error, list[idx] == nullptr, TEXT("Duplicated tier graph detected. Target: %s, Tier: %d"), *tier->Target.ToString(), tier->Tier);
		list[idx] = tier;
	});

	for (auto& pair : targetTiers) {
		auto& target = pair.Key;
		auto& tiers = pair.Value;
		auto it = tiers.Find(nullptr);
		checkRefMsgfRet(Error, it == INDEX_NONE, TEXT("Invalid tier graph detected. Cannot find %d tier. Target: %d"), it + 1, (int32)target);
	}

	DoRefIterationJob<URefSkillPatronage>([this, &targetTiers](URefSkillPatronage* reference) {
		reference->_text = AnuText::Get_CommonTable(reference->Name.ToString());
		reference->_icon = GetResourceRoute<UTexture2D>(reference->Icon);
		checkRefMsgfRet(Error, !reference->_icon.IsNull(), TEXT("SkillPatronage[%s] Icon UID[%s] is not valid."), *reference->UID.ToString(), *reference->Icon.ToString());

		TArray<URefSkillPatronageTier*>* tiers = targetTiers.Find(reference->GetSkillTarget());
		checkRefMsgfRet(Error, tiers && tiers->Num() >= reference->Tier, TEXT("Tier %d is not bound for Skill: %s, Target: %s"), reference->Tier, *reference->UID.ToString(), *reference->Target.ToString());
		reference->_tier = (*tiers)[reference->Tier - 1];

		TArray<FString> skillTimelineUIDs;
		reference->SkillTimelineUID.ToString().ParseIntoArray(skillTimelineUIDs, TEXT("|"));

		reference->_timelines.Reserve(skillTimelineUIDs.Num());
		for (auto& uid : skillTimelineUIDs) {
			uid.TrimStartAndEndInline();
			if (uid.IsEmpty()) {
				continue;
			}
			URefSkillTimeline*& timelineRef = reference->_timelines.Emplace_GetRef(GetRefObj<URefSkillTimeline>(uid));
			checkRefMsgfRet(Error, timelineRef != nullptr, TEXT("SkillPatronage[%s] SkillTimelineUID[%s] is not valid."), *reference->UID.ToString(), *uid);
		}

		const bool bParse = TryParse(reference->Target, reference->_target);
		checkRefMsgfRet(Error, bParse, TEXT("Could not parse patronage[%s] skill group by enum name: %s."), *reference->UID.ToString(), *reference->Target.ToString());
	});
}

void UReferenceBuilder::FAnuResourceWorldListPostProcessor()
{
	FString tableName = "ResourceWorldServerList";
	static FString ResourceTableDir{ "/Game/Anu/DataTable/References/Resource/" };
	FString assetName{ "DT_" + tableName };
	FString assetFullPath{ ResourceTableDir + assetName + "." + assetName };
	UDataTable* dt = LoadObject<UDataTable>(this, *assetFullPath);
	checkRefMsgfRet(Error, dt, TEXT("resource table[%s] not exist in [%s]"), *tableName, *assetFullPath);

	for (FName& uid : dt->GetRowNames()) {
		FAnuResourceWorldServerList* row = dt->FindRow<FAnuResourceWorldServerList>(uid, TEXT(""), false);
		row->GUID = UCRC32::GetPtr()->Generate32(uid);
	}
}

void UReferenceBuilder::URefFashionContentsGroupPostProcessor()
{
	DoRefIterationJob<URefFashionContentsStage>([this](URefFashionContentsStage* reference) {
		for (auto& scoreUID : reference->Add_Score_UID) {
			auto ref = GetRefObj<URefFashionContentsScore>(scoreUID);
			checkRefMsgfRet(Error, ref || scoreUID.IsNone(), TEXT("[fashion] contents stage[%s] Add Score UID[%s] invalid"), *reference->UID.ToString(), *scoreUID.ToString());
			if (ref) {
				reference->_addScores.Emplace(ref);
			}
		}

		for (auto& scoreUID : reference->Minus_Score_UID) {
			if (scoreUID.IsNone()) {
				break;
			}

			auto ref = GetRefObj<URefFashionContentsScore>(scoreUID);
			checkRefMsgfRet(Error, ref, TEXT("[fashion] contents stage[%s] Minus Score UID[%s] invalid"), *reference->UID.ToString(), *scoreUID.ToString());
			reference->_minusScores.Emplace(ref);
		}

		reference->_refGroup = GetRefObj<URefFashionContentsGroup>(reference->Group_UID);
		checkRefMsgfRet(Error, reference->_refGroup, TEXT("Fashion Stage[%s] bound with invalid Group[%s]. check FashionGroup table"), *reference->UID.ToString(), *reference->Group_UID.ToString());
		reference->_refStageInfo = GetRefObj<URefStageInfo>(reference->Stage_UID);
		checkRefMsgfRet(Error, reference->_refStageInfo || reference->Stage_UID.IsNone(), TEXT("Fashion Stage[%s] bound with invalid Stage_UID[%s]. check StageInfo table"), *reference->UID.ToString(), *reference->Stage_UID.ToString());
	});

	DoRefIterationJob<URefFashionContentsNPC>([this](URefFashionContentsNPC* reference) {
		for (auto& scoreUID : reference->Add_Score_UID) {
			auto ref = GetRefObj<URefFashionContentsScore>(scoreUID);
			checkRefMsgfRet(Error, ref, TEXT("[fashion_npc] npcUID[%s] Add Score UID[%s] invalid"), *reference->UID.ToString(), *scoreUID.ToString());
			reference->_addScores.Emplace(ref);
		}

		for (auto& scoreUID : reference->Minus_Score_UID) {
			auto ref = GetRefObj<URefFashionContentsScore>(scoreUID);
			checkRefMsgfRet(Error, ref, TEXT("[fashion_npc] npcUID[%s] Minus Score UID[%s] invalid"), *reference->UID.ToString(), *scoreUID.ToString());
			reference->_minusScores.Emplace(ref);
		}

		reference->_refNPC = GetRefObj<URefNPC>(reference->NPC_UID);
		checkRefMsgfRet(Error, reference->_refNPC, TEXT("[fashion_npc] invalid npc[%s]"), *reference->NPC_UID.ToString());
		if (reference->Condition == "Panel") {
			for (int32 i = 0; i < 2; ++i) {
				TArray<FString> uids;
				URefBase::GetTrimedStringArray(reference->ConditionValue[i].ToString(), uids);
				auto& container = i == 0 ? reference->_likeTag : reference->_hateTag;
				for (auto& scoreUID : uids) {
					URefFashionContentsScore* score = GetRefObj<URefFashionContentsScore>(scoreUID);
					checkRefMsgfRet(Error, score, TEXT("[fashion_npc] npcUID[%s] Minus Score UID[%s] invalid"), *reference->UID.ToString(), *scoreUID);
					container.Emplace(score);
				}
			}
		}
	});

	DoRefIterationJob<URefFashionContentsGroup>([this](URefFashionContentsGroup* reference) {
		for (auto& it : reference->Schedule) {
			URefSchedule* schedule = GetRefObj<URefSchedule>(it);
			checkRefMsgfRet(Error, schedule || it.IsNone(), TEXT("Fashion Group[%s] bound with invalid Schedule[%s]. check Common/Schedule table"), *reference->UID.ToString(), *it.ToString());

			if (schedule) {
				reference->_refSchedule.Emplace(schedule);
			}
		}
		
		for (auto& it : reference->Donations) {
			if (reference->_specialDonation == nullptr || reference->_specialDonation->Score < it->Score) {
				reference->_specialDonation = it;
			}
		}
	});

	DoRefIterationJob<URefFashionContentsScore>([this](URefFashionContentsScore* reference) {
		if (reference->TargetUID.IsNone()) {
			return;
		}

		TArray<FString> values;
		URefBase::GetTrimedStringArray(reference->TargetUID.ToString(), values);
		if (values.Num() == 0) {
			return;
		}

		reference->_tag = GetRefObj<URefTag>(values[1]);
		checkRefMsgfRet(Error, values[0] != "Tag" || reference->_tag, "[fashion] FashionContentsScore[%s] bound with invalid Tag[%s]", *reference->UID.ToString(), *reference->TargetUID.ToString());
		reference->_tagGroup = GetRefObj<URefTagGroup>(values[1]);
		checkRefMsgfRet(Error, values[0] != "TagGroup" || reference->_tagGroup, "[fashion] FashionContentsScore[%s] bound with invalid TagGroup[%s]", *reference->UID.ToString(), *reference->TargetUID.ToString());
	});
}

void UReferenceBuilder::URefFashionContentsStagePostProcessor()
{
	DoRefIterationJob<URefFashionContentsStage>([this](URefFashionContentsStage* reference) {
		if (reference->ClassExp[0] == "None") {
			return;
		}

		checkRefMsgfRet(Error, reference->ClassExp.Num() == 3, TEXT("Fashion Stage[%s] invalid ClassExp, array size not equals 3"), *reference->UID.ToString());
		reference->rewardClass = GetRefObj<URefClass>(reference->ClassExp[0]);
		checkRefMsgfRet(Error, reference->rewardClass, TEXT("Fashion Stage[%s] invalid ClassExp[%s]. bound with invalid Class_UID[%s]"), *reference->UID.ToString(), *reference->ClassExp[0]);
		reference->rewardClassExp = FCString::Atoi(*reference->ClassExp[2]);
	});
}

void UReferenceBuilder::URefTagGroupPostProcessor()
{
	DoRefIterationJob<URefTag>([this](URefTag* tag) {
		URefTagGroup* tagGroup = GetRefObj<URefTagGroup>(tag->Group_UID);
		checkRefMsgfRet(Error, tagGroup, TEXT("[tag] Tag[%s] bound with invalid TagGroup[%s]"), *tag->UID.ToString(), *tag->Group_UID.ToString());
		tag->_group = tagGroup;
		tagGroup->_tags.Emplace(tag);

		tag->_schedule = ParseDynamicSchedule(tag->Schedule, "Tag", true);
		checkRefMsgfRet(Error, tag->Schedule.Compare("None") == 0 || tag->_schedule, TEXT("[tag] Tag[%s] bound with invalid Schedule[%s]"), *tag->UID.ToString(), *tag->Schedule);
	});
}

void UReferenceBuilder::URefPaletteGroupPostProcessor()
{
	DoRefIterationJobNoUID<URefPalette>([this](URefPalette* palette) {
		palette->_color = GetRefObj<URefColor>(palette->Color_UID);
		checkRefMsgfRet(Error, palette->_color, TEXT("Palette[%s] bound with invalid Color"), *palette->GetDebugString());

		palette->_group = GetRefObj<URefPaletteGroup>(palette->Group_UID);
		checkRefMsgfRet(Error, palette->_group, TEXT("Palette[%s] bound with invalid PaletteGroup"), *palette->GetDebugString());
		checkRefMsgfRet(Error, palette->_group->_colors.Find(palette->_color) == nullptr, TEXT("PaletteGroup[%s] has duplicate Color[%s]"), *palette->Group_UID.ToString(), *palette->Color_UID.ToString());
		palette->_group->_colors.Emplace(palette->_color, palette);
	});

	DoRefIterationJob<URefPaletteGroup>([this](URefPaletteGroup* paletteGroup) {
		paletteGroup->_schedule = GetRefObj<URefSchedule>(paletteGroup->Schedule);
		checkRefMsgfRet(Error, paletteGroup->Schedule.IsNone() || paletteGroup->_schedule, TEXT("PaletteGroup[%s] bound with invalid Schedule[%s]"), *paletteGroup->UID.ToString(), *paletteGroup->Schedule.ToString());

		if (paletteGroup->Icon.IsNone() == false) {
			paletteGroup->_displayInfo._icon.Emplace(GetResourceRoute<UTexture2D>(paletteGroup->Icon));
		}

		CostPostProcessor(paletteGroup->_dyeingCost);
		CostPostProcessor(paletteGroup->_unlockCost);
	});

	TMap<FName, TArray<URefColorPigment*>> pigments;
	DoRefIterationJobNoUID<URefColorPigment>([this, &pigments](URefColorPigment* pigment) {
		CostPostProcessor(pigment->_impl);

		auto& list = pigments.FindOrAdd(pigment->Pigment_UID);
		list.Emplace(pigment);
	});

	DoRefIterationJob<URefColor>([this, &pigments](URefColor* color) {
		auto& list = pigments.FindOrAdd(color->ColorPigment_UID);
		color->_pigmentRecipes.Empty();
		color->_pigmentRecipes.Append(list);

		auto& replacable = _shopCosts.FindOrAdd(color->ColorPigment_Replacable);
		color->_pigmentRecipeReplacable.Empty();
		color->_pigmentRecipeReplacable.Append(replacable);
	});
}

void UReferenceBuilder::URefSchedulePostProcessor()
{
}

void UReferenceBuilder::URefEquipCollectionGroupPostProcessor()
{
	URefEquipCollection::ByEquipGuids.Empty();
	DoRefIterationJob<URefEquipCollection>([this](URefEquipCollection* reference) {
		reference->_equipItem = GetRefObj<URefItemEquip>(reference->Item_UID);
		checkRefMsgfRet(Error, reference->_equipItem, TEXT("Collection[%s] bound with invalid EquipItem[%s]"), *reference->UID.ToString(), *reference->Item_UID.ToString());

		auto& collections = URefEquipCollection::ByEquipGuids.FindOrAdd(reference->_equipItem->GUID);
		collections.Emplace(reference);

		reference->_group = GetRefObj<URefEquipCollectionGroup>(reference->Group_UID);
		checkRefMsgfRet(Error, reference->_group, TEXT("Collection[%s] bound with invalid Collection Group[%s]"), *reference->UID.ToString(), *reference->Group_UID.ToString());
		reference->_group->_collections.Emplace(reference);
	});
}

void UReferenceBuilder::URefEmblemEffectPostProcessor()
{
	FString globalStr;	
	bool res = GetRefGlobal("emblem.max.upgrade.value", URefItemEmblem::MaxUpgradeValue_NonTranscended);
	checkRefMsgfRet(Error, res, TEXT("[emblem] cannot find Gloabl[emblem.max.upgrade.value]"));
	URefItemEmblem::MaxUpgradeValue = URefItemEmblem::MaxUpgradeValue_NonTranscended;

	res = GetRefGlobal("emblem.max.transcend.value", URefItemEmblem::MaxTranscendValue);
	checkRefMsgfRet(Error, res, TEXT("[emblem] cannot find Gloabl[emblem.max.transcend.value]"));

	res = GetRefGlobal("emblem.max.upgrade.transendence.value", globalStr);
	checkRefMsgfRet(Error, res, TEXT("[emblem] cannot find Gloabl[emblem.max.upgrade.transendence.value]"));
	URefItemEmblem::TranscendedUpgradeValues.Empty();
	URefBase::GetIntegerArray(globalStr, URefItemEmblem::TranscendedUpgradeValues);
	if (URefItemEmblem::TranscendedUpgradeValues.Num() != 0) {
		URefItemEmblem::MaxUpgradeValue = URefItemEmblem::TranscendedUpgradeValues.Last();
	}

	res = GetRefGlobal("emblem.transcendable.grade", URefItemEmblem::TranscendableGrade);
	checkRefMsgfRet(Error, res, TEXT("[emblem] cannot find Gloabl[emblem.transcendable.grade]"));

	DoRefIterationJobNoUID<URefEmblemEffect>([this](URefEmblemEffect* emblemEffect) {
		URefItemEmblem* emblem = GetRefObj<URefItemEmblem>(emblemEffect->Emblem_UID);
		checkRefMsgfRet(Error, emblem, TEXT("Emblem_Effect[%s] bound with invalid Emblem_UID"), *emblemEffect->GetDebugString());

		if (emblem->_upgradeEffects.Num() == 0) {
			emblem->_upgradeEffects.Init(nullptr, URefItemEmblem::MaxUpgradeValue + 1); // +1: for zero index(non-upgrade)
		}
		checkRefMsgfRet(Error, emblem->_upgradeEffects.IsValidIndex(emblemEffect->Upgrade_Value), TEXT("Emblem_Effect[%s] bound with invalid Upgrade_Value"), *emblemEffect->GetDebugString());
		emblem->_upgradeEffects[emblemEffect->Upgrade_Value] = emblemEffect;

		TagPostProcessor(emblemEffect->GetDebugString(), emblemEffect->_tagWithValues, emblemEffect->_tags);
	});
}

void UReferenceBuilder::URefStatPostProcessor()
{
	DoRefIterationJob<URefStat>([this](URefStat* reference) {
		if (reference->SpecificTargets.Num() == 0) {
			return;
		}

		// skill attribute bonus
		static const TSet<FName> SkillStatType {
			"SkillPower",
			"SkillPower_Boss",
			"SkillRange",
			"SkillDuration"
		};

		static const TSet<FName> SkillTreeLevelUpType {
			"SkillTreeLevelUp"
		};

		if (SkillStatType.Contains(reference->Group[0])) {
			for (auto& uid : reference->SpecificTargets) {
				auto target = GetRefObj<URefSkillTimeline>(uid);
				checkRefMsgfRet(Error, target, TEXT("Stat:[%s] bound with invalid skill:[%s]"), *reference->UID.ToString(), *uid.ToString());
				reference->_targetSkills.Emplace(target);
				target->_refSkill->StatEffects.Emplace(reference);

				if (reference->Group.Contains("SkillPower")) {
					target->_extraPower.Emplace(reference);
				}
				else if (reference->Group.Contains("SkillPower_Boss")) {
					target->_extraBossDamage.Emplace(reference);
				}
				else if (reference->Group.Contains("SkillRange")) {
					target->_extraRange.Emplace(reference);

					if (target->_refEffect->Effect == SkillEffect::MakeObject) {
						target->_refSkill->_extraRange.Emplace(reference);
					}
				}
				else if (reference->Group.Contains("SkillDuration")) {
					target->_extraDuration.Emplace(reference);

					if (target->_refEffect->Effect == SkillEffect::MakeObject) {
						auto targetEffect = Cast<URefSkillEffectMakeObject>(target->_value);
						auto targetObject = targetEffect->refSkillObject;
						targetObject->_extraDuration.Emplace(reference);
					}
				}
			}
			return;
		}
		if (SkillTreeLevelUpType.Contains(reference->Group[0])) {
			for (auto& uid : reference->SpecificTargets) {
				auto target = GetRefObj<URefSkillTreeSlot>(uid);
				checkRefMsgfRet(Error, target, TEXT("Stat:[%s] bound with invalid skill:[%s]"), *reference->UID.ToString(), *uid.ToString());
				reference->_targetTreeSlots.Emplace(target);
				target->_extraLevels.Emplace(reference);
			}
		}

		// class exp & mastery bonus
		static constexpr int32 STAT_CLASS_EXP		= 1;
		static constexpr int32 STAT_CLASS_MASTERY	= 2;
		static auto AddClassTarget = [](URefClass* target, int32 mask, URefStat* stat) {
			if ((mask & STAT_CLASS_EXP) != 0) {
				target->_extraExpStats.Emplace(stat);
			}
			if ((mask & STAT_CLASS_MASTERY) != 0) {
				target->_extraMasteryStats.Emplace(stat);
			}
		};

		uint8 mask =
			reference->Group.Contains("ClassExp") * STAT_CLASS_EXP |
			reference->Group.Contains("ClassMastery") * STAT_CLASS_MASTERY;

		if (mask > 0) {
			if (reference->SpecificTargets.Num() == 1 && reference->SpecificTargets[0].IsNone()) {
				DoRefIterationJob<URefClass>([mask, reference](URefClass* target) {
					AddClassTarget(target, mask, reference);
				});
			}
			else {
				for (auto& uid : reference->SpecificTargets) {
					auto target = GetRefObj<URefClass>(uid);
					checkRefMsgfRet(Error, target, TEXT("Stat:[%s] bound with invalid class:[%s]"), *reference->UID.ToString(), *uid.ToString());
					AddClassTarget(target, mask, reference);
				}
			}
			return;
		}
	});
}

void UReferenceBuilder::URefCharacterStatPostProcessor()
{
	TMap<FName, URefCharacterStat*> stats;

	DoRefIterationJobNoUID<URefCharacterStat>([this, &stats](URefCharacterStat* reference) {
		if (reference->PrimaryKey.IsNone() == false) {
			stats.Emplace(reference->PrimaryKey, reference);
		}
	});

	DoRefIterationJob<URefCharacter>([this, &stats](URefCharacter* reference) {
		if (reference->StatKey == NAME_None) {
			return;
		}

		reference->BaseStat = stats.FindRef(reference->StatKey);
		checkRefMsgf(Error, reference->BaseStat != nullptr, TEXT("Character[%s] bound with invalid stat[%s]"), *reference->UID.ToString(), *reference->StatKey.ToString());
	});
}

void UReferenceBuilder::URefArbeitRewardPostProcessor()
{
	DoRefIterationJob<URefArbeitReward>([this](URefArbeitReward* arbeitRwd) {
		for (auto& uid : arbeitRwd->Reward) {
			if (uid.IsNone()) {
				break;
			}
			URefReward* rwd = GetRefObj<URefReward>(uid);
			checkRefMsgfCont(Error, rwd, TEXT("[arbeit] ArbeitReward[%s] bound with invalid Reward[%s]"), *arbeitRwd->UID.ToString(), *uid.ToString());
			arbeitRwd->_rewards.Emplace(rwd);
		}

		for (auto& it : arbeitRwd->ScheduleRewards) {
			if (it.Compare("None") == 0) {
				break;
			}

			TArray<FString> strValues;
			URefBase::GetTrimedStringArray(it, strValues);
			checkRefMsgfCont(Error, strValues.Num() == 2, TEXT("[arbeit] ArbeitReward[%s] bound with invalid Schedule_Reward[%s]; format must be>> {schedule.uid} | {reward.uid}"), *arbeitRwd->UID.ToString(), *it);

			const FString& scheduleUID = strValues[0];
			URefSchedule* schedule = GetRefObj<URefSchedule>(scheduleUID);
			checkRefMsgfCont(Error, schedule, TEXT("[arbeit] ArbeitReward[%s] bound with invalid Schedule[%s]"), *arbeitRwd->UID.ToString(), *scheduleUID);

			const FString& rewardUID = strValues[1];
			URefReward* rwd = GetRefObj<URefReward>(rewardUID);
			checkRefMsgfCont(Error, rwd, TEXT("[arbeit] ArbeitReward[%s] bound with invalid Reward[%s]"), *arbeitRwd->UID.ToString(), *rewardUID);

			arbeitRwd->_scheduledRewards.Emplace(schedule, rwd);
		}
	});

	DoRefIterationJob<URefQuestArbeit>([this](URefQuestArbeit* arbeit) {
		arbeit->_arbeitReward = GetRefObj<URefArbeitReward>(arbeit->ArbeitReward_UID);
		checkRefMsgfRet(Error, arbeit->_arbeitReward, TEXT("[arbeit] Quest_Arbeit[%s] bound with invalid ArbeitReward[%s]"), *arbeit->UID.ToString(), *arbeit->ArbeitReward_UID.ToString());

		if (arbeit->_arbeitReward->AutoReward) {
			arbeit->_rewards.Append(arbeit->_arbeitReward->_rewards);
		}
		else {
			arbeit->_manualRewards.Append(arbeit->_arbeitReward->_rewards);
		}
	});
}

void UReferenceBuilder::URefRankingPostProcessor()
{
	DoRefIterationJob<URefRanking>([this](URefRanking* ranking) {
		ranking->_resetSchedule = GetRefObj<URefSchedule>(ranking->ResetSchedule);
		if (ranking->ResetSchedule != NAME_None) {
			checkRefMsgfRet(Error, ranking->_resetSchedule, TEXT("[ranking] RefRanking[%s] bound with invalid ResetSchedule[%s]"), *ranking->UID.ToString(), *ranking->ResetSchedule.ToString());
		}

		DoRefIterationJob<URefRankingReward>([this, ranking](URefRankingReward* reward) {
			if (ranking->RankingRewardName != NAME_None) {
				if (ranking->RankingRewardName == reward->Ranking_Name) {
					ranking->_rewards.Emplace(reward);
				}

				if (ranking->TopRankerRewardName.Contains(reward->Ranking_Name)) {
					if (reward->_rangeValues[0] == reward->_rangeValues[1]) {
						ranking->_topRankerEachRewards.Emplace(reward->_rangeValues[0], reward);
					}
					else {
						ranking->_topRankerCommonRewards.Emplace(reward);
					}
				}
			}
		});

		ranking->_rewards.Sort([](const URefRankingReward& lhs, const URefRankingReward& rhs) {
			return lhs._rangeValues[0] < rhs._rangeValues[0];
		});
	});
}

void UReferenceBuilder::URefSkillTreePostProcessor()
{
	DoRefIterationJob<URefSkillTree>([this](URefSkillTree* reference) {
		reference->_class = GetRefObj<URefClass>(reference->Class_UID);
		checkRefMsgfRet(Error, reference->_class, TEXT("[skilltree] tree[%s] bound with invalid class[%s]"), *reference->UID.ToString(), *reference->Class_UID.ToString());
	});
}

void UReferenceBuilder::URefSkillTreeStepPostProcessor()
{
	DoRefIterationJobNoUID<URefSkillTreeStep>([this](URefSkillTreeStep* reference) {
		auto skilltree = GetRefObj<URefSkillTree>(reference->SkillTree_UID);
		checkRefMsgfRet(Error, skilltree, TEXT("[skilltree] step bound with invalid tree[%s]"), *reference->SkillTree_UID.ToString());
		skilltree->_steps.Emplace(reference);
		reference->_tree = skilltree;
	});

	DoRefIterationJob<URefSkillTree>([this](URefSkillTree* reference) {
		reference->_steps.Sort([](const URefSkillTreeStep & a, const URefSkillTreeStep & b) {
			return a.Step < b.Step;
		});

		for (int32 i = 0; i < reference->_steps.Num(); ++i) {
			int32 prevIndex = i - 1;
			if (prevIndex >= 0) {
				reference->_steps[i]->_prev = reference->_steps[prevIndex];
			}

			int32 nextIndex = i + 1;
			if (nextIndex < reference->_steps.Num()) {
				reference->_steps[i]->_next = reference->_steps[nextIndex];
			}
		}
	});
}

void UReferenceBuilder::URefSkillTreeSlotPostProcessor()
{
	using ClassName = FName;
	using StepNumber = int32;
	TMap<ClassName, TArray<URefSkillTreeSlot*>> builded;

	DoRefIterationJob<URefSkillTreeSlot>([this, &builded](URefSkillTreeSlot* reference) {

		// Build Reward Skill
		if (reference->Reward_Skill.Num() >= 2) {
			FString srcName = reference->Reward_Skill[0];
			FString dstName = reference->Reward_Skill[1];
			reference->_rewardSkill.Key = GetRefObj<URefSkill>(srcName);
			reference->_rewardSkill.Value = GetRefObj<URefSkill>(dstName);
			checkRefMsgfRet(Error, reference->_rewardSkill.Key && reference->_rewardSkill.Value, TEXT("[skilltree] level[%s] bound with invalid reward skill[%s->%s]"), *reference->UID.ToString(), *srcName, *dstName);
			reference->_rewardSkill.Value->SkillTreeSlot = reference;
		}

		// Build Icon
		reference->_iconTexture = GetResourceRoute<UTexture2D>(reference->Icon);
		//checkRefMsgfRet(Error, reference->_iconTexture.IsNull() == false, TEXT("[skilltree] element[%s] bound with invalid icon[%s]"), *reference->UID.ToString(), *reference->Icon.ToString());

		auto& entry = builded.FindOrAdd(reference->SkillTree_UID);
		entry.Emplace(reference);
	});

	// Bind
	using SkillTreeName = FName;
	TMap<SkillTreeName, TMap<int32, URefSkillTreeStep*>> steps;
	DoRefIterationJobNoUID<URefSkillTreeStep>([this, &steps](URefSkillTreeStep* reference) {
		auto& entry = steps.FindOrAdd(reference->SkillTree_UID);
		entry.Emplace(reference->Step, reference);
	});

	for (auto& pair : builded) {
		auto& entry = steps[pair.Key];
		for (auto& one : pair.Value) {
			one->_step = entry[one->Step];
			entry[one->Step]->_slots.Emplace(one);
		}
	}
}

void UReferenceBuilder::URefSkillTreeLevelPostProcessor()
{
	using SlotName = FName;
	TMap<SlotName, TArray<URefSkillTreeLevel*>> builded;

	DoRefIterationJob<URefSkillTreeLevel>([&](URefSkillTreeLevel* reference) {

		// Build Reward
		if (reference->Reward_Stat.Num() >= 2) {
			for (int32 i = 0; i < reference->Reward_Stat.Num(); i += 2) {
				FStatInfoData stat;
				stat.StatString = FName(reference->Reward_Stat[i]);
				stat.StatType = URefObject::GetStatEnum(stat.StatString);
				stat.Value = FCString::Atoi(*reference->Reward_Stat[i + 1]);
				stat.ApplyType = EStatApplyType::Add;
				reference->_rewardStats.Emplace(stat);
			}
		}

		// Build Condition
		reference->_conditionLicenseClass = GetRefObj<URefClass>(reference->Condition_License[0]);
		checkRefMsgfRet(Error, reference->_conditionLicenseClass, TEXT("[skilltree] level[%s] bound with invalid license class[%s]"), *reference->UID.ToString(), *reference->Condition_License[0]);
		reference->_conditionLicenseType = URefClass::GetLicenceType(reference->Condition_License[1]);

		// Build Cost
		if (reference->Cost_UID != NAME_None) {
			FDynamicCost cost;
			cost._typeName = "Item";
			cost._type = URefItem::StaticClass();
			cost.UID = reference->Cost_UID;
			cost._target = GetRefObj<URefItem>(cost.UID);
			cost.SetOriginAmount(reference->Cost_Value);
			checkRefMsgfRet(Error, cost._target, TEXT("[skilltree] level[%s] invliad cost item[%s]"), *reference->UID.ToString(), *reference->Cost_UID.ToString());
			reference->_cost = cost;
		}

		auto& entry = builded.FindOrAdd(reference->SkillTreeSlot_UID);
		entry.Emplace(reference);
	});

	// Bind
	for (auto& pair : builded) {
		pair.Value.Sort([](const URefSkillTreeLevel& a, const URefSkillTreeLevel& b) {
			return a.Level < b.Level;
		});

		auto reference = GetRefObj<URefSkillTreeSlot>(pair.Key);
		checkRefMsgfRet(Error, reference, TEXT("[skilltree] level[%s] bound with invalid element[%s]"), *reference->UID.ToString(), *pair.Key.ToString());
		reference->_levels = pair.Value;
	}
}