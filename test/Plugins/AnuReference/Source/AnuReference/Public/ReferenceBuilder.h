#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "KeyGenerator.h"
#include "ReferenceBuilder.generated.h"

DEFINE_LOG_CATEGORY_STATIC(LogReference, Verbose, All);

class FXmlNode;
class FJsonObject;
class URefBase;

UCLASS()
class ANUREFERENCE_API UReferences : public UObject
{
	GENERATED_BODY()

	UPROPERTY()
		TMap<int32, URefBase*> _references;
	UPROPERTY()
		TMap<FName, URefBase*> _referencesByUID;

public:
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

public:
	inline static FString SpawnPathPrefix{ "Spawn_" };

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

private:
	void InitializeDataTable();

	bool LoadFiles();
	TSharedPtr<class FXmlFile> LoadTableFile(const FString& name);

	void IterateNodes(const FXmlNode* root, TFunction<void(const FXmlNode*)> handler);

public:
	template<class T>
	UReferences* GetReferences()
	{
		return _references.FindRef(T::StaticClass());
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
};
