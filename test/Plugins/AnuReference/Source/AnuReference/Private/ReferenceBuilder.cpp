#include "ReferenceBuilder.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Misc/DefaultValueHelper.h"
#include "Misc/FileHelper.h"
#include "Kismet/KismetStringTableLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraActor.h"
#include "Engine/AssetManager.h"
#include "Serialization/JsonSerializer.h"
#include "GameFramework/Character.h"
#include "Particles/ParticleSystem.h"

#include "BehaviorTree/BehaviorTree.h"
#include "Sound/SoundWave.h"

#include "Internationalization/StringTableRegistry.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTable.h"

#include "Reference.h"

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

FString UReferenceBuilder::GetJsonSrcDirectory()
{
	return FPaths::ProjectContentDir() + "Anu/JsonSrc/";
}

FString UReferenceBuilder::GetJsonIndexPath()
{
	return GetJsonSrcDirectory() + "index.json";
}

bool UReferenceBuilder::Initialize()
{
	//FReferenceLogBuilder::Cleanup();

	UE_LOG(LogReference, Verbose, TEXT("UReferenceBuilder::Initialize"));
	auto started = FPlatformTime::Seconds();

#define REGISTER_ABSTRACT_TABLE(klass) \
	_references.Add(klass::StaticClass(), NewObject<UReferences>(this));
#define REGISTER_ABSTRACT_TABLE_WITH_POSTHANDLER(klass) { \
	_references.Add(klass::StaticClass(), NewObject<UReferences>(this)); \
	_postProcessors.Emplace([this]() { UReferenceBuilder::klass##PostProcessor(); }); \
}

	if (LoadFiles() == false) {
		return false;
	}

	auto end = FPlatformTime::Seconds();
	UE_LOG(LogReference, Verbose, TEXT("UReferenceBuilder::Initialize completed! takes [%.2f] sec"), end - started);

	return true;
}

void UReferenceBuilder::InitializeDataTable()
{

}

void UReferenceBuilder::Finalize()
{
	_refHandlers.Empty();
	_postProcessors.Empty();

	_references.Empty();
	_globals.Empty();
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
				//checkRefMsgfCont(Error, false, TEXT("[%s] json src file[%s] format is invalid; open file and check by text editor"), *tableName, *path);
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
