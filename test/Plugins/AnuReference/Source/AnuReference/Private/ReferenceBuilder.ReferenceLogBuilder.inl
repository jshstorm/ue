// Copyright 2017 CLOVERGAMES Co., Ltd. All right reserved.

#include "Misc/MessageDialog.h"

namespace ReferenceLogBuilder::Internal
{
	FString LogLevelToString(FReferenceLogBuilder::ELevel level)
	{
		switch (level) {
		case FReferenceLogBuilder::ELevel::Error:
			return TEXT("Error");
		case FReferenceLogBuilder::ELevel::Warning:
			return TEXT("Warning");
		default:
			return TEXT("MissingLevel");
		}
	}
}

TArray<FString> FReferenceLogBuilder::_errors;
TArray<FString> FReferenceLogBuilder::_warnings;

void FReferenceLogBuilder::Cleanup()
{
	_errors.Empty();
	_warnings.Empty();
}

void FReferenceLogBuilder::Report()
{
#if DO_CHECK
	// If contains one more error, print logs with open new dialog.
	if (ContainsError()) {
		FString composedDlgTitle = TEXT("레퍼런스 빌드 중 오류 발견!");
		FText dlgTitle = FText::FromString(composedDlgTitle);

		FString composedDlgMsg;
		if (ContainsWarning()) {
			composedDlgMsg = FString::Printf(TEXT("%d개의 오류 및 %d개의 경고가 발생했습니다.\n자세한 내용은 출력 로그를 확인하세요."), _errors.Num(), _warnings.Num());
		}
		else {
			composedDlgMsg = FString::Printf(TEXT("%d개의 오류가 발생했습니다.\n자세한 내용은 출력 로그를 확인하세요."), _errors.Num());
		}
		FText dlgMsg = FText::FromString(composedDlgMsg);

		// Open report dialog.
		EAppReturnType::Type _ = FMessageDialog::Open(EAppMsgType::Ok, dlgMsg, &dlgTitle);
	}
#endif

	// Otherwise, just print logs to output log console.
	for (FString& log : _warnings) {
		UE_LOG(LogReference, Warning, TEXT("%s"), *log);
	}

	for (FString& log : _errors) {
		UE_LOG(LogReference, Error, TEXT("%s"), *log);
	}
}

bool FReferenceLogBuilder::ContainsError()
{
	return _errors.Num() != 0;
}

bool FReferenceLogBuilder::ContainsWarning()
{
	return _warnings.Num() != 0;
}

bool FReferenceLogBuilder::CleanSucceeded()
{
	return !ContainsError() && !ContainsWarning();
}

void FReferenceLogBuilder::InternalAddLog(ELevel level, const FString& expr_t, const FString& composed)
{
	TArray<FString>* myContainer = nullptr;
	switch (level) {
	default:
	case ELevel::Error:
		myContainer = &_errors;
		break;
	case ELevel::Warning:
		myContainer = &_warnings;
		break;
	}

	FString composedText = FString::Printf(TEXT("(expression: %s) %s"), *expr_t, *composed);
	myContainer->Emplace(MoveTemp(composedText));
}