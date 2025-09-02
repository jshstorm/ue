#pragma once

#include <vector>
#include <map>
#include <string>
#include <cstddef>
#include <bitset>
#include <array>
#include <cassert>

#ifdef check
#define check1 check
#else
#define check1 assert
#endif

#define MAKE_ENUM_CLASS_MASK(E)											\
constexpr E operator &(const E& l, const E& r) noexcept					\
{																		\
	using T = std::underlying_type_t<E>;								\
	return static_cast<E>(static_cast<T>(l) & static_cast<T>(r));		\
}																		\
																		\
constexpr E operator |(const E& l, const E& r) noexcept					\
{																		\
	using T = std::underlying_type_t<E>;								\
	return static_cast<E>(static_cast<T>(l) | static_cast<T>(r));		\
}																		\
																		\
constexpr E operator ^(const E& l, const E& r) noexcept					\
{																		\
	using T = std::underlying_type_t<E>;								\
	return static_cast<E>(static_cast<T>(l) ^ static_cast<T>(r));		\
}																		\
																		\
constexpr E& operator &=(E& l, const E& r) noexcept						\
{																		\
	return l = l & r;													\
}																		\
																		\
constexpr E& operator |=(E& l, const E& r) noexcept						\
{																		\
	return l = l | r;													\
}																		\
																		\
constexpr E& operator ^=(E& l, const E& r) noexcept						\
{																		\
	return l = l ^ r;													\
}																		\
																		\
constexpr E operator ~(const E& l) noexcept								\
{																		\
	using T = std::underlying_type_t<E>;								\
	return static_cast<E>(~static_cast<T>(l));							\
}

static const int32 PROTOCOL_HASH = 0;

enum class PlatformType : uint8
{
	INVALID = 0,
	GUEST = 10,
	FACEBOOK = 11,
	GOOGLE_PLAY = 12,
	APPLE_GAME_CENTER = 13,
	APPLE_ID = 14,
	LINE = 15,
	TWIITER = 16,
	NHN_TOAST_GAMEBASE = 20,
	UUID = 99, // db 상에 남기기 위한 추가 코드 기기 식별용 코드
	CLOVER = 127, // 내부 테스트용 분리 하기 위해서 사용되는 code. (platform 쪽에 없음)
};

inline PlatformType StrToPlatformType(const char* str) 
{
	static std::map<std::string_view, PlatformType> str2Platforms =
	{
		{"guest", PlatformType::GUEST },
		{"facebook", PlatformType::FACEBOOK },
		{"google", PlatformType::GOOGLE_PLAY },
		{"appleid", PlatformType::APPLE_ID },
		{"twitter", PlatformType::TWIITER },
	};

	auto it = str2Platforms.find(std::string_view(str));
	return it == str2Platforms.end() ? PlatformType::INVALID : it->second;
}

enum class PlatformLangCode : uint8
{
	en = 0,
	ko,
	fr,
	it,
	de,
	es,
	pt,
	all = 0xFF,
};

inline constexpr PlatformLangCode DefaultLangCode = PlatformLangCode::en;

inline PlatformLangCode StrToLangCode(const char* str)
{
	static std::map<std::string_view, PlatformLangCode> str2lang =
	{
		{ "ko-KR", PlatformLangCode::ko },
		{ "en", PlatformLangCode::en },
		{ "ko", PlatformLangCode::ko },
		{ "fr", PlatformLangCode::fr },
		{ "it", PlatformLangCode::it },
		{ "de", PlatformLangCode::de },
		{ "es", PlatformLangCode::es },
		{ "pt", PlatformLangCode::pt },
	};

	auto it = str2lang.find(std::string_view(str));
	check1(it != str2lang.end());
	return it == str2lang.end() ? (PlatformLangCode)0 : it->second;
}

inline const char* LangCodeToStr(PlatformLangCode lang)
{
	static std::map<PlatformLangCode, std::string_view> str2lang =
	{
		{ PlatformLangCode::ko, "ko-KR" },
		{ PlatformLangCode::en, "en" },
		{ PlatformLangCode::ko, "ko" },
		{ PlatformLangCode::fr, "fr" },
		{ PlatformLangCode::it, "it" },
		{ PlatformLangCode::de, "de" },
		{ PlatformLangCode::es, "es" },
		{ PlatformLangCode::pt, "pt" },
	};

	auto it = str2lang.find(lang);
	check1(it != str2lang.end());
	return it == str2lang.end() ? str2lang[(PlatformLangCode)0].data() : it->second.data();
}

enum class AccountBanState : uint8
{
	NONE = 0,
	LOGIN = 1,
	CHAT = 2,
};

enum class AccountType : uint8
{
	INVALID = 0,
	GUEST = 1,
	NORMAL = 3,
	QA = 5,
	ADMINISTRATOR = 7,
	CLOVERS = 8,
	BOT = 9,
};

enum class MigrationType : uint8
{
	PlayInWorld,
	Spectating
};

