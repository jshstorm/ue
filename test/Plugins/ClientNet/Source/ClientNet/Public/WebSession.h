// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"

#include "Worker.h"
#include "NetPacket.h"

#include "WebSession.generated.h"

DECLARE_DELEGATE_TwoParams(FPacketRecvWsQueue, int32, TSharedWebSocketPacket&)

UCLASS()
class CLIENTNET_API UWebSession : public UObject
{
	GENERATED_BODY()

public:
	void Create(const FString& address, const FString& protocol, TSharedPtr<FPacketRecvWsQueue>& recvQueue);
	void Send(const TSharedWebSocketPacket& packet);
	void Close();

	int32 GetSessionID() { return _sessionID; }

private:
	void OnConnected();
	void OnNetworkError(const FString& error);
	void OnMessage(const FString& message);
	void OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean);

private:
	int32 _sessionID = 0;
	TSharedPtr<class IWebSocket> _socket;
	TOptional<TFunction<void(int32)>> _finalizer;
	TSharedPtr<FPacketRecvWsQueue> _receivedQueue;
};
