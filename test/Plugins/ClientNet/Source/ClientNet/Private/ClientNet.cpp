// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "ClientNet.h"

#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "Runtime/Sockets/Public/SocketSubsystem.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "Runtime/Online/Websockets/Public/IWebSocket.h"
#include "Runtime/Online/Websockets/Public/WebSocketsModule.h"
#include "Runtime/Core/Public/Async/Async.h"

#include "framework_msg_define.h"
#include "anu_msg_string.h"

#include "WebSession.h"

/*
#ifdef _MSC_VER
#pragma warning(disable : 4503)
#define _SCL_SECURE_NO_WARNINGS

#define ASIO_STANDALONE
#define FALSE 0
#define TRUE 1
#endif

#if PLATFORM_WINDOWS
   //#define WIN32_LEAN_AND_MEAN
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#endif

#include <asio.hpp>

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformAtomics.h"
#endif
//*/

FString FClientAccountInfo::ToString() const
{
	return FString::Printf(TEXT("[platform_id:%s] [platform_code:%d] [region:%s] [language:%s] [timezone:%d] [accType:%d]"),
		*_platform_id, _platform_code, *_country, *_language, _timeZone, _account_type);
}

void FClientAccountInfo::Pop(TSharedPacket& packet)
{
	accountID = *packet;
	_account_type = (AccountType)((uint8)(*packet));
	_account_state = (uint8)(*packet);
	_ban_state = (uint8)(*packet);
	_ban_reason = (uint8)(*packet);
	UE_LOG(LogClientNet, Verbose, TEXT("accInfo type[%d] banState[%d] banReason[%d]"), _account_type, _ban_state, _ban_reason);
	
	int64 remainBanEndTime = *packet;
	int64 remainUnregDate = *packet;
	if (remainBanEndTime != 0) {
		_ban_endTime =  FDateTime::Now() + FTimespan::FromSeconds(remainBanEndTime);
		UE_LOG(LogClientNet, Verbose, TEXT("accInfo banEndTime[%s]"), *_ban_endTime.ToString());
	}

	if (remainUnregDate != 0) {
		_unreg_date = FDateTime::UtcNow() + FTimespan{ remainUnregDate * ETimespan::TicksPerSecond };
		UE_LOG(LogClientNet, Verbose, TEXT("accInfo unregDate[%s]"), *_unreg_date.ToString());
	}
}

