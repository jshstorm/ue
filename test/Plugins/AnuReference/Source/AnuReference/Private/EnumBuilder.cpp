#include "EnumBuilder.h"

#define ENUMSTRING_BEGIN(T) {		\
auto& data = UEnumBuilder::_datas.Add(TEXT(#T));

#define ENUMSTRING_ADD(V) data.Add(TEXT(#V), V);

#define ENUMSTRING_END \
}

TMap<FString, TMap<FString, int32>> UEnumBuilder::_datas;

void UEnumBuilder::Initialize()
{

}

void UEnumBuilder::Finalize()
{
	UEnumBuilder::_datas.Empty();
}

int32 UEnumBuilder::GetEnumValue(const FString& enumType, const FString& key)
{
	const EnumData* data = UEnumBuilder::_datas.Find(enumType);
	if (!data) {
		return 0;
	}

	int32 value = data->FindRef(key);
	return value;
}

const UEnumBuilder::EnumData* UEnumBuilder::GetEnumDatas(const FString& enumType)
{
	const EnumData* data = UEnumBuilder::_datas.Find(enumType);
	return data;
}
