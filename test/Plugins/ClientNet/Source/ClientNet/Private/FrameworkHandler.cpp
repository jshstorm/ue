// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "ClientNet.h"
#include "Timer.h"
#include "framework_msg_define.h"
#include "anu_global_def.h"

void UClientNet::RegisterPacketHandlers()
{
	_handlers.Empty();

#define REGISTER_HANDLER(msg) \
	AddPacketHandler(msg, FOnPacket::CreateUObject(this, &UClientNet::On##msg))

	REGISTER_HANDLER(FRAMEWORKMSG_SIGN_IN_ACK);
	REGISTER_HANDLER(FRAMEWORKMSG_SETUP_CORD);
	REGISTER_HANDLER(FRAMEWORKMSG_HEART_BEAT_ACK);
	REGISTER_HANDLER(FRAMEWORKMSG_WORLD_LIST_ACK);
	REGISTER_HANDLER(FRAMEWORKMSG_QUEUE_UP_IMMIGRATION_WORLD_ACK);
	REGISTER_HANDLER(FRAMEWORKMSG_IMMIGRATION_WORLD);
	REGISTER_HANDLER(FRAMEWORKMSG_IMMIGRATION_COMPLETED);

	REGISTER_HANDLER(FRAMEWORKMSG_START_ENTER_WORLD_ACK);
	REGISTER_HANDLER(FRAMEWORKMSG_DISCONTINUE_SESSION);
	REGISTER_HANDLER(FRAMEWORKMSG_RENEWAL_SESSION_ACK);
	REGISTER_HANDLER(FRAMEWORKMSG_REPAIR_SESSION_ACK);
	REGISTER_HANDLER(FRAMEWORKMSG_HANDOVER_ACK);

	REGISTER_HANDLER(FRAMEWORKMSG_SECURITY_EXCHANGE_ACK);
	REGISTER_HANDLER(FRAMEWORKMSG_SECURITY_ERROR);

	REGISTER_HANDLER(CLIENTNETMSG_INTERNAL_CONNECTED);
	REGISTER_HANDLER(CLIENTNETMSG_INTERNAL_DISCONNECTED);

	REGISTER_HANDLER(FRAMEWORKMSG_CONTENT_SERVICE_SHUTDOWN);
}

void UClientNet::AddPacketHandler(uint16 msg, const FOnPacket& handler)
{
	_handlers.Emplace(msg, handler);
}

void UClientNet::SendHeartbeats()
{
	TSharedPacket req = AllocPacket(FRAMEWORKMSG_HEART_BEAT_REQ);
	SendPacket(req, true);
}

void UClientNet::ScheduleSessionTokenRenewal()
{
	_timer.AddTask(TEXT("TokenRenewal"), SESSION_RENEWAL_CHECK_INTERVAL_SEC, RepeatableTaskDelegate::CreateLambda([this] {
		if (_sessionToken.IsEmpty() == false) {
			Request_RENEWAL_SESSION();
		}
		return true;
	}));
}

void UClientNet::OnCLIENTNETMSG_INTERNAL_CONNECTED(TSharedPacket packet)
{
	int32 sessionID = *packet;

	UE_LOG(LogClientNet, Verbose, TEXT("server connected! sessionID [%d]"), sessionID);

	_serverSession = sessionID;

	AsyncTask(ENamedThreads::GameThread, [this] {
		Request_SETUP_CORD();
	});

	const static float HEART_BEAT_INTERVAL_SEC = 10.0f;
	_timer.AddTask(TEXT("HeartBeat"), HEART_BEAT_INTERVAL_SEC, RepeatableTaskDelegate::CreateLambda([this]() -> bool {
		SendHeartbeats();
		return true;
	}));

	TSharedPacket req = AllocPacket(CLIENTNETMSG_INTERNAL_CONNECTED);
	_contentMsgQueue.Enqueue(req);
}

void UClientNet::OnCLIENTNETMSG_INTERNAL_DISCONNECTED(TSharedPacket packet)
{
	int32 sessionID;
	*packet >> sessionID;

	_timer.Remove(TEXT("HeartBeat"));
	
	TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_DISCONNECTED);
	*nfy << sessionID;
	_contentMsgQueue.Enqueue(nfy);
}

void UClientNet::OnFRAMEWORKMSG_SETUP_CORD(TSharedPacket)
{
	Reqeust_SECURITY_EXCHANGE();
}

void UClientNet::OnFRAMEWORKMSG_HEART_BEAT_ACK(TSharedPacket)
{

}

void UClientNet::OnFRAMEWORKMSG_SIGN_IN_ACK(TSharedPacket packet)
{
	if (_security->DecryptPacket(packet) == false) {
		CloseClientNet();
		UE_LOG(LogClientNet, Verbose, TEXT("sign in decrypt packet failed"));
		return;
	}

	uint8 result = *packet;
	
	switch (result) 
	{
		case SIGN_IN_SUCCEEDED:
		{
			_sessionEstablished = true;
			_accountInfo->Pop(packet);

			*packet >> _signinToken;
			*packet >> _sessionToken;
			
			_accountInfo->_characters.Empty();
			
			for (uint8 count = *packet; count != 0; --count) {
				FClientCharacterInfo character;
				character.worldID = (uint32)*packet;
				character.charID = *packet;
				FString nicknameTemp;
				*packet >> nicknameTemp;
				character.nickName = FText::FromString(nicknameTemp);
				
				_accountInfo->_characters.Add(character.worldID, character);
			}

			ScheduleSessionTokenRenewal();

			UE_LOG(LogClientNet, Verbose, TEXT("sign in success signin token[%s] selected worldID[%u] created CharID[%lld]"), *_signinToken);

			Request_WORLD_LIST();

		}
		break;
		case SIGN_IN_ERROR_BACKEND:
		{	
			CloseClientNet();

			int32 code = *packet;
		
			TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SIGN_IN);
			*nfy << result;
			*nfy << code;
			_contentMsgQueue.Enqueue(nfy);
		}
		break;
		case SIGN_IN_INVALID_ACCOUNT_STATE:
		{
			// ban or invalid state
			_accountInfo->Pop(packet);

			TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SIGN_IN);
			*nfy << result;
			_contentMsgQueue.Enqueue(nfy);
		}
		break;
		default:
		{
			TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SIGN_IN);
			*nfy << result;
			_contentMsgQueue.Enqueue(nfy);
		}
		break;
	}
}

