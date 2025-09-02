// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "Session.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

#include "ClientNet.h"

FSession::FSession(int32 sessionID, FPacketRecvQueue& recvQueue)
{
	_sessionID = sessionID;
	_receivedQueue = recvQueue;
}

FSession::~FSession()
{
	if (_socket) {
		Close();
		_socket = nullptr;
	}

	_buildingPacket.Reset();
	_sendingQueue.Empty();
}

bool FSession::Create(const FString& host, int32 port)
{
	_host = host;
	_port = port;
	_ip = 0;

	_socket = UClientNet::_socketSubsystem->CreateSocket(NAME_Stream, TEXT("ClientNet.TCPSocket"), false);
	if (_socket == nullptr) {
		return false;
	}

	bool failed = !_socket->SetReuseAddr(false) || !_socket->SetLinger(false, 0) || !_socket->SetRecvErr();
	if (failed) {
		UClientNet::_socketSubsystem->DestroySocket(_socket);
		return false;
	}

	static const int32 MAX_SOCKET_BUFF_SIZE = 1024 * 124;

	int32 newSize = 0;
	_socket->SetReceiveBufferSize(MAX_SOCKET_BUFF_SIZE, newSize);
	_socket->SetSendBufferSize(MAX_SOCKET_BUFF_SIZE, newSize);
	
	_socket->SetNoDelay(true);

	return true;
}

bool FSession::Connect()
{
	TSharedRef<FInternetAddr> addr = UClientNet::_socketSubsystem->CreateInternetAddr();
	addr->SetIp(_ip);
	addr->SetPort(_port);

	if (_socket->Connect(*addr) == false) {
		return false;
	}

	_socket->SetNonBlocking(true);
	return true;
}

void FSession::Close()
{
	if (_socket == nullptr) {
		return;
	}

	_socket->Close();

	UClientNet::_socketSubsystem->DestroySocket(_socket);
	_socket = nullptr;
}

bool FSession::PrepareConnect()
{
	if (_socket == nullptr || _host.IsEmpty()) {
		return false;
	}

	FAddressInfoResult result = UClientNet::_socketSubsystem->GetAddressInfo(*_host, nullptr, EAddressInfoFlags::Default, NAME_None);
	if (result.Results.Num() <= 0) {
		return false;
	}

	for (auto& data : result.Results) {
		uint32 ip = 0;
		data.Address->GetIp(ip);
		if (ip != 0 && _ip == 0) {
			_ip = ip;
			break;
		}
	}
	
	return (bool)_ip != 0;
};

void FSession::SendPacket(const TSharedPacket& packet)
{
	_sendingQueue.Enqueue(packet);
}

bool FSession::PumpNetIO()
{
	if (_socket == nullptr) {
		return false;
	}

	if (Receiving() == false) {
		return false;
	}

	if (Sending() == false) {
		return false;
	}

	return true;
}

bool FSession::Receiving()
{
	while (true)
	{
		int32 bytesRead = 0;
		if (_buildingPacket.IsValid() == false) {
			_buildingPacket = MakeShared<FNetPacket>();
			_buildingPacket->SetSessionID(_sessionID);
		}

		uint16 remains = 0;
		if (_buildingPacket->GetWrPos() < MSG_HEADER_SIZE) {

			remains = MSG_HEADER_SIZE - _buildingPacket->GetWrPos();

		} else {
			uint16 packetSize = _buildingPacket->GetPacketSize();
			if (packetSize > _buildingPacket->GetCapacity()) {
				_buildingPacket->Resize(packetSize);
			}
			remains = packetSize - _buildingPacket->GetWrPos();
		}

		check(remains > 0 && remains <= MAX_MESSIVE_BUF_SIZE);

		if (_socket->Recv(_buildingPacket->GetWrBuffer(), remains, bytesRead) == false) {
			_buildingPacket.Reset();
			return false;
		}
		
		if (bytesRead == 0) {
			break;
		}

		_buildingPacket->IncWrPos(bytesRead);

		if (_buildingPacket->IsReceivingPacketCompleted()) {
			_buildingPacket->SetRdPos(0);
			_receivedQueue.ExecuteIfBound(_buildingPacket);
			_buildingPacket.Reset();
		}
	}

	return true;
}

bool FSession::Sending()
{
	TSharedPacket packet;
	while (_sendingQueue.Dequeue(packet)) {

		int32 remainBytes = packet->GetPacketSize();
		uint8* buffer = packet->GetPacketBuffer();

		while (remainBytes > 0) {

			int32 sent = 0;
			if (_socket->Send(buffer, remainBytes, sent) == false) {
				ESocketErrors err = UClientNet::_socketSubsystem->GetLastErrorCode();
				packet.Reset();
				return false;
			}

			check(remainBytes >= sent);
			remainBytes -= sent;
			buffer += sent;
		}
	}

	return true;
}
