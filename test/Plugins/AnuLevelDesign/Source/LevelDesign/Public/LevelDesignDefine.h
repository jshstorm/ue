// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include <array>
#include <functional>
#include <any>

class CLIENTNET_API FNetPacket;

#include "LevelDesignDefine.generated.h"

#define DEBUG_LD 1

DEFINE_LOG_CATEGORY_STATIC(LogLD, Verbose, All);

#define GET_ENUM_PTR(EnumType) \
	FindObject<UEnum>(ANY_PACKAGE, TEXT(#EnumType))

UENUM(BlueprintType)
enum class ELDGameResult : uint8
{
	None,
	Success,
	Failure,
};

UENUM(BlueprintType)
enum class ELDSkillTarget : uint8
{
	None,
	Players,
	Monsters,
	Group
};

UENUM(BlueprintType)
enum class ELDQuestEvent : uint8
{
	None,
	Collision,
	LevelDesign,
};

UENUM(BlueprintType)
enum class ELDNodeState : uint8
{
	None,
	Activate,
	Running,
	Failure,
	Deactivate
};

UENUM(BlueprintType)
enum class ELDSpawnEvent : uint8
{
	None,
	ChestOfRuins,
	Blacklist
};

UENUM(BlueprintType)
enum class ELDPlayerStance : uint8
{
	Normal,
	Battle
};

//
// Blackboard
//
DECLARE_DELEGATE_OneParam(FRegisterEventListener, UObject*);
DECLARE_DELEGATE_OneParam(FProxyUpdateDelegate, FNetPacket*);

struct ANULEVELDESIGN_API FLDBlackboard
{
	FRegisterEventListener Register;
	FRegisterEventListener Unregister;

	UPROPERTY()
	class AAmbientSound* BGMPlayer;

	int32 CheckPointStep = 0;
};

//
// Structure
//
USTRUCT(BlueprintType)
struct FBombInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ObjectUID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BonusScore;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DamageScore;
};

//
// Event
//
#define DEFINE_STAGE_EVENT(evt) static const FName evt = TEXT(#evt)

DEFINE_STAGE_EVENT(STAGE_STATE_START);

DEFINE_STAGE_EVENT(STAGE_OBJECT_HP_UPDATED);
DEFINE_STAGE_EVENT(STAGE_OBJECT_LIFE_STATE_UPDATED);

struct ANULEVELDESIGN_API FStageEventHandler
{
	TFunction<void(void* pack)> callback;

public:
	template<typename T, typename... TArgs>
	FStageEventHandler(T* listener, void(T::* handler)(TArgs...))
	{
		callback = [this, listener, handler](void* pack) {
			Invoke(listener, handler, pack, std::make_index_sequence<sizeof...(TArgs)>{ });
		};
	}

	template<typename... TArgs>
	void Execute(TArgs... args)
	{
		auto pack = std::make_tuple(args...);
		callback(&pack);
	}

	template<typename T, typename... TArgs, size_t... sequence>
	void Invoke(T* listener, void(T::* handler)(TArgs...), void* pack, std::index_sequence<sequence...> seq)
	{
		auto& tuple = *(std::tuple<TArgs...>*)pack;
		(listener->*handler)(std::get<sequence>(tuple)...);
	}
};

UCLASS()
class ANULEVELDESIGN_API UStageEventListener : public UObject
{
	GENERATED_BODY()

protected:
	TMap<FName, FStageEventHandler> _handlers;

public:
	template <typename...TArgs>
	void OnStageEvent(FName evt, TArgs&&...args)
	{
		auto it = _handlers.Find(evt);
		if (it != nullptr) {
			it->Execute(Forward<TArgs>(args)...);
		}
	}

protected:
#define REGISTER_STAGE_EVENT_HANDLER(className, evt) \
		RegisterStageEvent(evt, FStageEventHandler(this, &className::On##evt))

	void RegisterStageEvent(FName evt, FStageEventHandler handler)
	{
		_handlers.Emplace(evt, handler);
	}
};