void UClientNet::OnFRAMEWORKMSG_WORLD_LIST_ACK(TSharedPacket packet)
{
	_activeWorldIDs.Empty();

	uint16 count = *packet;
	UE_LOG(LogClientNet, Verbose, TEXT("received world lists [%d]"), count);

	for (int16 i = 0; i < count; ++i) {
		uint32 worldID = *packet;
		_activeWorldIDs.Add(worldID);
	}

	TSharedPacket req = AllocPacket(CLIENTNETMSG_INTERNAL_WORLD_LIST_RECEIVED);
	_contentMsgQueue.Enqueue(req);
}

void UClientNet::OnFRAMEWORKMSG_QUEUE_UP_IMMIGRATION_WORLD_ACK(TSharedPacket packet)
{
	uint8 result = *packet;
	if (result != QUEUE_UP_RESULT_OK) {
		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_QUEUE_UP);
		*nfy << result;
		if (result == QUEUE_UP_INTERNAL_SERVER_MAINTENANCE) {
			std::map<std::string, std::string> messages;
			*packet >> messages;
			*nfy << messages;
		}
		_receivedQueue.Enqueue(nfy);
		UE_LOG(LogClientNet, Verbose, TEXT("queue up immigration failed code:%u"), result);
		return;
	}

	uint16 waitingNumber = *packet;
	uint16 processedNumber = *packet;
	*packet >> _sessionToken;
	check(_sessionToken.IsEmpty() == false);

	ScheduleSessionTokenRenewal();

	TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_QUEUE_UP);
	*nfy << result << waitingNumber << processedNumber;
	_receivedQueue.Enqueue(nfy);

	UE_LOG(LogClientNet, Verbose, TEXT("queue up immigration number:%u processed:%u remain:%u"), waitingNumber, processedNumber, (uint16)(waitingNumber - processedNumber));
}