#define SESSION_TOKEN_IDENTITY_LENGTH (size_t)10
static std::string GetShortenToken(const std::string& token)
{
	size_t given = token.size();
	if (given <= SESSION_TOKEN_IDENTITY_LENGTH) {
		static std::string empty;
		return empty;
	}
	return token.substr(given - SESSION_TOKEN_IDENTITY_LENGTH, SESSION_TOKEN_IDENTITY_LENGTH);
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////
#define DATA_FOR_MY_CLIENT			(uint8)0x01
#define DATA_FOR_OTHERS				(uint8)0x02

///////////////////////////////////////////////////////
// LifeState
///////////////////////////////////////////////////////
enum class LifeState : uint8
{
	LIFESTATE_ALIVE = 0x01,
	LIFESTATE_DEAD = 0x02,
	LIFESTATE_GONE = 0x03,
	LIFESTATE_TERMINATED = 0x04,
	LIFESTATE_NUM
};

///////////////////////////////////////////////////////
// BodyMode
///////////////////////////////////////////////////////
#define BODYMODE_MASK_NORMAL			(uint8)0x00
#define BODYMODE_MASK_INVINCIBLE		(uint8)0x01 // 타겟은 되지만 데미지는 들어오지 않는 상태(넉다운 등은 받음)
#define BODYMODE_MASK_UNTOUCHABLE		(uint8)0x02 // 타겟 조차가 되지 않는 상태, 모든 효과 무시
#define BODYMODE_MASK_INVISIBLE			(uint8)0x04 // 직접 타겟에 포함되진 않지만, 광범위등으로 인해 선택 될 수 있는 상태
#define BODYMODE_MASK_TRANSFORM         (uint8)0x08
#define BODYMODE_MASK_SUPERARMOR		(uint8)0x10 // 경직되지 않음 (공격이 캔슬되지 않음, 이동 중 맞아도 멈추지 않음 등...)
#define BODYMODE_MASK_IMMORTAL			(uint8)0x20 // HP등은 소모되나 사망에 이르지 않음.

namespace BodyMode {
	static uint8 StringTo(const char* type) {
		static std::map<std::string, uint8> TYPES{
			{ "Transform", BODYMODE_MASK_TRANSFORM },
		};

		auto itr = TYPES.find(type);
		return itr->second;
	}
}

///////////////////////////////////////////////////////
// Team
///////////////////////////////////////////////////////
enum class TeamTag : uint8
{
	Neutral,
	Monster,
	Team1, Team2, Team3, Team4, Team5, Team6,
	Observer,
	DummyPC,
};

///////////////////////////////////////////////////////
// Collision
///////////////////////////////////////////////////////
enum class CollisionChannel : uint8
{
	None			= (uint8)0x00,
	PC				= (uint8)0x01,
	Monster			= (uint8)0x02,
	Boss			= (uint8)0x04,
	SkillObject		= (uint8)0x08,
	Gimmick			= (uint8)0x10
};

inline constexpr CollisionChannel operator&(CollisionChannel a, CollisionChannel b)
{
	return static_cast<CollisionChannel>
		(static_cast<uint8>(a) & static_cast<uint8>(b));
}

inline constexpr CollisionChannel operator|(CollisionChannel a, CollisionChannel b)
{
	return static_cast<CollisionChannel>
		(static_cast<uint8>(a) | static_cast<uint8>(b));
}

inline CollisionChannel& operator|=(CollisionChannel& a, CollisionChannel b)
{
	return (CollisionChannel&)((uint8&)a |= (uint8)b);
}

enum class CollisionResponse
{
	None = 0,
	Overlap,
	Block
};

///////////////////////////////////////////////////////
// Debug
///////////////////////////////////////////////////////
enum class DebugMode : uint8
{
	None			= (uint8)0x00,
	Location		= (uint8)0x01,
	Voxel			= (uint8)0x02
};

///////////////////////////////////////////////////////
// ControllState -> motionState
// !!check 'reference_enum_class.h'
///////////////////////////////////////////////////////
enum class MotionState : uint8 {
	None,
	Normal,
	SkillDash,
	SkillImpluse,
	Falling,
	Swimming
};

///////////////////////////////////////////////////////
// ControllState -> Stance
///////////////////////////////////////////////////////
enum class Stance : uint8
{
	Normal,
	Battle,
	Carry
};

#define SPEED_RATE_BY_STANCE(stance, rate) rate == 0.0f ? 0.0f : stance != Stance::Normal ? 1.0f : rate > 0.5f ? 1.0f : 0.5f

///////////////////////////////////////////////////////
// AnimationState
///////////////////////////////////////////////////////
enum class TurnAnimType : uint8 
{
	Left45,
	Right45,
	Left180,
	Right180,
};

enum class BehaviorMotionType : uint8 
{
	None,
	Stand,
	Sit
};

///////////////////////////////////////////////////////
// Synchronization
///////////////////////////////////////////////////////
enum class SnapshotSyncState : uint8
{
	None			= (uint8)0x00,
	Rotation		= (uint8)0x01,
	Location		= (uint8)0x02,
	Motion			= (uint8)0x04,
	Skill			= (uint8)0x08,
	Controller		= (uint8)0x10
};

#define SNAPSHOT_SYNC_ROTATION		(uint8)SnapshotSyncState::Rotation
#define SNAPSHOT_SYNC_LOCATION		(uint8)SnapshotSyncState::Location
#define SNAPSHOT_SYNC_MOTION		(uint8)SnapshotSyncState::Motion
#define SNAPSHOT_SYNC_SKILL			(uint8)SnapshotSyncState::Skill
#define SNAPSHOT_SYNC_CONTROLLER	(uint8)SnapshotSyncState::Controller
#define SNAPSHOT_SYNC_MOBILE		SNAPSHOT_SYNC_ROTATION | SNAPSHOT_SYNC_LOCATION
#define SNAPSHOT_SYNC_CHARACTER		SNAPSHOT_SYNC_MOBILE | SNAPSHOT_SYNC_MOTION | SNAPSHOT_SYNC_SKILL | SNAPSHOT_SYNC_CONTROLLER
#define SNAPSHOT_SYNC_FF			(uint8)0xFF

#pragma pack(push, 1)

template<uint8 _SyncState>
struct SnapshotData
{
};

#define SNAPSHOT_DATA_SINGLE_UNIT(Sync, Var)	\
template<> struct SnapshotData<Sync>			\
{												\
	Var;										\
};

// Add implement structure for that contains single unit only.
SNAPSHOT_DATA_SINGLE_UNIT(SNAPSHOT_SYNC_ROTATION, float Rotation);
SNAPSHOT_DATA_SINGLE_UNIT(SNAPSHOT_SYNC_LOCATION, float Location[3]);
SNAPSHOT_DATA_SINGLE_UNIT(SNAPSHOT_SYNC_MOTION, MotionState Motion);
SNAPSHOT_DATA_SINGLE_UNIT(SNAPSHOT_SYNC_SKILL, uint32 SkillGUID);
SNAPSHOT_DATA_SINGLE_UNIT(SNAPSHOT_SYNC_CONTROLLER, uint8 ControllerEnabled);

// Add implement structure for that contains multiple units.
// Like this:
// template<> struct SnapshotData<SNAPSHOT_SYNC_X>
// {
//		float Var1;
//		int32 Var2;
// };

template<uint8... _Impls>
struct AmbiguousSnapshotDataImpl
{
};

template<uint8... _Impls>
struct AmbiguousSnapshotDataImpl<0, _Impls...> : public AmbiguousSnapshotDataImpl<_Impls...>
{
};

template<uint8 _Impl, uint8... _Impls>
struct AmbiguousSnapshotDataImpl<_Impl, _Impls...> : public SnapshotData<_Impl>, public AmbiguousSnapshotDataImpl<_Impls...>
{
};

template<uint8 _ImplFlags>
struct AmbiguousSnapshotDataArrangedTypedef
{
private:
	template<size_t... _Seq>
	static constexpr AmbiguousSnapshotDataImpl<(_ImplFlags& (1 << (_Seq)))...> Impl(std::index_sequence<_Seq...>&&);
	static constexpr auto Impl() -> decltype(Impl(std::make_index_sequence<8>{}));

public:
	using T = decltype(Impl());
};

template<uint8 _ImplFlags>
struct ArrangedSnapshotData : public AmbiguousSnapshotDataArrangedTypedef<_ImplFlags>::T
{
};

struct AmbiguousSnapshotData
{

	uint8 SyncState;

	template<uint8 _SyncState>
	auto GetArrangedImpl()
	{
		check1(_SyncState == SyncState);
		return reinterpret_cast<ArrangedSnapshotData<_SyncState>*>(GetArrangedStart());
	}

	template<uint8 _SyncState>
	auto GetArrangedImpl() const
	{
		check1(_SyncState == SyncState);
		return reinterpret_cast<const ArrangedSnapshotData<_SyncState>*>(GetArrangedStart());
	}

	template<uint8 _ImplClass>
	auto GetImplClass()
	{
		static_assert(CountBits<_ImplClass>() == 1, "The count of bits of _ImplClass template parameter of GetImplClass() function must be 1.");
		check1(_ImplClass & SyncState);
		return reinterpret_cast<SnapshotData<_ImplClass>*>(GetArrangedStart() + CalcArrangedGetImplClassHead<0, _ImplClass>(SyncState));
	}

	template<uint8 _ImplClass>
	auto GetImplClass() const
	{
		static_assert(CountBits<_ImplClass>() == 1, "The count of bits of _ImplClass template parameter of GetImplClass() function must be 1.");
		check1(_ImplClass & SyncState);
		return reinterpret_cast<const SnapshotData<_ImplClass>*>(GetArrangedStart() + CalcArrangedGetImplClassHead<0, _ImplClass>(SyncState));
	}

	size_t CompressedSz() const
	{
		return sizeof(SyncState) + CompressedSzImpl<0>(SyncState);
	}

	static size_t CompressedSz(uint8 state)
	{
		return sizeof(SyncState) + CompressedSzImpl<0>(state);
	}

private:
	ArrangedSnapshotData<0xFF> _structuredPadder;

private:
	template<size_t _Flags>
	static constexpr size_t CountBits()
	{
		if constexpr (_Flags == 0) {
			return 0;
		}
		else {
			return ((_Flags & 1) == 0 ? 0 : 1) + CountBits<(_Flags >> 1)>();
		}
	}

	template<size_t _Idx, uint8 _ImplClass>
	static constexpr size_t GetImplClassIndex()
	{
		if constexpr (_Idx == 8) {
			return _Idx;
		}

		constexpr uint8 _Flag = 1 << _Idx;
		if constexpr (_Flag == _ImplClass) {
			return _Idx;
		}
		else {
			return GetImplClassIndex<_Idx + 1, _ImplClass>();
		}
	}

	template<size_t _Idx>
	static inline size_t CompressedSzImpl(uint8 state)
	{
		constexpr uint8 _Flag = ((uint8)1) << ((uint8)_Idx);
		size_t _Sizeof = ((state & _Flag) != 0) ? sizeof(SnapshotData<_Flag>) : 0;

		if constexpr (_Idx + 1 < 8) {
			return _Sizeof + CompressedSzImpl<_Idx + 1>(state);
		}
		else {
			return _Sizeof;
		}
	}

	template<uint8 _Idx, uint8 _ImplClass>
	static inline auto CalcArrangedGetImplClassHead(uint8 sync)
	{
		constexpr bool _This = (1 << _Idx) == _ImplClass;
		const bool impls = sync & (1 << _Idx);
		size_t size = impls ? sizeof(SnapshotData<(1 << _Idx)>) : 0;

		if constexpr (_This) {
			return 0;
		}
		else {
			return CalcArrangedGetImplClassHead<_Idx + 1, _ImplClass>(sync) + size;
		}
	}

	inline uint8* GetArrangedStart() { return reinterpret_cast<uint8*>(this) + sizeof(SyncState); }
	inline const uint8* GetArrangedStart() const { return reinterpret_cast<const uint8*>(this) + sizeof(SyncState); }
};

#pragma pack(pop)

#define MOVING_REPORT_THRESHOLD (float)100.0f
#define MOVING_REPORT_THRESHOLD_SQUARED (float)(MOVING_REPORT_THRESHOLD * MOVING_REPORT_THRESHOLD)
#define MOVING_ESTIMATED_POS_OFFSET (float)(MOVING_REPORT_THRESHOLD * 1.0f)

//////////////////////////////////////////////////
//////////////////////////////////////////////////
#define SAFE_DEFINE_INDEX(symbol, index) const uint8_t symbol = index;

//////////////////////////////////////////////////
//	TYPEID1

SAFE_DEFINE_INDEX(TID1_CHARACTER, 1)
SAFE_DEFINE_INDEX(TID1_ITEM, 2)
SAFE_DEFINE_INDEX(TID1_LIFE_OBJECT, 3)
SAFE_DEFINE_INDEX(TID1_SKILL_OBJECT, 4)
SAFE_DEFINE_INDEX(TID1_PERMISSION, 5)
SAFE_DEFINE_INDEX(TID1_GIMMICK, 6)

//////////////////////////////////////////////////
//	TYPEID2

// TID1_CHARACTER
SAFE_DEFINE_INDEX(TID2_PC, 1)
SAFE_DEFINE_INDEX(TID2_MONSTER, 2)
SAFE_DEFINE_INDEX(TID2_NPC, 3)

// TID1_ITEM
SAFE_DEFINE_INDEX(TID2_USABLE, 1)
SAFE_DEFINE_INDEX(TID2_MATERIAL, 2)
SAFE_DEFINE_INDEX(TID2_EQUIP, 3)
SAFE_DEFINE_INDEX(TID2_COSTUME, 4)
SAFE_DEFINE_INDEX(TID2_ETC, 5)
SAFE_DEFINE_INDEX(TID2_EMBLEM, 6)
SAFE_DEFINE_INDEX(TID2_QUEST, 7)
SAFE_DEFINE_INDEX(TID2_EQUIPSKIN, 8)
SAFE_DEFINE_INDEX(TID2_EMBLEMPIECE, 9)

// TID1_LIFE_OBJECT
SAFE_DEFINE_INDEX(TID2_BERRY, 1)
SAFE_DEFINE_INDEX(TID2_TREE, 2)
SAFE_DEFINE_INDEX(TID2_STONE, 3)
SAFE_DEFINE_INDEX(TID2_FISH, 4)
SAFE_DEFINE_INDEX(TID2_PIN, 5)
SAFE_DEFINE_INDEX(TID2_PORTAL, 6)
SAFE_DEFINE_INDEX(TID2_TOUCH, 7)
SAFE_DEFINE_INDEX(TID2_MANAGING, 8)
SAFE_DEFINE_INDEX(TID2_ANIMAL_SITTING, 9)

// TID1_SKILL_OBJECT
SAFE_DEFINE_INDEX(TID2_PROJECTILE, 1)
SAFE_DEFINE_INDEX(TID2_ORB, 2)
SAFE_DEFINE_INDEX(TID2_FIELD, 3)

//////////////////////////////////////////////////
//	TID3_EQUIP
SAFE_DEFINE_INDEX(TID3_WEAPON, 1)
SAFE_DEFINE_INDEX(TID3_HELMET, 2)
SAFE_DEFINE_INDEX(TID3_ARMOR, 3)
SAFE_DEFINE_INDEX(TID3_BOOTS, 4)
SAFE_DEFINE_INDEX(TID3_GAUNTLET, 5)

// TID3_COSTUME
SAFE_DEFINE_INDEX(TID3_HANDWEAR, 1)
SAFE_DEFINE_INDEX(TID3_HAT, 2)
SAFE_DEFINE_INDEX(TID3_EYEWEAR, 3)
SAFE_DEFINE_INDEX(TID3_MASK, 4)
SAFE_DEFINE_INDEX(TID3_BACK, 5)
SAFE_DEFINE_INDEX(TID3_TAIL, 6)
SAFE_DEFINE_INDEX(TID3_HAIR, 7)
SAFE_DEFINE_INDEX(TID3_OUTFIT, 8)
SAFE_DEFINE_INDEX(TID3_SHOES, 9)
SAFE_DEFINE_INDEX(TID3_EAR, 10)
SAFE_DEFINE_INDEX(TID3_UPPER, 11)
SAFE_DEFINE_INDEX(TID3_LENS, 12)
SAFE_DEFINE_INDEX(TID3_MAKEUP, 13)
SAFE_DEFINE_INDEX(TID3_NPC_BODY, 14)

// TID3_ETC
SAFE_DEFINE_INDEX(TID3_UNLOCKER, 1)

// TID3_Monster
SAFE_DEFINE_INDEX(TID3_HECKLER, 1)

// TID3_NPC
SAFE_DEFINE_INDEX(TID3_FAIRY, 1)

// TID3_SkillObject
SAFE_DEFINE_INDEX(TID3_MISSILE, 1)
SAFE_DEFINE_INDEX(TID3_CHAIN, 2)

// TID3_USABLE
SAFE_DEFINE_INDEX(TID3_CURRENCY, 1)
SAFE_DEFINE_INDEX(TID3_COUPON, 2)
SAFE_DEFINE_INDEX(TID3_REWARD, 3)
SAFE_DEFINE_INDEX(TID3_DYEING, 4)
SAFE_DEFINE_INDEX(TID3_CLASSEXP, 5)
SAFE_DEFINE_INDEX(TID3_EQUIP_UPGRADE, 6)
SAFE_DEFINE_INDEX(TID3_SKILL, 7)

// TID2_MANAGING
SAFE_DEFINE_INDEX(TID3_SWEEP, 1)
SAFE_DEFINE_INDEX(TID3_FENCE, 2)
SAFE_DEFINE_INDEX(TID3_WALL, 3)
SAFE_DEFINE_INDEX(TID3_WIPE, 4)

// TID2_ANIMAL_SITTING
SAFE_DEFINE_INDEX(TID3_BRUSHING, 1)
SAFE_DEFINE_INDEX(TID3_PATTING, 2)
SAFE_DEFINE_INDEX(TID3_FEED, 3)

// TID3_EMBLEM
SAFE_DEFINE_INDEX(TID3_NORMAL, 1)
SAFE_DEFINE_INDEX(TID3_SPECIAL, 2)
SAFE_DEFINE_INDEX(TID3_SOLAR_NORMAL, 3)
SAFE_DEFINE_INDEX(TID3_SOLAR_SPECIAL, 4)

//////////////////////////////////////////////////
//	TYPEID4

// TID3_HANDWEAR
SAFE_DEFINE_INDEX(TID4_BRACELET, 1)
SAFE_DEFINE_INDEX(TID4_PHONE, 2)
SAFE_DEFINE_INDEX(TID4_GLOVES, 3)

// TID3_UPPER
SAFE_DEFINE_INDEX(TID4_NECKLACE, 1)
SAFE_DEFINE_INDEX(TID4_ARMBAND, 2)

// TID3_WEAPON
SAFE_DEFINE_INDEX(TID4_SWORD, 1)
SAFE_DEFINE_INDEX(TID4_BOW, 2)
SAFE_DEFINE_INDEX(TID4_UNIT, 3)

// TID3_DYEING
SAFE_DEFINE_INDEX(TID4_FIXED, 1)
SAFE_DEFINE_INDEX(TID4_RANDOM, 2)
SAFE_DEFINE_INDEX(TID4_COLOR_PIGMENT, 3)

//////////////////////////////////////////////////
struct TypeID
{
	TypeID() { typeID = 0; }
	TypeID(uint32 tid) { typeID = tid; }

	union
	{
		uint32 typeID;
		struct
		{
			uint8 typeID1;
			uint8 typeID2;
			uint8 typeID3;
			uint8 typeID4;
		};
	};

	bool operator==(const TypeID& str)
	{
		return Contains(str);
	}

	bool operator!=(const TypeID& str)
	{
		return Contains(str) == false;
	}

	bool IsCharacter() const { return (typeID1 == TID1_CHARACTER); }
	bool IsSkillObject() const { return (typeID1 == TID1_SKILL_OBJECT); }
	bool IsItem() const { return (typeID1 == TID1_ITEM); }
	bool IsLifeObject() const { return (typeID1 == TID1_LIFE_OBJECT); }
	bool IsPermission() const { return  typeID1 == TID1_PERMISSION; }
	bool IsGimmick() const { return typeID1 == TID1_GIMMICK; }

	bool IsPC() const { return (IsCharacter() && typeID2 == TID2_PC); }
	bool IsMonster() const { return (IsCharacter() && typeID2 == TID2_MONSTER); }
	bool IsNPC() const { return (IsCharacter() && typeID2 == TID2_NPC); }

	bool IsUsable() const { return (IsItem() && (typeID2 == TID2_USABLE)); }
	bool IsMaterial() const { return (IsItem() && typeID2 == TID2_MATERIAL); }
	bool IsEquip() const { return (IsItem() && typeID2 == TID2_EQUIP); }
	bool IsCostume() const { return (IsItem() && typeID2 == TID2_COSTUME); }
	bool IsEmblem() const { return (IsItem() && typeID2 == TID2_EMBLEM); }
	bool IsEquipSkin() const { return (IsItem() && typeID2 == TID2_EQUIPSKIN); }
	bool IsEmblemPiece() const { return IsItem() && typeID2 == TID2_EMBLEMPIECE; }

	bool IsBerry() const { return (IsLifeObject() && typeID2 == TID2_BERRY); }
	bool IsTree() const { return (IsLifeObject() && typeID2 == TID2_TREE); }
	bool IsStone() const { return (IsLifeObject() && typeID2 == TID2_STONE); }
	bool IsFish() const { return (IsLifeObject() && typeID2 == TID2_FISH); }
	bool IsPortal() const { return (IsLifeObject() && typeID2 == TID2_PORTAL); }
	bool IsManaging() const { return (IsLifeObject() && typeID2 == TID2_MANAGING); }
	bool IsAnimalSitting() const { return (IsLifeObject() && typeID2 == TID2_ANIMAL_SITTING); }
	bool IsDummyPin() const { return (IsLifeObject() && typeID2 == TID2_PIN); }

	bool IsProjectile() const { return (IsSkillObject() && typeID2 == TID2_PROJECTILE); }
	bool IsOrb() const { return (IsSkillObject() && typeID2 == TID2_ORB); }
	bool IsField() const { return (IsSkillObject() && typeID2 == TID2_FIELD); }

	bool IsHeckler() const { return (IsMonster() && typeID3 == TID3_HECKLER); }

	bool IsWeapon() const { return ((IsEquip()) && typeID3 == TID3_WEAPON); }
	bool IsHelmet() const { return ((IsEquip()) && typeID3 == TID3_HELMET); }
	bool IsArmor() const { return ((IsEquip()) && typeID3 == TID3_ARMOR); }
	bool IsBoots() const { return ((IsEquip()) && typeID3 == TID3_BOOTS); }
	bool IsGauntlet() const { return ((IsEquip()) && typeID3 == TID3_GAUNTLET); }

	bool IsHandwear() const { return (IsCostume() && typeID3 == TID3_HANDWEAR); }
	bool IsHat() const { return (IsCostume() && typeID3 == TID3_HAT); }
	bool IsEyewear() const { return (IsCostume() && typeID3 == TID3_EYEWEAR); }
	bool IsMask() const { return (IsCostume() && typeID3 == TID3_MASK); }
	bool IsBack() const { return (IsCostume() && typeID3 == TID3_BACK); }
	bool IsTail() const { return (IsCostume() && typeID3 == TID3_TAIL); }
	bool IsEar() const { return (IsCostume() && typeID3 == TID3_EAR); }
	bool IsUpper() const { return (IsCostume() && typeID3 == TID3_UPPER); }
	bool IsLens() const { return (IsCostume() && typeID3 == TID3_LENS); }
	bool IsMakeUp() const { return (IsCostume() && typeID3 == TID3_MAKEUP); }
	bool IsHair() const { return (IsCostume() && typeID3 == TID3_HAIR); }
	bool IsOutfit() const { return (IsCostume() && typeID3 == TID3_OUTFIT); }
	bool IsShoes() const { return (IsCostume() && typeID3 == TID3_SHOES); }

	bool IsUnlocker() const { return (typeID2 == TID2_ETC && typeID3 == TID3_UNLOCKER); }

	bool IsFairy() const { return IsNPC() && typeID3 == TID3_FAIRY; }

	bool IsMissile() const { return IsSkillObject() && typeID3 == TID3_MISSILE; }
	bool IsChain() const { return IsSkillObject() && typeID3 == TID3_CHAIN; }

	bool IsUsableCurrency() const { return IsUsable() && typeID3 == TID3_CURRENCY; }
	bool IsUsableReward() const { return IsUsable() && typeID3 == TID3_REWARD; }
	bool IsUsableDyeing() const { return IsUsable() && typeID3 == TID3_DYEING; }
	bool IsUsableClassExp() const { return IsUsable() && typeID3 == TID3_CLASSEXP; }
	bool IsUsableEquipUpgrade () const { return IsUsable() && typeID3 == TID3_EQUIP_UPGRADE; }
	bool IsUsableCoupon() const { return IsUsable() && typeID3 == TID3_COUPON; }
	bool IsUsableSkill() const { return IsUsable() && typeID3 == TID3_SKILL; }

	bool IsBasicEmblem() const { return IsEmblem() && (typeID3 == TID3_NORMAL || typeID3 == TID3_SPECIAL); }
	bool IsSolarEmblem() const { return IsEmblem() && (typeID3 == TID3_SOLAR_NORMAL || typeID3 == TID3_SOLAR_SPECIAL); }
	bool IsNormalEmblem() const { return IsEmblem() && (typeID3 == TID3_NORMAL || typeID3 == TID3_SOLAR_NORMAL); }
	bool IsSpecialEmblem() const { return IsEmblem() && (typeID3 == TID3_SPECIAL || typeID3 == TID3_SOLAR_SPECIAL); }

	bool IsBracelet() const { return (IsHandwear() && typeID4 == TID4_BRACELET); }
	bool IsPhone() const { return (IsHandwear() && typeID4 == TID4_PHONE); }
	bool IsGloves() const { return (IsHandwear() && typeID4 == TID4_GLOVES); }
	bool IsNecklace() const { return (IsUpper() && typeID4 == TID4_NECKLACE); }
	bool IsArmband() const { return (IsUpper() && typeID4 == TID4_ARMBAND); }

	bool IsSword() const { return typeID4 == TID4_SWORD; }
	bool IsBow() const { return typeID4 == TID4_BOW; }
	bool IsUnit() const { return typeID4 == TID4_UNIT; }

	bool ContainsType(uint8 src, uint8 dst) const { return src == 0 ? true : src == dst; }

	bool Contains(const TypeID& target) const {

		if (ContainsType(typeID1, target.typeID1) == false) {
			return false;
		}

		if (ContainsType(typeID2, target.typeID2) == false) {
			return false;
		}

		if (ContainsType(typeID3, target.typeID3) == false) {
			return false;
		}

		if (ContainsType(typeID4, target.typeID4) == false) {
			return false;
		}

		return true;
	}
};

#define TYPEID_FIELD_COUNT (size_t)4

#define MIN_CHARACTER_NAME_LENGTH (size_t)2
#define MAX_CHARACTER_NAME_LENGTH (size_t)16

#define MAX_CHARACTER_SLOT_COUNT (int32)1

#define DEFAULT_RADIUS (float)31.f

////////////////////////////////////////////////////////////////////////// stat
// 소숫점 2자리까지 지원, 적용 value -> (1 / 100)
using StatValue = int32;

enum class StatApplyType : uint8
{
	Invalid,
	Add,
	Per,
};

#define DEFAULT_POISE_HEALTH (StatValue)100

////////////////////////////////////////////////////////////////////////// customizing
#define MAX_BELL_SOUND_COUNT (uint8)3

////////////////////////////////////////////////////////////////////////// item
#define MAX_EYE_DYE_SLOT_COUNT (uint8)4

#define MAX_ITEM_COUNT					(int)999
#define MAX_INVENSIZE					(size_t)256
#define MAX_EMBLEM_SLOT_COUNT_NORMAL	(size_t)3
#define EMBLEM_SLOT_INDEX_SPECIAL		(uint8)0xFF

//item update mask
#define ITEM_UPDATE_MASK_ADD        (uint8)0x01
#define ITEM_UPDATE_MASK_UPDATED    (uint8)0x02
#define ITEM_UPDATE_MASK_DELETE     (uint8)0x04

enum EquipSlot : uint8
{
	EQUIP_INVALID = 0,
	EQUIP_HELMET = 1,
	EQUIP_ARMOR,
	EQUIP_BOOTS,
	EQUIP_GAUNTLET,
	EQUIP_MAIN_WEAPON,
	EQUIP_SUB_WEAPON,
	EQUIP_COUNT = EQUIP_SUB_WEAPON
};

enum CostumeSlot : uint8
{
	COSTUME_INVALID = 0,
	COSTUME_HANDWEAR = 1,
	COSTUME_HAT,
	COSTUME_EYEWEAR,
	COSTUME_MASK,
	COSTUME_BACK,
	COSTUME_TAIL,
	COSTUME_EAR,
	COSTUME_UPPER,
	COSTUME_LENS,
	COSTUME_MAKEUP,
	COSTUME_DETACHABLE_COUNT = COSTUME_MAKEUP,
	COSTUME_HAIR,
	COSTUME_OUTFIT,
	COSTUME_SHOES,
	COSTUME_PHONE,
	COSTUME_COUNT = COSTUME_PHONE
};

#define CUSTOMIZABLE_COLOR_COUNT (size_t)6

using ItemRefID = uint32;
using ItemDBID = int64;
using CustomizingColors = std::array<uint32, CUSTOMIZABLE_COLOR_COUNT>;

using Equipment = std::array<ItemDBID, EQUIP_COUNT>;

using PresetID = uint8;
using Costume = std::array<ItemRefID, COSTUME_COUNT>;
using Costumes = std::map<PresetID, Costume>;

#define CURRENT_EQUIPMENTS_PRESET	(PresetID)0
#define DEFAULT_EQUIPMENTS_PRESET	(PresetID)3
#define MAX_EQUIPMENTS_PRESET		(PresetID)20
#define BASE_EQUIPMENTS_PRESET		(PresetID)UINT8_MAX

#define SPECIAL_EQUIPMENTS_PRESET			(PresetID)200
#define SPECIAL_EQUIPMENTS_PRESET_AUDITION	(PresetID)((int)SPECIAL_EQUIPMENTS_PRESET + 1)

enum class EmblemType : uint8
{
	None,
	Back = TID3_BACK,
	Tail = TID3_TAIL,
	Hair = TID3_HAIR,
	Outfit = TID3_OUTFIT,
	Shoes = TID3_SHOES
};

constexpr bool EmblemType__IsValid(EmblemType emblemType)
{
	switch (emblemType) {
	case EmblemType::Back:
	case EmblemType::Tail:
	case EmblemType::Hair:
	case EmblemType::Outfit:
	case EmblemType::Shoes:
		return true;
	default:
		return false;
	}
}

constexpr bool EmblemType__IsValid(int32 emblemType)
{
	return EmblemType__IsValid((EmblemType)emblemType);
}

inline EmblemType EmblemType__FromTID(TypeID tid)
{
	if (tid.IsCostume() == false) {
		return EmblemType::None;
	}

	auto ev = (EmblemType)tid.typeID3;
	if (EmblemType__IsValid(ev) == false) {
		return EmblemType::None;
	}

	return ev;
}

enum class ItemUpgradeType : uint8
{
	Upgrade,
	Unlock,
	Emblem,
	EquipCarve,
	EquipTranscendence,
	EmblemTranscendence,
};

// matched with game db => item_carve.carve_type
enum class ItemCarveType : uint8
{
	Staging = 0,
	Commit = 1,
};

namespace ItemUtil
{
	static const std::map<std::string, CostumeSlot> NameToSlotID{
		{"handwear", CostumeSlot::COSTUME_HANDWEAR},
		{"hat", CostumeSlot::COSTUME_HAT},
		{"eyewear", CostumeSlot::COSTUME_EYEWEAR},
		{"mask", CostumeSlot::COSTUME_MASK},
		{"back", CostumeSlot::COSTUME_BACK},
		{"tail", CostumeSlot::COSTUME_TAIL},
		{"hair", CostumeSlot::COSTUME_HAIR},
		{"outfit", CostumeSlot::COSTUME_OUTFIT},
		{"shoes", CostumeSlot::COSTUME_SHOES},
		{"ear", CostumeSlot::COSTUME_EAR},
		{"upper", CostumeSlot::COSTUME_UPPER},
		{"lens", CostumeSlot::COSTUME_LENS},
		{"makeup", CostumeSlot::COSTUME_MAKEUP},
		{"phone", CostumeSlot::COSTUME_PHONE},
	};

	static CostumeSlot GetCostumeSlotByString(const char* string) {
		auto it = NameToSlotID.find(string);
		return it != NameToSlotID.end() ? it->second : COSTUME_INVALID;
	}

	static std::string_view GetCostumeString(CostumeSlot slotIndex){
		for (auto& it : NameToSlotID) {
			if (it.second == slotIndex) {
				return it.first;
			}
		}
		return "";
	}

	static bool IsCostumeHandwear(TypeID& tid) {
		return tid.IsBracelet() || tid.IsGloves();
	}

	static EquipSlot GetEquipSlotByType(TypeID& tid)
	{
		if (tid.IsWeapon()) {
			return EquipSlot::EQUIP_MAIN_WEAPON;
		}
		else if (tid.IsHelmet()) {
			return EquipSlot::EQUIP_HELMET;
		}
		else if (tid.IsArmor()) {
			return EquipSlot::EQUIP_ARMOR;
		}
		else if (tid.IsBoots()) {
			return EquipSlot::EQUIP_BOOTS;
		}
		else if (tid.IsGauntlet()) {
			return EquipSlot::EQUIP_GAUNTLET;
		}
		return EquipSlot::EQUIP_INVALID;
	}

	static CostumeSlot GetCostumeSlotByType(TypeID& tid)
	{
		if (IsCostumeHandwear(tid)) {
			return CostumeSlot::COSTUME_HANDWEAR;
		}
		else if (tid.IsHat()) {
			return CostumeSlot::COSTUME_HAT;
		}
		else if (tid.IsEyewear()) {
			return CostumeSlot::COSTUME_EYEWEAR;
		}
		else if (tid.IsMask()) {
			return CostumeSlot::COSTUME_MASK;
		}
		else if (tid.IsBack()) {
			return CostumeSlot::COSTUME_BACK;
		}
		else if (tid.IsTail()) {
			return CostumeSlot::COSTUME_TAIL;
		}
		else if (tid.IsEar()) {
			return CostumeSlot::COSTUME_EAR;
		}
		else if (tid.IsUpper()) {
			return CostumeSlot::COSTUME_UPPER;
		}
		else if (tid.IsLens()) {
			return CostumeSlot::COSTUME_LENS;
		}
		else if (tid.IsMakeUp()) {
			return CostumeSlot::COSTUME_MAKEUP;
		}
		else if (tid.IsHair()) {
			return CostumeSlot::COSTUME_HAIR;
		}
		else if (tid.IsOutfit()) {
			return CostumeSlot::COSTUME_OUTFIT;
		}
		else if (tid.IsShoes()) {
			return CostumeSlot::COSTUME_SHOES;
		}
		else if (tid.IsPhone()) {
			return CostumeSlot::COSTUME_PHONE;
		}
		return CostumeSlot::COSTUME_INVALID;
	}

	static bool IsWeaponEquipSlotID(EquipSlot slotID) {
		return slotID == EquipSlot::EQUIP_MAIN_WEAPON || slotID == EquipSlot::EQUIP_SUB_WEAPON;
	}

	static bool IsValidEquipSlotID(EquipSlot slot) {
		return slot > EquipSlot::EQUIP_INVALID && slot <= EquipSlot::EQUIP_COUNT;
	}

	static bool IsDetachableSlot(CostumeSlot slot) {
		return slot >= CostumeSlot::COSTUME_HANDWEAR && slot <= CostumeSlot::COSTUME_DETACHABLE_COUNT;
	}
};

////////////////////////////////////////////////////////////////////////// chat
enum class ChatMsgType : uint8
{
	None,
	Text,
	Sticker,
	System,
	TagEvent,
	TextReaction,
	StickerReaction
};

////////////////////////////////////////////////////////////////////////// quest
#define QUEST_REWARD_MASK_AUTO		1
#define QUEST_REWARD_MASK_MANUAL	(QUEST_REWARD_MASK_AUTO << 1)
#define QUEST_REWARD_MASK_GROUP		(QUEST_REWARD_MASK_AUTO << 2)
#define QUEST_REWARD_MASK_SYSTEM	(QUEST_REWARD_MASK_AUTO << 3)
#define QUEST_REWARD_MASK_SEQUENCE	(QUEST_REWARD_MASK_AUTO << 4)
enum class QuestRewardType : uint8
{
	Invalid = 0,
	Auto = QUEST_REWARD_MASK_AUTO,
	Manual = QUEST_REWARD_MASK_MANUAL,
	Group = QUEST_REWARD_MASK_GROUP,
	System = QUEST_REWARD_MASK_SYSTEM,
	Sequence = QUEST_REWARD_MASK_SEQUENCE
};

// 다음 에피소드 추천시 이 값을 통해 정렬하는 로직이 있으므로, enum의 값을 뒤죽박죽 하지 말 것. 작을수록 우선권을 가짐
// Type 추가시 로그 테이블 추가 및 log builder에 분산하는 코드도 작성 필요
enum class QuestGroupType : uint8 
{
	Invalid = 0,
	Hidden_Event,
	Stage,
	Tutorial,
	Essential,
	Class,
	Arbeit,
	Epic,
	Challenge,
	Keyword,
	Hidden,
	Fairy,
	TimeEvent,
	Attendance,
	PassMission,
	Activity,
	Blacklist,
	Prototype,
	ContentsGuide,
	MassiveArbeit,
	Count
};

enum class ArbeitCategory : uint8
{
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

enum class QuestClientTaskType : uint8
{
	Invalid = 0,
	// streaming
	Streaming,
	StreamingDonation,
	TakePhoto,
	Inspecting,
	ItemStreaming,
	ItemReview,
	ItemTrade,
	// dialog
	DialogEnd,
	// condition update
	FeedReaction,
	ContentsGuide,
	Switch,
	SubmitKey,
	OriginalsIntro,
	TryUpdate,
	PassBoost,
	PassMissionReroll,
};

enum class QuestChallengeRankType : uint8
{
	Invalid =0,
	Default,
	Effort,
	Lucky,
	Extra,
};
constexpr size_t ChallengeCeremonyRankerCnt = 3;

#define CHALLENGE_REWARD_MASK_JOIN		1
#define CHALLENGE_REWARD_MASK_RANKING	(CHALLENGE_REWARD_MASK_JOIN << 1)
enum class QuestChallengeRwdType : uint8
{
	Invalid = 0,
	Join = CHALLENGE_REWARD_MASK_JOIN,
	Ranking = CHALLENGE_REWARD_MASK_RANKING,
};

enum class QuestRuntimeMode : uint8
{
	Unmarked = 0,
	Running = 1,
	Completed = 2,
};

////////////////////////////////////////////////////////////////////////// skill
static constexpr uint8 CLASS_SKILL_DEFAULT_TIER{ 0 };

enum class SkillEffect : uint8
{
	None = 0, 
	
	// 일반적인 효과타입
	TimeScale,
	Damage,
	Damage_Fixed,
	HP,
	MaxHP_Fixed,
	ClassChange,
	Impulse_Airborne,
	Impulse_Push,
	Impulse_Pull,
	Move,
	Bump,
	Warp,
	MakeObject,
	Dispel,
	LockOn,
	SpawnParticle,
	CoolDown,
	Immune,

	// 스탯
	Stats,
	FinalDamage,
	CriticalChance,
	CriticalDamage,
	ElementDamage,
	HP_Regen,
	HP_Recovery,
	Melee_Attack,
	Magic_Attack,
	Melee_Defense,
	Magic_Defense,
	MoveSpeed,
	AttackIncrement_Per,
	DamageReduction,
	Slow,

	// 상태이상
	State,
	Shock,
	Stun,
	HoldAnim,
	Frog,
	Untouchable,
	Channel,
	Silence,
	Invincible,
	Invisible,
	Immortal,
	SuperArmor,
	Godlike,
	BodyEffectBuff,
	BodyEffectDebuff,

	// 특수
	CounterSkill,

	// 오러
	Aura,
	IncreaseDamageTaken_Aura,
	ReduceRecovery_Aura,

	// 특정 스테이지만을 위한 특수 효과
	DropOrb,
	FeverTime,
	Score_Per
};

enum class SkillEffectGroup : uint8 {
	None, Transform, Dot, Control, Weakening, Strengthen
};

enum class DispelMatchType : uint8 {
	None, Group, Caster, Skill, Timeline
};

enum class MoveDirection : uint8 {
	Forward = 0, Backward,
};

enum class AutoMoveState : uint8
{
	None, JustStartedMoving, Moving, AtRest
};

#define	BLOCK_MASK_NAVIGATION			(uint8)0x01
#define BLOCK_MASK_LIFEOBJ_INTERACTION	(uint8)0x02
#define BLOCK_MASK_NPC_INTERACTION		(uint8)0x04
//#define BLOCK_MASK_ITEM_USE				(uint8)0x08
#define BLOCK_MASK_CHAT					(uint8)0x10
#define BLOCK_MASK_STAGE				(uint8)0x20
#define BLOCK_MASK_SKILL_BASIC			(uint8)0x40
#define BLOCK_MASK_SKILL_ACTIVE			(uint8)0x80
#define BLOCK_MASK_ALL					(uint8)0xff

namespace SkillUtil
{
	static constexpr int32 TIME_INFINITY{ -1 };

	static SkillEffect GetEffectByString(const char* str) 
	{
		static std::map<std::string, SkillEffect> EFFECTS = {

			// 일반적인 효과타입
			{ "TimeScale", SkillEffect::TimeScale },
			{ "Damage_Add", SkillEffect::Damage },
			{ "Damage_Per", SkillEffect::Damage },
			{ "Damage_Fixed", SkillEffect::Damage_Fixed },
			{ "HP_Per", SkillEffect::HP },
			{ "MaxHP_Fixed", SkillEffect::MaxHP_Fixed },
			{ "ClassChange", SkillEffect::ClassChange },
			{ "Impulse_Airborne", SkillEffect::Impulse_Airborne },
			{ "Impulse_Push", SkillEffect::Impulse_Push },
			{ "Impulse_Pull", SkillEffect::Impulse_Pull },
			{ "Move", SkillEffect::Move },
			{ "Bump", SkillEffect::Bump },
			{ "Warp", SkillEffect::Warp },
			{ "MakeObject", SkillEffect::MakeObject },
			{ "Dispel", SkillEffect::Dispel },
			{ "LockOn", SkillEffect::LockOn },
			{ "SpawnParticle", SkillEffect::SpawnParticle },
			{ "CoolDown", SkillEffect::CoolDown },

			// 스탯
			{ "Stats", SkillEffect::Stats },
			{ "FinalDamage", SkillEffect::FinalDamage },
			{ "CriticalChance", SkillEffect::CriticalChance },
			{ "CriticalDamage", SkillEffect::CriticalDamage },
			{ "ElementDamage", SkillEffect::ElementDamage },
			{ "HP_Regen", SkillEffect::HP_Regen },
			{ "HP_Recovery", SkillEffect::HP_Recovery },
			{ "Melee_Attack", SkillEffect::Melee_Attack },
			{ "Magic_Attack", SkillEffect::Magic_Attack },
			{ "Melee_Defense", SkillEffect::Melee_Defense },
			{ "Magic_Defense", SkillEffect::Magic_Defense },
			{ "MoveSpeed", SkillEffect::MoveSpeed },
			{ "AttackIncrement_Per", SkillEffect::AttackIncrement_Per },
			{ "DamageReduction", SkillEffect::DamageReduction },
			{ "Slow", SkillEffect::Slow },
			
			// 상태이상
			{ "State", SkillEffect::State },
			{ "Shock", SkillEffect::Shock },
			{ "Stun", SkillEffect::Stun },
			{ "HoldAnim", SkillEffect::HoldAnim },
			{ "Frog", SkillEffect::Frog },
			{ "Untouchable", SkillEffect::Untouchable },
			{ "Channel", SkillEffect::Channel },
			{ "Silence", SkillEffect::Silence },
			{ "Invincible", SkillEffect::Invincible },
			{ "Invisible", SkillEffect::Invisible },
			{ "Immortal", SkillEffect::Immortal },
			{ "SuperArmor", SkillEffect::SuperArmor },
			{ "Godlike", SkillEffect::Godlike },
			{ "BodyEffectBuff", SkillEffect::BodyEffectBuff },
			{ "BodyEffectDebuff", SkillEffect::BodyEffectDebuff },
			{ "Immune", SkillEffect::Immune },

			// 특수
			{ "CounterSkill", SkillEffect::CounterSkill },

			// 오러
			{ "Aura", SkillEffect::Aura },
			{ "IncreaseDamageTaken_Aura", SkillEffect::IncreaseDamageTaken_Aura },
			{ "ReduceRecovery_Aura", SkillEffect::ReduceRecovery_Aura },
			
			// 특정 스테이지만을 위한 특수 효과
			{ "DropOrb", SkillEffect::DropOrb },
			{ "FeverTime", SkillEffect::FeverTime },
			{ "Score_Per", SkillEffect::Score_Per },
		};

		auto itr = EFFECTS.find({ str });
		return itr->second;
	}

	static SkillEffectGroup GetEffectGroupByString(const char* str)
	{
		static std::map<std::string, SkillEffectGroup> GROUPS = {
			{"None"	, SkillEffectGroup::None },
			{"Transform", SkillEffectGroup::Transform },
			{"Control", SkillEffectGroup::Control },
			{"Weakening", SkillEffectGroup::Weakening },
			{"Strengthen", SkillEffectGroup::Strengthen },
		};
		auto itr = GROUPS.find({ str });
		return itr->second;
	}

	static DispelMatchType GetDispelMatchTypeByString(const char* str)
	{
		static std::map<std::string, DispelMatchType> MAPS = {
			{"Group", DispelMatchType::Group },
			{"Caster", DispelMatchType::Caster },
			{"Skill", DispelMatchType::Skill },
			{"Timeline", DispelMatchType::Timeline }
		};
		auto itr = MAPS.find({ str });
		return itr->second;
	}

	static StatApplyType GetApplyTypeByString(const char* str)
	{
		std::string temp{ str };
		if (temp.find("Per") != std::string::npos) {
			return StatApplyType::Per;
		}
		else if (temp.find("Add") != std::string::npos) {
			return StatApplyType::Add;
		}
		else {
			return StatApplyType::Invalid;
		}
	}

	static uint8 GetBlockState(SkillEffect eftType)
	{
		static std::map<SkillEffect, uint8> BLOCK_MPAS{
			{SkillEffect::Stun, BLOCK_MASK_SKILL_BASIC | BLOCK_MASK_SKILL_ACTIVE | BLOCK_MASK_NAVIGATION },
			{SkillEffect::Shock, BLOCK_MASK_SKILL_BASIC | BLOCK_MASK_SKILL_ACTIVE | BLOCK_MASK_NAVIGATION },
			{SkillEffect::Channel, BLOCK_MASK_NAVIGATION },
			{SkillEffect::Frog, BLOCK_MASK_SKILL_BASIC | BLOCK_MASK_SKILL_ACTIVE | BLOCK_MASK_STAGE },
			{SkillEffect::Silence, BLOCK_MASK_SKILL_ACTIVE },
			{SkillEffect::HoldAnim, BLOCK_MASK_ALL },
		};
		auto itr = BLOCK_MPAS.find(eftType);
		return itr != BLOCK_MPAS.end() ? itr->second : 0;
	}

	static uint8 GetBodyState(SkillEffect eftType)
	{
		static std::map<SkillEffect, uint8> BODY_MPAS{
			{ SkillEffect::Frog,			BODYMODE_MASK_TRANSFORM | BODYMODE_MASK_UNTOUCHABLE },
			{ SkillEffect::Invincible,		BODYMODE_MASK_INVINCIBLE },
			{ SkillEffect::Invisible,		BODYMODE_MASK_INVISIBLE | BODYMODE_MASK_UNTOUCHABLE },
			{ SkillEffect::SuperArmor,		BODYMODE_MASK_SUPERARMOR },
			{ SkillEffect::Immortal,		BODYMODE_MASK_IMMORTAL },
			{ SkillEffect::Untouchable,		BODYMODE_MASK_UNTOUCHABLE },
			{ SkillEffect::Godlike,			BODYMODE_MASK_INVINCIBLE | BODYMODE_MASK_SUPERARMOR },
		};
		auto itr = BODY_MPAS.find(eftType);
		return itr != BODY_MPAS.end() ? itr->second : 0;
	}

	enum class EffectState : uint8 {
		Body = 0x01,
		Block = 0x02,
	};
}

////////////////////////////////////////////////////////////////////////// 
enum class Gender : uint8
{
	Female = 0,
	Male,
	Invalid
};

// 숫자로 정렬할 것임. 높을수록 상단에 노출됨
enum class RewardObjectType : uint8
{
	Invalid,
	Friend,
	Favor,
	Popularity,
	Title,
	UserInteraction,
	Object,
	Currency,
	ClassExp,
	ClassLicense,
	Tag,
	Permission,
	Stat,
	QuestPoint,
	Palette,
	Stress,
	Follow,
	ChatSticker,
	SocialMotion,
};

////////////////////////////////////////////////////////////////////////// user runtime interaction
enum class RtInteractionType : uint8
{
	Arbeit,
	Matching,
	Matching_Player,
	Matching_Spectator,
	MassiveArbeit,
	Count,
};

////////////////////////////////////////////////////////////////////////// post msg
enum class PostMsgType : uint8
{
	Favor,
	Shop,
	Reward,
	Object,
	Notify, // deprecated by notice system
	SystemRewardOver,
	Package,
	Maintenance,
	Advertisement,
	Count
};

namespace PostMsgUtil {
	static std::map<std::string, PostMsgType> s_types{
				{"Favor", PostMsgType::Favor},
				{"Shop", PostMsgType::Shop},
				{"Reward", PostMsgType::Reward},
				{"Object", PostMsgType::Object},
				{"Notify", PostMsgType::Notify},
				{"SystemRewardOver", PostMsgType::SystemRewardOver},
				{"Package", PostMsgType::Package},
				{"Maintenance", PostMsgType::Maintenance},
				{"Advertisement", PostMsgType::Advertisement},
	};
	static PostMsgType GetType(const std::string& typeStr)
	{
		auto it = s_types.find(typeStr);
		return it->second;
	}
	static std::string GetString(PostMsgType type) {
		for (auto& it : s_types) {
			if (it.second == type) {
				return it.first;
			}
		}
		return "";
	}
}

enum PostMsgActionMask : uint8
{
	Open = 1,
};

////////////////////////////////////////////////////////////////////////// permission
// 순서 바꾸면 bp에서 꼬일 수 있으니 주의
enum class PermissionType : uint8
{
	Invalid = 0,
	Tutorial,
	Beginner,
	Originals,
	Friend,
	Arbeit,
	BattleStage,
	Class,
	Fashion_Shop,
	Streaming,
	Title,
	Equip,
	Topic,
	Status,
	MagicShop,
	UserInteraction,
	Equip_Upgrade,
	Feed,
	Crew,
	MMO_Stage,
	Blacklist,
	QuestChallenge,
	Collection,
	Fashion_Color,
	Fashion_Snap,
	Fashion_PVP,
	Profile,
	MMO_Stage_Raid,
	MMO_Games,
	MMO_FashionShow,
	Pamphlet,
	Portal_Wolfgang,
	Reforge,
	Arbeit_Benefit,
	Market_Benefit,
	Fashion_Emblem,
	Social_Benefit,
	Event_Menu,
	MMO_Stage_Raid_Observation,
	Pass,
	Fashion_ColorPigment,
	Tower,
	Stage_Hard,
	SkillTree,
	Weapon_SubSlot,
	Weapon_Swap
};

namespace PermissionUtil
{
	static std::map<std::string, PermissionType> PermissionTypes
	{
		// common
		{ "Tutorial", PermissionType::Tutorial },
		{ "Beginner", PermissionType::Beginner },
		// quest
		{ "Originals", PermissionType::Originals },
		{ "Title", PermissionType::Title },
		{ "Arbeit", PermissionType::Arbeit },
		{ "Arbeit_Benefit", PermissionType::Arbeit_Benefit },
		{ "Blacklist", PermissionType::Blacklist },
		// stage
		{ "Stage", PermissionType::BattleStage },
		{ "Stage_Hard", PermissionType::Stage_Hard },
		{ "Tower", PermissionType::Tower },
		// class
		{ "Class", PermissionType::Class },
		// feed
		{ "Feed", PermissionType::Feed },
		{ "Topic", PermissionType::Topic },
		// pamphlet
		{ "Pamphlet", PermissionType::Pamphlet },
		{ "Pass", PermissionType::Pass },
		// activity
		{ "Streaming", PermissionType::Streaming },
		// status
		{ "Status", PermissionType::Status },
		// equip
		{ "Equip", PermissionType::Equip },
		{ "Equip_Upgrade", PermissionType::Equip_Upgrade },
		{ "Collection", PermissionType::Collection },
		{ "Reforge", PermissionType::Reforge },
		{ "Weapon_SubSlot", PermissionType::Weapon_SubSlot },
		{ "Weapon_Swap", PermissionType::Weapon_Swap },
		// fashion
		{ "Fashion_Shop", PermissionType::Fashion_Shop },
		{ "Fashion_Color", PermissionType::Fashion_Color },
		{ "Fashion_ColorPigment", PermissionType::Fashion_ColorPigment },
		{ "Fashion_Snap", PermissionType::Fashion_Snap },
		{ "Fashion_PVP", PermissionType::Fashion_PVP },
		{ "Fashion_Emblem", PermissionType::Fashion_Emblem },
		{ "MagicShop", PermissionType::MagicShop },
		// community
		{ "UserInteraction", PermissionType::UserInteraction },
		{ "Profile", PermissionType::Profile },
		{ "Friend", PermissionType::Friend },
		{ "Crew", PermissionType::Crew },
		{ "Social_Benefit", PermissionType::Social_Benefit },
		// challenge
		{ "Event_Menu", PermissionType::Event_Menu },
		{ "Challenge", PermissionType::QuestChallenge },
		// mmo
		{ "MMO_Stage", PermissionType::MMO_Stage },
		{ "MMO_Stage_Raid", PermissionType::MMO_Stage_Raid },
		{ "MMO_Stage_Raid_Observation", PermissionType::MMO_Stage_Raid_Observation },
		{ "MMO_Games", PermissionType::MMO_Games },
		{ "MMO_FashionShow", PermissionType::MMO_FashionShow },
		// portal
		{ "Portal_Wolfgang", PermissionType::Portal_Wolfgang },
		// market
		{ "Market_Benefit", PermissionType::Market_Benefit },
		// skill
		{ "SkillTree", PermissionType::SkillTree },
	};

	static PermissionType GetPermissionType(const std::string& str)
	{
		auto it = PermissionTypes.find(str);
		check1(it != PermissionTypes.end());
		return it->second;
	}

	static const std::string& GetPermissionString(PermissionType type)
	{
		static const std::string NAME_None = "None";

		for (auto& it : PermissionTypes) {
			if (it.second == type) {
				return it.first;
			}
		}
		return NAME_None;
	}
};
// [end] permission

////////////////////////////////////////////////////////////////////////// match make
enum class MatchMakeCmd : uint8
{
	Match,
	QuickMatch,
	Update,
	Ready,
	Complete,
	Cancel,
};

////////////////////////////////////////////////////////////////////////// friend
enum class FriendStatus : uint8 
{
	None,
	Follower,
	Following,
	F4F,
	Proposer,
	Proposing,
	Friend,
	Blocked
};

enum class FriendState : uint8
{
	None = 0,
	Follow = 1,
	Friend = 2,
	Blocked = 3
};

enum class FriendContents : uint8
{
	Warp,
	Recommend,
};

////////////////////////////////////////////////////////////////////////// proxy
enum class ProxyType : uint8
{
	None,
	EventReporting,
	CombatManaging,
	RoundManaging,
	LevelDesign
};

enum class ReportMask : uint8
{
	None  = 0x00,
	Move  = 0x01,
	Dash  = 0x02,
	Skill = 0x04,
	Kill  = 0x08,
	Flush = 0x10
};

inline constexpr ReportMask operator|(ReportMask a, ReportMask b)
{
	return static_cast<ReportMask>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

inline constexpr ReportMask operator&(ReportMask a, ReportMask b)
{
	return static_cast<ReportMask>(static_cast<uint8>(a) & static_cast<uint8>(b));
}

// session renewal
#define SESSION_RENEWAL_TIME_OUT (uint32)(120)
#define SESSION_RENEWAL_CHECK_INTERVAL_SEC (float)(30.f)
#define SESSION_RENEWAL_CHECK_INTERVAL (uint32)(SESSION_RENEWAL_CHECK_INTERVAL_SEC * 100)

// session restore backup state
enum class BackupState : uint8 
{
	None,
	Immigration,
	Lobby,
	WorldEntering,
	WorldPlay,
};

////////////////////////////////////////////////////////////////////////////////
// fashion contents
#define FASHION_CONTENTS_SKILL_COUNT 2

////////////////////////////////////////////////////////////////////////////////
// crew
enum class CrewGrade : uint8
{
	Normal, Staff, Master, None
};

#define CREW_COMMENT_MAX_lEN 400

////////////////////////////////////////////////////////////////////////////////
// spectating
enum class SkillPatronageTarget : uint8
{
	PC,
	Mob
};

inline bool TryParse(std::string_view name, SkillPatronageTarget& value)
{
	if (name == "PC") {
		value = SkillPatronageTarget::PC;
		return true;
	}
	else if (name == "Mob") {
		value = SkillPatronageTarget::Mob;
		return true;
	}
	else {
		return false;
	}
}

inline std::string_view ToString(SkillPatronageTarget value)
{
	switch (value) {
	case SkillPatronageTarget::PC:
		return "PC";
	case SkillPatronageTarget::Mob:
		return "Mob";
	default:
		return "Unknown";
	}
}

constexpr uint8 MULTI_JOIN_PLAYER_SEND_COUNT{ 3 };

////////////////////////////////////////////////////////////////////////////////
// shared quest
enum class ESharedQuestDisconnectReason : uint8
{
	Unknown,
	UserDisconnected,
	QuestCompleted,
	OwnerUserCanceled,
	UserCanceled,
	Expired,
};

////////////////////////////////////////////////////////////////////////////////
// gallery
constexpr uint8 PHTOGRPR_GALLERY_lIKE_COUNT{ 5 };
constexpr uint8 PHTOGRPR_GALLERY_REPORT_REASON_LEN{ 250 };

// 인덱스 db에 저장
enum class GalleryAchvmType : uint8 {
	Invalid,
	Take_Like, // 좋아요
	Take_Post, // 댓글
	Act_Like,
	Act_Post,
};

////////////////////////////////////////////////////////////////////////////////
// guest book
enum class GuestbookType : uint8
{
	Invalid,
	Post, // 댓글
	Reply, // 대댓글
};


////////////////////////////////////////////////////////////////////////////////
// Coupon API resopnses.
enum class CouponApiReplyStatusCodes : int32
{
	Ok = 0,

	// Service Status Codes...
	CharacterNotBeFound = -1,
	CharacterMetadataMismatch = -2,
	CouponNotBeFound = -3,
	WorldServerResponseCorrupted = -4,
	WaitingToApply = -5,
	CriticalError = -6,
	ServiceNotFound = -7,

	// Coupon API Server Status Codes...
	InvalidCouponId = 101,
	InvalidCouponCode = 102,
	CouponDataCorrupted = 103,
	DuplicatedCoupon = 104,
	MismatchAccountId = 105,
	CouponLimited = 106,
	DuplicatedGroupName = 107,
	AlreadyDeleted = 108,
	AlreadyUsed = 109,
	AlreadyExhausted = 110,
	DateExpired = 111,
	InvalidGenerateCode = 112,
	SpecifiedGenerateCode = 113,
	UserAccessDeniedTriggered = 114,
	UserAccessDenied = 115,
	InvalidUseCount = 116,
	UseCountLimited = 117,

	Unknown = -999,
	GameCodeError = -998,
	InternalSystemError = -997,
	InternalDatabaseError = -996,
	NoDataError = -995,
	ParameterError = -994
};



////////////////////////////////////////////////////////////////////////////////
// Tag Component
enum class TagComponentClass : uint8
{
	Default,
	Level1,
	Level2,
	MAX_CLASSES
};

enum class TagComponentTypeMasks : uint8
{
	None				= 0,

	// CLASS_Default
	Nickname			= 0b1,

	// CLASS_Level1
	ArbeitHUD			= 0b1,
	Title				= 0b10,
	Matchmake			= 0b100,

	// CLASS_Level2
	BusyState			= 0b1,
	Chat				= 0b10,
	Highfive			= 0b100,

	All					= 0xFF
};

MAKE_ENUM_CLASS_MASK(TagComponentTypeMasks);


////////////////////////////////////////////////////////////////////////////////
// Remote user save
enum class RemoteSaveType : uint8
{
	None = 0,
	ClassBookMark,
	ContestReaction,
	Ranking,
	CostumePresetSave
};



#undef MAKE_ENUM_CLASS_MASK
#undef check1