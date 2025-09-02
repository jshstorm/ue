#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "KeyGenerator.generated.h"
/////////////////////////////////////////////
// CRC32 - for generating checksum value
/////////////////////////////////////////////
#define CRC32_TABLE_SIZE		256

UCLASS()
class ANUREFERENCE_API UCRC32 : public UObject
{
	GENERATED_BODY()
public:
	//virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//virtual void Deinitialize() override;

	virtual void BeginDestroy() override;

	void Initialize(uint32 seed = 0);
	uint32 Generate32(const FName& uid);
	void Reset() { _table = 0; }
	static UCRC32* GetPtr();
	
protected:
	uint32 _table = 0;
	//uint32 _seed = 0;

public:
	static uint32 crc32Seed[CRC32_TABLE_SIZE];
	static uint32 crcTable[CRC32_TABLE_SIZE][CRC32_TABLE_SIZE];
	static bool crcInitialized;

private:
	uint32 Generate32(const char* data, unsigned int length);
	
	static UCRC32* _instance;
	void InitializeCrcLookupTable();
};

/////////////////////////////////////////////
// SeqGenerator
/////////////////////////////////////////////
#define SEQ_GEN_DUMMY_PARAM		(unsigned char)0x00

class CSeqNumGenerator
{
public:
	CSeqNumGenerator()
	{
		Reset();
	}

	void SetParams(unsigned char adder, unsigned char multiplier)
	{ 
		if (adder == 0) {
			adder = 1;
		}

		if (multiplier == 0) {
			multiplier = 1;
		}

		_sequence = adder ^ multiplier;
		_adder = adder;
		_multiplier	= multiplier;
	}

	unsigned char GetCurSeqNum() { return _sequence; }
	void SetCurSeqNum(unsigned char seq) { _sequence = seq; }

	unsigned char GenSeqNum()
	{
		_sequence = ((~_sequence) + _adder) * _multiplier;
		_sequence = _sequence ^ (_sequence >> 4);
		return _sequence; 
	}
	
	void Reset()
	{
		_adder = SEQ_GEN_DUMMY_PARAM;
		_multiplier = SEQ_GEN_DUMMY_PARAM;
	}
	
private:
	unsigned char	_sequence;
	unsigned char	_adder;
	unsigned char	_multiplier;
};

class CRndGenerator
{
public:
	CRndGenerator(unsigned int seed)
	{
		if (seed == 0) {
			seed = 0x9abfb3b6;
		}

		_seed = seed;
	}

	unsigned int Generate()
	{
		unsigned int value1 = SampleValue();
		unsigned int value2 = SampleValue();

		value1 ^= value2;
		return value1;
	}

private:
	unsigned int _seed = 0;

private:
	unsigned int SampleValue()
	{
		unsigned int temp = _seed;
		unsigned int bit = 0;

		for (unsigned int n = 0; n < 32; ++n)
		{
			bit  = ((temp >> 0) ^ (temp >> 1) ^ (temp >> 2) ^ (temp >> 3) ^ (temp >> 5) ^ (temp >> 7)) & 1;
			temp = (((temp >> 1) | (temp << 31)) & ~1) | bit;
		}

		_seed = temp;

		return temp;
	}
};