void UClientNet::OnFRAMEWORKMSG_IMMIGRATION_WORLD(TSharedPacket packet)
{
	uint16 processedNumber = *packet;
	
	TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_IMMIGRATION);
	*nfy << processedNumber;
	_receivedQueue.Enqueue(nfy);

	UE_LOG(LogClientNet, Verbose, TEXT("processe immigration number:%u"), processedNumber);
}

void UClientNet::OnFRAMEWORKMSG_IMMIGRATION_COMPLETED(TSharedPacket)
{
	TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_IMMIGRATION_COMPLETED);
	_receivedQueue.Enqueue(nfy);
	UE_LOG(LogClientNet, Verbose, TEXT("immigration completed"));
}

void UClientNet::OnFRAMEWORKMSG_START_ENTER_WORLD_ACK(TSharedPacket packet)
{
	uint8 result = *packet;

	switch (result)
	{
	case ENTER_WORLD_RESULT_OK:
	{
		int64 waitingEntryID = *packet;
		uint64 timestamp = *packet;

		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_START_ENTER_WORLD);
		*nfy << result << waitingEntryID << (uint64)timestamp;
		_contentMsgQueue.Enqueue(nfy);

		StartWebSocketService();
	}
	break;
	default:
	{
		UE_LOG(LogClientNet, Error, TEXT("failed! FRAMEWORKMSG_START_ENTER_WORLD_ACK result [%d]"), result);
	}
	break;
	}
}

void UClientNet::OnFRAMEWORKMSG_DISCONTINUE_SESSION(TSharedPacket packet)
{
	uint8 reason = *packet;

	TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_DISCONTINUE_SESSION);
	*nfy << reason;
	_contentMsgQueue.Enqueue(nfy);

	UE_LOG(LogClientNet, Verbose, TEXT("FRAMEWORKMSG_DISCONTINUE_SESSION reason[%u] serverSession[%u]"), reason, *_serverSession);

	// close session
	CloseSession(*_serverSession);

	_sessionEstablished = false;
	_serverSession = 0;
	_sessionToken.Empty();
	_signinToken.Empty();
	_timer.Remove(TEXT("TokenRenewal"));
}

void UClientNet::OnFRAMEWORKMSG_RENEWAL_SESSION_ACK(TSharedPacket packet)
{
	if (_security->DecryptPacket(packet) == false) {
		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SECURITY_ERROR);
		_contentMsgQueue.Enqueue(nfy);
		return;
	}

	*packet >> _sessionToken;
	
	UE_LOG(LogClientNet, Verbose, TEXT("renewal session [%s]"), *_sessionToken);
}

void UClientNet::OnFRAMEWORKMSG_REPAIR_SESSION_ACK(TSharedPacket packet)
{
	if (_security->DecryptPacket(packet) == false) {
		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SECURITY_ERROR);
		_contentMsgQueue.Enqueue(nfy);
		return;
	}
	
	uint8 error = *packet;
	if (error != RESULT_SUCCEEDED) {
		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_REPAIR_SESSION);
		*nfy << error;
		_contentMsgQueue.Enqueue(nfy);
		return;
	}

	uint8 type = *packet;
	*packet >> _signinToken;
	*packet >> _sessionToken;

	ScheduleSessionTokenRenewal();

	TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_REPAIR_SESSION);
	*nfy << error;
	*nfy << type;
	nfy->WriteBytes(packet->GetRdBuffer(), packet->GetRemainBytesToRead());
	_contentMsgQueue.Enqueue(nfy);

	_sessionEstablished = true;
}

void UClientNet::OnFRAMEWORKMSG_HANDOVER_ACK(TSharedPacket packet)
{
	TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_HANDOVER);
	nfy->WriteBytes(packet->GetRdBuffer(), packet->GetRemainBytesToRead());
	_contentMsgQueue.Enqueue(nfy);
}

void UClientNet::OnFRAMEWORKMSG_SECURITY_EXCHANGE_ACK(TSharedPacket packet)
{
	if (_security->LoadSecurityExchangeContext(packet) == false) {
		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SECURITY_ERROR);
		_contentMsgQueue.Enqueue(nfy);
		return;
	}

	if (_sessionToken.IsEmpty() == false) {
		Request_REPAIR_SESSION();
		return;
	}

	Request_LOGIN();
}

