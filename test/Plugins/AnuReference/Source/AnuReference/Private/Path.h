// Copyright 2017 CLOVERGAMES Co., Ltd. All right reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Provides methods for processing file system strings in a cross-platform manner.
 */
class FPath final
{
public:
	static constexpr const TCHAR UniversalDirectorySeparator = TEXT('/');

private:
	// The Windows Directory Separator is not supports for cross-platform. But,
	// Windows supports Universal Directory Separator in their directory system.
	static constexpr const TCHAR WindowsDirectorySeparator = TEXT('\\');

	enum class EDirectoryComposeNode
	{
		CurrentDirectory,
		ParentDirectory,
		DirectoryName,
		Invalid
	};

	FString _absolute;
	FString _parent;
	FString _filename;
	FString _extension;

	uint8 _bValid : 1;

public:
	FORCEINLINE FPath()
		: _bValid(false)
	{
	}

	template<class... TArgs, typename TEnableIf<TIsConstructible<FString, TArgs...>::Value>::Type* = nullptr>
	FORCEINLINE FPath(TArgs&&... args) : FPath() { AssignStringPath(FString(Forward<TArgs>(args)...)); }

	FORCEINLINE FPath GetParent() const { return _parent; }
	FORCEINLINE const FString& GetFilename() const { return _filename; }
	FORCEINLINE const FString& GetExtension() const { return _extension; }
	FORCEINLINE FPath GetRelativePath(const FString& relativePath) const { return FPath(FPaths::Combine(_absolute, relativePath)); }
	FORCEINLINE bool IsValid() const { return _bValid; }
	FORCEINLINE const FString& GetPath() const { return _absolute; }

	FORCEINLINE bool operator ==(const FPath& rhs) const { return _absolute == rhs._absolute; }
	FORCEINLINE bool operator !=(const FPath& rhs) const { return _absolute != rhs._absolute; }
	FORCEINLINE FPath operator /(const FString& relativePath) const { return GetRelativePath(relativePath); }
	FORCEINLINE const FString& operator *() const { return GetPath(); }

private:
	void AssignStringPath(FString&& inAbsolutePath)
	{
		_absolute = MoveTemp(inAbsolutePath);
		if (_bValid = NormalizePath(); !_bValid) {
			return;
		}
		FPaths::Split(_absolute, _parent, _filename, _extension);
	}

	bool NormalizePath()
	{
		constexpr TCHAR UniversalDirectorySeparatorString[] = { UniversalDirectorySeparator, 0 };

		// Make Platform-special Directory Separator characters to Universal Directory Separator.
		for (int32 i = 0; i < _absolute.Len(); ++i) {
			if (IsPlatformSpecialSeparator(_absolute[i])) {
				_absolute[i] = UniversalDirectorySeparator;
			}
		}

		// Normalize relative paths.
		TArray<FString> directoryComposeNodes;
		if (_absolute.ParseIntoArray(directoryComposeNodes, UniversalDirectorySeparatorString) == 0) {
			return false;
		}

		TArray<FString> finalNodes;

		for (int32 i = 0; i < directoryComposeNodes.Num(); ++i) {
			EDirectoryComposeNode nodeType = ValidationNode(directoryComposeNodes[i]);
			switch (nodeType) {
			case EDirectoryComposeNode::ParentDirectory:
				if (const int32 depth = finalNodes.Num(); depth == 0) {
					// Invalid.
					return false;
				}
				finalNodes.RemoveAt(finalNodes.Num() - 1);
				break;
			case EDirectoryComposeNode::CurrentDirectory:
				// Do NOT anything.
				break;
			case EDirectoryComposeNode::DirectoryName:
				finalNodes.Emplace(directoryComposeNodes[i]);
				break;
			}
		}

		constexpr TCHAR DirectorySeparatorAsText[] = { UniversalDirectorySeparator, 0 };
		_absolute = TEXT("/") + FString::Join(finalNodes, DirectorySeparatorAsText);

		return true;
	}

	FORCEINLINE static bool IsPlatformSpecialSeparator(TCHAR inChar)
	{
		return inChar == WindowsDirectorySeparator;
	}

	FORCEINLINE static EDirectoryComposeNode ValidationNode(FString& node)
	{
		if (node == TEXT("..")) {
			return EDirectoryComposeNode::ParentDirectory;
		}
		else if (node == TEXT(".")) {
			return EDirectoryComposeNode::CurrentDirectory;
		}
		else {
			// Last dot is ignored.
			int32 len = node.Len();
			int32 removeCnt = 0;
			while (removeCnt < len && node[len - (1 + removeCnt)] == TEXT('.')) {
				++removeCnt;
			}

			if (removeCnt >= len) {
				return EDirectoryComposeNode::Invalid;
			}

			return EDirectoryComposeNode::DirectoryName;
		}
	}
};