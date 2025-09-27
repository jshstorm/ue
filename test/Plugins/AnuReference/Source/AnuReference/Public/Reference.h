#pragma once

#include "CoreMinimal.h"
#include "XmlParser.h"
#include "Engine/DataTable.h"
#include "Sound/SoundBase.h"
#include "UObject/TextProperty.h"
#include "Engine/Texture2D.h"

#include "Reference.generated.h"

#define DATA_NULL_VALUE TEXT("null")
class FJsonObject;

#define SAFE_CAST(src, Type, dst) \
	check(Cast<Type>(src)); Type* dst = static_cast<Type*>(src);

UCLASS(BlueprintType)
class ANUREFERENCE_API URefBase : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		FName UID;
	UPROPERTY(BlueprintReadOnly, Category = "Anu|Reference")
		int32 GUID = 0;

	virtual void Parse(const FXmlNode* node);
	virtual void Parse(UScriptStruct* type, FTableRowBase* row);
	//virtual void Parse(const FJsonObject* root);

	void ParseArrayProperty(const FArrayProperty* arrProp, const TArray<FString>& strValues, void* memberProp);

	FName GetEnumValueName(const FName& enumMemberName, int64 value);
	FString GetEnumValueString(const FName& enumMemberName, int64 value, bool trimNamespace = false);
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
class ANUREFERENCE_API URefResourceCore : public URefResourceBase
{
	GENERATED_BODY()
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