void UClientNet::OnFRAMEWORKMSG_SECURITY_ERROR(TSharedPacket)
{
	UE_LOG(LogClientNet, Warning, TEXT("OnFRAMEWORKMSG_SECURITY_ERROR"));

	TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SECURITY_ERROR);
	_contentMsgQueue.Enqueue(nfy);
}

void UClientNet::OnFRAMEWORKMSG_CONTENT_SERVICE_SHUTDOWN(TSharedPacket)
{
	UE_LOG(LogClientNet, Warning, TEXT("OnFRAMEWORKMSG_CONTENT_SERVICE_SHUTDOWN"));

	TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_CONTENT_SERVICE_SHUTDOWN);
	_contentMsgQueue.Enqueue(nfy);
}

void UClientNet::Request_SETUP_CORD()
{
	TSharedPacket req = AllocPacket(FRAMEWORKMSG_SETUP_CORD);
	*req << "AnuClient";
	SendPacket(req, true);
}

void UClientNet::Request_WORLD_LIST()
{
	UE_LOG(LogClientNet, Verbose, TEXT("request world lists"));

	TSharedPacket req = AllocPacket(FRAMEWORKMSG_WORLD_LIST_REQ);
	SendPacket(req);

	TSharedPacket noti = AllocPacket(CLIENTNETMSG_INTERNAL_WORLD_LIST_REQUESTED);
	_receivedQueue.Enqueue(noti);
}

void UClientNet::Request_LOGIN()
{
	TSharedPacket req = AllocPacket(FRAMEWORKMSG_SIGN_IN_REQ);
	{
		std::string platform_id = TCHAR_TO_UTF8(*_accountInfo->_platform_id);
		*req << platform_id << _accountInfo->_platform_code;
	}
	{
		std::string market{ TCHAR_TO_UTF8(*_accountInfo->_market) };
		std::string os{ TCHAR_TO_UTF8(*_accountInfo->_os) };
		std::string os_version{ TCHAR_TO_UTF8(*_accountInfo->_os_version) };
		std::string device_model{ TCHAR_TO_UTF8(*_accountInfo->_device_model) };

		std::string country{ TCHAR_TO_UTF8(*_accountInfo->_country) };
		std::string lang{ TCHAR_TO_UTF8(*_accountInfo->_language) };
		std::string accessToken{ TCHAR_TO_UTF8(*_accountInfo->_gamebaseAccessToken) };

		*req << market << os << os_version << device_model << country << lang << _accountInfo->_timeZone << accessToken;
	}

	if (_security->EncryptPacket(req) == false) {
		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SECURITY_ERROR);
		_contentMsgQueue.Enqueue(nfy);
		return;
	}

	SendPacket(req, true);
}

void UClientNet::Request_RENEWAL_SESSION()
{
	TSharedPacket req = AllocPacket(FRAMEWORKMSG_RENEWAL_SESSION_REQ);
	*req << _sessionToken;

	if (_security->EncryptPacket(req) == false) {
		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SECURITY_ERROR);
		_contentMsgQueue.Enqueue(nfy);
		return;
	}

	SendPacket(req);
}

void UClientNet::Request_REPAIR_SESSION()
{
	TSharedPacket req = AllocPacket(FRAMEWORKMSG_REPAIR_SESSION_REQ);
	*req << _sessionToken << _signinToken;

	if (_security->EncryptPacket(req) == false) {
		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SECURITY_ERROR);
		_contentMsgQueue.Enqueue(nfy);
		return;
	}

	SendPacket(req, true);
}

void UClientNet::Reqeust_SECURITY_EXCHANGE()
{
	UE_LOG(LogClientNet, Verbose, TEXT("security exchange"));

	_security->GenerateKey();

	TSharedPacket req = AllocPacket(FRAMEWORKMSG_SECURITY_EXCHANGE_REQ);
	_security->BuildSecurityExchangePubKey(req);
	SendPacket(req, true);
}