int32 UClientNet::GenerateSessionID()
{
	static std::atomic<int32> poolSessionID = 0;
	return ++poolSessionID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UClientNet::Initialize(FSubsystemCollectionBase& Collection)
{
	UClientNet::_socketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	UClientNet::_http = &FHttpModule::Get();

	_security = MakeUnique<Security>();

	constexpr uint32 CLIENTNET_WORKER_THREAD_COUNT = 2;

	_workerThreadPooler = FQueuedThreadPool::Allocate();
	_workerThreadPooler->Create(CLIENTNET_WORKER_THREAD_COUNT);
}

void UClientNet::Deinitialize()
{
	UE_LOG(LogClientNet, Verbose, TEXT("start clientnet deinitialize"));

	CloseClientNet();

	if (_workerThreadPooler) {
		_workerThreadPooler->Destroy();
		_workerThreadPooler = nullptr;
	}

	UE_LOG(LogClientNet, Verbose, TEXT("clientnet deinitialized"));
}

const FString& UClientNet::GetSigninToken()
{
	return _signinToken;
}

TSharedPtr<FConnector> UClientNet::StartClientNet(const FString& addr, int32 port)
{
	UE_LOG(LogClientNet, Verbose, TEXT("starting clientnet"));

	CloseClientNet();

	_shutdownEvent = FPlatformProcess::GetSynchEventFromPool(true);

	_mainWorker = new FAsyncTask<FNetAsyncTask>([this] {
		StartPumpNetIO();
	});

	_mainWorker->StartBackgroundTask(_workerThreadPooler);

	if (0 == port) {
		UE_LOG(LogClientNet, Error, TEXT("port is not set"));
		return nullptr;
	}

	_addr = addr;
	_port = port;

	auto connector = CreateConnector(addr, port);
	if (OpenConnection(connector) == false) {
		return nullptr;
	}

	return connector;
}

void UClientNet::CloseClientNet()
{
	UE_LOG(LogClientNet, Verbose, TEXT("starting close clientnet"));

	_timer.RemoveAll();

	if (_shutdownEvent) {
		_shutdownEvent->Trigger();
	}

	if (_mainWorker) {
		_mainWorker->EnsureCompletion();
	}
	SAFE_DELETE(_mainWorker);

	if (_shutdownEvent) {
		FPlatformProcess::ReturnSynchEventToPool(_shutdownEvent);
		_shutdownEvent = nullptr;
	}

	for (auto& session : _sessions) {
		session.Value->Close();
	}
	_sessions.Empty();

	for (auto session : _webSessions) {
		(session.Value)->Close();
	}
	_webSessions.Empty();

	_serverSession.Reset();
	_webSocketCurrentID = 0;

	_connectings.Empty();
	_connectionIssues.Empty();

	_postPendingQueue.Empty();
	_receivedQueue.Empty();
	_contentMsgQueue.Empty();

	_sessionEstablished = false;
	_authorizedWebSessions.Empty();

	_signinToken.Empty();
	_sessionToken.Empty();

	UE_LOG(LogClientNet, Verbose, TEXT("clientnet closed"));
}

void UClientNet::StartContentService(int32 worldID)
{
	UE_LOG(LogClientNet, Verbose, TEXT("starting content service [%d] token[%s]"), worldID, *_signinToken);
	
	TSharedPacket req = AllocPacket(FRAMEWORKMSG_QUEUE_UP_IMMIGRATION_WORLD_REQ);
	
	*req << _signinToken << worldID;
	if (_security->EncryptPacket(req) == false) {
		TSharedPacket nfy = AllocPacket(CLIENTNETMSG_INTERNAL_SECURITY_ERROR);
		_contentMsgQueue.Enqueue(nfy);
		return;
	}

	SendPacket(req);
}

void UClientNet::ConfirmService(uint16 waitingNumber)
{
	UE_LOG(LogClientNet, Verbose, TEXT("enter service waitingNumber[%d]"), waitingNumber);

	TSharedPacket req = AllocPacket(FRAMEWORKMSG_START_ENTER_WORLD_REQ);
	*req << waitingNumber;
	SendPacket(req);
}

void UClientNet::Handover()
{
	TSharedPacket req = AllocPacket(FRAMEWORKMSG_HANDOVER_REQ);
	SendPacket(req, true);
}

TSharedPtr<FConnector> UClientNet::CreateConnector(const FString& addr, int32 port)
{
	FPacketRecvQueue recvQueue;
	recvQueue.BindLambda([this](TSharedPacket& packet) {
		_receivedQueue.Enqueue(packet);
	});

	auto sessionID = UClientNet::GenerateSessionID();

	auto& connector = _connectings.FindOrAdd(sessionID);
	connector = MakeShared<FConnector>(addr, port);
	connector->sessionID = sessionID;
	connector->session = MakeShared<FSession>(sessionID, recvQueue);
	return connector;
}

bool UClientNet::OpenConnection(TSharedPtr<FConnector> connector, float timeout)
{
	if (UClientNet::_socketSubsystem == nullptr) {
		UE_LOG(LogClientNet, Error, TEXT("clientnet not initiated"));
		_connectings.Remove(connector->sessionID);
		return false;
	}

	auto task = new FAutoDeleteAsyncTask<FNetAsyncTask>([this, connector, timeout] {
		
		static const size_t RETIRE_COUNT = 10;
		static const uint32 RETRY_INTERVAL = 3 * 1000;
		
		ConnectResult result = ConnectResult::Retired;
		auto sessionID = connector->sessionID;
		auto session = connector->session;
		
		auto started = FPlatformTime::Seconds();
		size_t trying = 0;
		while (trying < RETIRE_COUNT) {

			if (timeout != 0 && FPlatformTime::Seconds() - started >= timeout) {
				result = ConnectResult::Timeout;
				break;
			}

			uint8* ip = (uint8*)&session->_ip;

			if (session->Create(connector->addr, connector->port) == false) {
				UE_LOG(LogClientNet, Warning, TEXT("failed to create session"));
				goto WAITING_RETRY;
			}
			
			UE_LOG(LogClientNet, Verbose, TEXT("connecting task started session_id[%d]"), sessionID);
			if (!session->PrepareConnect()) {
				UE_LOG(LogClientNet, Error, TEXT("failed to init session network environment session_id[%d]"), sessionID);
				goto WAITING_RETRY;
			}

			UE_LOG(LogClientNet, Verbose, TEXT("trying to connect [%d.%d.%d.%d : %d]"), ip[3], ip[2], ip[1], ip[0], session->_port);
			if (session->Connect()) {
				result = ConnectResult::Success;
				break;
			}

		WAITING_RETRY:
			// close 
			session->Close();
			
			++trying;
			UE_LOG(LogClientNet, Warning, TEXT("failed to connect.. retry in %d msec"), RETRY_INTERVAL);
			if (_shutdownEvent == nullptr || _shutdownEvent->Wait(RETRY_INTERVAL)) {
				UE_LOG(LogClientNet, Verbose, TEXT("shutting down event triggered, closing connection task"));
				break;
			}
		}

		AsyncTask(ENamedThreads::GameThread, [this, result, session] {
			OnConnectCompleted(session, result);
		});
	});

	task->StartBackgroundTask(_workerThreadPooler);
	return true;
}

void UClientNet::CloseSession(int32 sessionID)
{
	if (sessionID <= 0) {
		UE_LOG(LogClientNet, Error, TEXT("invalid session_id[%d]"), sessionID);
		return;
	}

	UE_LOG(LogClientNet, Verbose, TEXT("close session_id[%d] intentionally"), sessionID);
	SessionInfo info(sessionID, nullptr);
	_connectionIssues.Enqueue(info);
}

void UClientNet::StartPumpNetIO()
{
	UE_LOG(LogClientNet, Verbose, TEXT("starting pumping netio task"));
	constexpr uint32 WAIT_TIMEOUT_MSEC = 1;

	while (_shutdownEvent->Wait(WAIT_TIMEOUT_MSEC) == false)
	{
		SessionInfo info;
		while (_connectionIssues.Dequeue(info))
		{
			int32 sessionID = info.Key;
			check(sessionID != 0);
			TSharedPtr<FSession> session = info.Value;

			if (session.IsValid()) {
				check(session->_sessionID == sessionID);
				_sessions.Add(sessionID, session);
			} else {
				session = _sessions.FindRef(sessionID);
				if (session.IsValid() == false) {
					UE_LOG(LogClientNet, Error, TEXT("not a valid session_id[%d] trying to disconnect"), sessionID);
					continue;
				}
				session->Close();
				_sessions.Remove(sessionID);
			}
		}

		TPair<int32, TSharedPacket> request;
		while (_postPendingQueue.Dequeue(request)) {
			int32 sessionID = request.Key;
			if (TSharedPtr<FSession>* session = _sessions.Find(sessionID)) {
				(*session)->SendPacket(request.Value);
			}
		}

		TMap<int32, TSharedPtr<FSession>>::TIterator it = _sessions.CreateIterator();
		while (it) {
			TMap<int32, TSharedPtr<FSession>>::TIterator current = it;
			++it;

			TSharedPtr<FSession> session = current.Value();
			if (session->PumpNetIO() == false || _simulateDisconnect) {
				OnDisconnected(session);
				current.RemoveCurrent();
				_simulateDisconnect = false;
			}
		}
	}

	UE_LOG(LogClientNet, Verbose, TEXT("pumping netio task closed"));
}

void UClientNet::OnConnectCompleted(TSharedPtr<FSession> session, ConnectResult result)
{
	auto sessionID = session->_sessionID;

	if (auto connector = _connectings.Find(sessionID)) {
		(*connector)->SetResult(result);
		_connectings.Remove(sessionID);
	}

	uint16 msg = (result == ConnectResult::Success) ? CLIENTNETMSG_INTERNAL_CONNECTED : CLIENTNETMSG_INTERNAL_CONNECTION_RETIRED;

	TSharedPacket noti = AllocPacket(msg, sessionID);
	*noti << sessionID;
	_receivedQueue.Enqueue(noti);

	if (msg == CLIENTNETMSG_INTERNAL_CONNECTED) {
		SessionInfo info(sessionID, session);
		_connectionIssues.Enqueue(info);
	}
}

void UClientNet::OnDisconnected(TSharedPtr<FSession> session)
{
	auto sessionID = session->_sessionID;

	TSharedPacket noti = AllocPacket(CLIENTNETMSG_INTERNAL_DISCONNECTED, sessionID);
	*noti << sessionID;
	_receivedQueue.Enqueue(noti);

	_sessionEstablished = false;
	_timer.Remove(TEXT("TokenRenewal"));

	UE_LOG(LogClientNet, Error, TEXT("OnDisconnected sessionID:%u code:%u"), sessionID, UClientNet::_socketSubsystem->GetLastErrorCode());
}

TSharedPtr<FConnector> UClientNet::RestartClientNet(float delay, float timeout)
{
	if (_addr.IsSet() == false || _addr->IsEmpty() || _port.IsSet() == false || *_port == 0) {
		UE_LOG(LogClientNet, Error, TEXT("invalid entry server information."));
		return nullptr;
	}

	auto connector = CreateConnector(*_addr, *_port);
	_timer.AddTask(TEXT("RestartClientNet"), delay, ScheduleTaskDelegate::CreateLambda([this, connector, timeout] {
		OpenConnection(connector, timeout);
	}));
	return connector;
}

TSharedPacket UClientNet::AllocPacket(uint16 msgID, int32 sessionID)
{
	TSharedPacket packet = MakeShared<FNetPacket>(msgID);
	packet->SetSessionID(sessionID);
	return packet;
}

bool UClientNet::SendPacket(TSharedPacket& packet, bool framework)
{
#if !UE_BUILD_SHIPPING
	auto itr = MsgHelper::ProtocolMap.find(packet->GetMsgID());
	if (itr != MsgHelper::ProtocolMap.end()) {
		UE_LOG(LogClientNet, Verbose, TEXT("[REQ] %s"), WCHAR_TO_TCHAR(itr->second.c_str()));
	}
#endif
	if (_serverSession.IsSet() == false) {
		UE_LOG(LogClientNet, Verbose, TEXT("not have session msg_id[0x%x]"), packet->GetMsgID());
		return false;
	}

	if (_sessionEstablished == false && framework == false) {
		UE_LOG(LogClientNet, Verbose, TEXT("session not established skip send msg_id[0x%x]"), packet->GetMsgID());
		return false;
	}

	TPair<int32, TSharedPacket> send(*_serverSession, packet);
	return _postPendingQueue.Enqueue(send);
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UClientNet::AllocHTTP(EHTTPRequestVerb verb, EHTTPContentType contentType, float timeOut)
{
	_http->SetHttpTimeout(timeOut);

	auto request = UClientNet::_http->CreateRequest();
	switch (verb)
	{
	case EHTTPRequestVerb::GET:
		request->SetVerb(TEXT("GET"));
		break;

	case EHTTPRequestVerb::POST:
		request->SetVerb(TEXT("POST"));
		break;

	case EHTTPRequestVerb::PUT:
		request->SetVerb(TEXT("PUT"));
		break;

	case EHTTPRequestVerb::DEL:
		request->SetVerb(TEXT("DELETE"));
		break;

	default:
		break;
	}

	switch (contentType)
	{
	case EHTTPContentType::x_www_form_urlencoded_url:
		request->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
		break;
	case EHTTPContentType::x_www_form_urlencoded_body:
		request->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
		break;
	case EHTTPContentType::binary:
		break;
	case EHTTPContentType::json:
		request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		break;
	default:
		break;
	}

	return request;
}

void UClientNet::PostRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& request)
{
	request->ProcessRequest();
}

void UClientNet::SetWebSocketAddress(const FString& addr)
{
	_webSocketAddress = addr;
}

bool UClientNet::OpenWebSocket(const FString& address, const FString& protocol)
{
	UE_LOG(LogClientNet, Verbose, TEXT("open websocket address:[%s] protocol:[%s]"), *address, *protocol);

	TSharedPtr<FPacketRecvWsQueue> recvQueue = MakeShared<FPacketRecvWsQueue>();
	recvQueue->BindWeakLambda(this, [this](int32 sessionID, TSharedWebSocketPacket& packet) {

		if (_webSocketCurrentID == 0 && _sessionToken.IsEmpty()) {
			return;
		}

		//check internal session event
		if (ProcessInternalNetEvent(sessionID, packet)) {
			return;
		}

		//check authorized
		if (_authorizedWebSessions.Contains(sessionID) == false) {
			UpdateWebSocketAuthorization(sessionID, packet);
			return;
		}

		_receivedWebSocketQueue.Enqueue(packet);
	});

	auto session = NewObject<UWebSession>(this);
	session->Create(address, protocol, recvQueue);
	_webSessions.Add(session->GetSessionID(), session);
	return true;
}

bool UClientNet::ProcessInternalNetEvent(int32 sessionID, TSharedWebSocketPacket& packet)
{
	if (packet->HasField("netevent") == false) {
		return false;
	}

	auto on = packet->GetBoolField("netevent");
	if (on) {
		Request_AUTH_WEBSOCKET(sessionID);
	}
	else {
		WebSessionUnstabled(sessionID);
	}
	return true;
}

void UClientNet::UpdateWebSocketAuthorization(int32 sessionID, TSharedWebSocketPacket& packet)
{
	const TSharedWebSocketPacket* response = nullptr;
	if (packet->TryGetObjectField("response", response) == false) {
		return;
	}

	const FName cmd{ (*response)->GetStringField("cmd") };
	if (cmd.IsEqual("auth") == false) {
		return;
	}

	int32 result = (*response)->GetNumberField("resultCode");
	if (result != 1) {
		return;
	}

	_authorizedWebSessions.Add(sessionID);
	_webSocketCurrentID = sessionID;

	TSharedWebSocketPacket internalEevent = MakeShared<FJsonObject>();
	internalEevent->SetStringField("event", "ActiveSession");
	_receivedWebSocketQueue.Enqueue(internalEevent);

	UE_LOG(LogClientNet, Verbose, TEXT("websocket auth completed session:[%d]"), sessionID);
}

void UClientNet::WebSessionUnstabled(int32 sessionID)
{
	UE_LOG(LogClientNet, Verbose, TEXT("unstabled web session! session:[%d]"), sessionID);

	_webSessions.Remove(sessionID);
	_authorizedWebSessions.Remove(sessionID);

	if (_webSocketCurrentID == sessionID) {
		_webSocketCurrentID = 0;

		TSharedWebSocketPacket internalEevent = MakeShared<FJsonObject>();
		internalEevent->SetStringField("event", "DectiveSession");
		_receivedWebSocketQueue.Enqueue(internalEevent);
	}

	constexpr float RETRY = 5.f;
	UE_LOG(LogClientNet, Verbose, TEXT("retry to restore web session in [%.1f] sec."), RETRY);
	_timer.AddTask(TEXT("RetryWebSocket"), RETRY, ScheduleTaskDelegate::CreateWeakLambda(this, [this] {
		StartWebSocketService();
	}));
}

bool UClientNet::SendWebSocket(TSharedWebSocketPacket& packet)
{
	if (_webSocketCurrentID == 0) {
		UE_LOG(LogClientNet, Error, TEXT("websocket auth not completed, sending packet will be dropped."));
		return false;
	}

	return SendWebSocket(packet, _webSocketCurrentID);
}

bool UClientNet::CloseWebSocket(int32 id)
{
	if (auto session = _webSessions.Find(id)) {
		(*session)->Close();
		return true;
	}
	return false;
}

void UClientNet::StartWebSocketService()
{	
	if (_webSocketAddress.IsEmpty()) {
		return;
	}

	auto protocol = _webSocketAddress.Find("wss") >= 0 ? TEXT("wss") : TEXT("ws");
	OpenWebSocket(_webSocketAddress, protocol);
}

bool UClientNet::HasConnection()
{
	return _serverSession.IsSet() && *_serverSession != 0;
}

void UClientNet::SetExternalHandler(FOnPacket handler)
{
	_contentHandlers = handler;
}

void  UClientNet::SetExternalHandler(FOnWebSocketPacket handler)
 {
	_webSocketContentHandler = handler;
 }

void UClientNet::TestDisconnect()
{
	_simulateDisconnect = true;
}

void UClientNet::Tick(float deltaTime)
{
	_timer.Tick(deltaTime);

	TSharedPacket packet;
	while (_receivedQueue.Dequeue(packet)) {
		ConsumePacket(packet);
	}

	while (_contentMsgQueue.Dequeue(packet)) {
		_contentHandlers.ExecuteIfBound(packet);
	}

	TSharedWebSocketPacket webSocketPacket;
	while (_receivedWebSocketQueue.Dequeue(webSocketPacket)) {
		_webSocketContentHandler.ExecuteIfBound(webSocketPacket);
	}
}

inline std::set<uint16> ignoreProtocols =
{
	MOVE_SNAPSHOT_PATCH_NFY,
	DEBUG_LOCATION_ACK,
	DEBUG_VOXEL_ACK,
};

bool UClientNet::ConsumePacket(TSharedPacket packet)
{
	uint16 msgID = packet->GetMsgID();

#if !UE_BUILD_SHIPPING
	auto itr = MsgHelper::ProtocolMap.find(msgID);
	if (itr != MsgHelper::ProtocolMap.end() && ignoreProtocols.find(msgID) == ignoreProtocols.end()) {
		UE_LOG(LogClientNet, Verbose, TEXT("[REC] %s"), WCHAR_TO_TCHAR(itr->second.c_str()));
	}
#endif

	FOnPacket* handler = _handlers.Find(msgID);
	if (handler == nullptr) {
		OnUnhandledMsg(packet);
		return false;
	}

	handler->Execute(packet);
	return true;
}

void UClientNet::OnUnhandledMsg(TSharedPacket packet)
{
	_contentMsgQueue.Enqueue(packet);
}

bool UClientNet::SendWebSocket(TSharedWebSocketPacket& packet, int32 sessionID)
{
	auto webSession = _webSessions.FindRef(sessionID);
	if (webSession == nullptr) {
		UE_LOG(LogClientNet, Verbose, TEXT("websocket send session not found session:[%d]"), sessionID);
		return false;
	}

	webSession->Send(packet);
	return true;
}

void UClientNet::Request_AUTH_WEBSOCKET(int32 sessionID)
{
	if (_sessionToken.IsEmpty()) {
		UE_LOG(LogClientNet, Fatal, TEXT("websocket auth account sessionToken empty session:[%d]"), sessionID);
		return;
	}

	TSharedWebSocketPacket command = MakeShared<FJsonObject>();

	TSharedWebSocketPacket auth = MakeShared<FJsonObject>();
	auth->SetStringField("token",  _sessionToken);
	command->SetObjectField("auth", auth);

	UE_LOG(LogClientNet, Verbose, TEXT("websocket request auth session:[%d]"), sessionID);
	
	SendWebSocket(command, sessionID);
}
