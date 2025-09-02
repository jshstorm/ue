// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"

#include "Worker.h"
#include "NetPacket.h"

class FSocket;
DECLARE_DELEGATE_OneParam(FPacketRecvQueue, TSharedPacket&)

#define SAFE_DELETE(x)	{ delete (x); (x) = nullptr; }

struct FSession
{
	FSession(int32 sessionID, FPacketRecvQueue& recvQueue);
	~FSession();

	int32 _sessionID = 0;
	FString _host;
	uint32 _ip = 0;
	int32 _port = 0;

private:
	FSocket* _socket = nullptr;
	TSharedPacket _buildingPacket;
	FPacketRecvQueue _receivedQueue;
	TQueue<TSharedPacket> _sendingQueue;

public:
	bool Create(const FString& host, int32 port);
	void Close();

	bool Connect();
	bool PrepareConnect();
	bool PumpNetIO();
	void SendPacket(const TSharedPacket& packet);
	int32 GetSessionID() { return _sessionID; }

private:
	bool Receiving();
	bool Sending();
};
