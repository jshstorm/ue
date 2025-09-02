// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "WebSession.h"
#include "ClientNet.h"

#include "Runtime/Online/Websockets/Public/IWebSocket.h"
#include "Runtime/Online/Websockets/Public/WebSocketsModule.h"

void UWebSession::Create(const FString& address, const FString& protocol, TSharedPtr<FPacketRecvWsQueue>& recvQueue)
{
	_sessionID = UClientNet::GenerateSessionID();
	_receivedQueue = recvQueue;

	_socket = FWebSocketsModule::Get().CreateWebSocket(address, protocol);
	_socket->OnConnected().AddUObject(this, &UWebSession::OnConnected);
	_socket->OnConnectionError().AddUObject(this, &UWebSession::OnNetworkError);
	_socket->OnMessage().AddUObject(this, &UWebSession::OnMessage);
	_socket->OnClosed().AddUObject(this, &UWebSession::OnClosed);
	_socket->Connect();
}

void UWebSession::OnConnected()
{
	UE_LOG(LogTemp, Log, TEXT("Connected to websocket server. sessionID:[%d]"), _sessionID);
	TSharedWebSocketPacket json = MakeShared<FJsonObject>();
	json->SetBoolField("netevent", true);
	_receivedQueue->ExecuteIfBound(_sessionID, json);
}

void UWebSession::OnNetworkError(const FString& error)
{
	UE_LOG(LogTemp, Log, TEXT("Failed to connect to websocket server with sessionID:[%d] error: \"%s\"."), _sessionID, *error);
	TSharedWebSocketPacket json = MakeShared<FJsonObject>();
	json->SetBoolField("netevent", false);
	_receivedQueue->ExecuteIfBound(_sessionID, json);
}

void UWebSession::OnMessage(const FString& message)
{
	//UE_LOG(LogTemp, Log, TEXT("Received message from websocket sessionID:[%d] message:[%s]"), _sessionID, *message);

	TSharedPtr<FJsonObject> json;
	auto reader = TJsonReaderFactory<TCHAR>::Create(message);
	if (FJsonSerializer::Deserialize(reader, json)) {
		_receivedQueue->ExecuteIfBound(_sessionID, json);
	}
}

void UWebSession::OnClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(LogTemp, Log, TEXT("Connection to websocket server has been closed with status sessionID:[%d] code: \"%d\" and reason: \"%s\"."), _sessionID, StatusCode, *Reason);
	TSharedWebSocketPacket json = MakeShared<FJsonObject>();
	json->SetBoolField("netevent", false);
	_receivedQueue->ExecuteIfBound(_sessionID, json);
}

void UWebSession::Send(const TSharedWebSocketPacket& packet)
{
	if (_socket == nullptr) {
		return;
	}

	FString JsonText;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);
	if (FJsonSerializer::Serialize(packet.ToSharedRef(), Writer) == false) {
		UE_LOG(LogClientNet, Error, TEXT("failed to serialize json"));
		return;
	}
	_socket->Send(JsonText);
}

void UWebSession::Close()
{
	UE_LOG(LogTemp, Log, TEXT("Close websocket sessionID:[%d]"), _sessionID);

	if (_socket) {
		_socket->Close();
	}
}
