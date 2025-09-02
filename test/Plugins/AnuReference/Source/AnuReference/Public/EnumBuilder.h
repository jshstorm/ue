#pragma once

#include "CoreMinimal.h"
#include "EnumBuilder.generated.h"

UCLASS()
class ANUREFERENCE_API UEnumBuilder : public UObject
{
	GENERATED_BODY()

public:
	using EnumData = TMap<FString, int32>;
	static TMap<FString, EnumData> _datas;

public:
	void Initialize();
	void Finalize();

public:
	static int32 GetEnumValue(const FString& enumType, const FString& key);
	static const EnumData* GetEnumDatas(const FString& enumType);
};
