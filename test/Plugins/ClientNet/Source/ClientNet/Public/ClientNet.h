// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Engine.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Worker.h"
#include "NetPacket.h"
#include "Session.h"
#include "WebSession.h"
#include "Timer.h"
#include "Security.h"

#include "ClientNet.generated.h"

class SOCKETS_API ISocketSubsystem;
class FHttpModule;
class IHttpRequest;

struct CLIENTNET_API FWebSession;

DECLARE_DELEGATE_OneParam(FOnPacket, TSharedPacket);
DECLARE_DELEGATE_OneParam(FOnWebSocketPacket, TSharedWebSocketPacket);
DEFINE_LOG_CATEGORY_STATIC(LogClientNet, Verbose, All);

#define CLIENTNETMSG_INTERNAL_CONNECTED				(uint16)0x01
#define CLIENTNETMSG_INTERNAL_DISCONNECTED			(uint16)0x02
#define CLIENTNETMSG_INTERNAL_CONNECTION_RETIRED	(uint16)0x03
#define CLIENTNETMSG_INTERNAL_WORLD_LIST_REQUESTED	(uint16)0x04
#define CLIENTNETMSG_INTERNAL_WORLD_LIST_RECEIVED	(uint16)0x05
#define CLIENTNETMSG_INTERNAL_START_ENTER_WORLD		(uint16)0x06
#define CLIENTNETMSG_INTERNAL_RETRY_ENTER_WORLD		(uint16)0x07
#define CLIENTNETMSG_INTERNAL_REPAIR_SESSION		(uint16)0x08
#define CLIENTNETMSG_INTERNAL_SIGN_IN				(uint16)0x09
#define CLIENTNETMSG_INTERNAL_QUEUE_UP				(uint16)0x0A
#define CLIENTNETMSG_INTERNAL_IMMIGRATION			(uint16)0x0B
#define CLIENTNETMSG_INTERNAL_IMMIGRATION_COMPLETED	(uint16)0x0C
#define CLIENTNETMSG_INTERNAL_HANDOVER				(uint16)0x0D
#define CLIENTNETMSG_INTERNAL_SECURITY_ERROR		(uint16)0x0E
#define CLIENTNETMSG_INTERNAL_DISCONTINUE_SESSION	(uint16)0x0F
#define CLIENTNETMSG_INTERNAL_CONTENT_SERVICE_SHUTDOWN	(uint16)0x10

#define REGISTER_PACKET_HANDLER(className, msg) \
	AddPacketHandler(msg, FOnPacket::CreateUObject(this, &className::On##msg))

//DECLARE_DELEGATE_OneParam(FUIMsgEvent, const FString&);
//DECLARE_DELEGATE_OneParam(FResult, const bool&)

enum class AccountType : uint8;
enum class AccountBanState : uint8;

enum class EHTTPRequestVerb : uint8
{
	GET,
	POST,
	PUT,
	DEL
};

enum class EHTTPContentType : uint8
{
	x_www_form_urlencoded_url,
	x_www_form_urlencoded_body,
	json,
	binary
};

UENUM()
enum class ConnectResult : uint8
{
	Success,
	Retired,
	Timeout,
	IntenalError,
};

USTRUCT()
struct CLIENTNET_API FConnector
{
	GENERATED_USTRUCT_BODY()

	FString addr;
	int32 port = 0;
	int32 sessionID = 0;
	TSharedPtr<FSession> session;

private:
	TSharedPtr<TPromise<ConnectResult>> promise = MakeShared<TPromise<ConnectResult>>();

public:
	FConnector()
	{
	}

	FConnector(const FString& addr, int32 port)
	{
		this->addr = addr;
		this->port = port;
	}

	~FConnector()
	{
		if (promise) {
			promise->SetValue(ConnectResult::Timeout);
		}
	}

	FConnector(const FConnector&) = delete;

	using OnConnectionFutured = TFunction<void(TFuture<ConnectResult>)>;
	void Then(OnConnectionFutured&& futured) {
		promise->GetFuture().Then(futured);
	}

	void SetResult(ConnectResult result)
	{
		if (promise) {
			promise->SetValue(result);
		}
		promise.Reset();
	}
};

USTRUCT(BlueprintType)
struct CLIENTNET_API FClientCharacterInfo
{
	GENERATED_USTRUCT_BODY()

	int32 worldID;
	int64 charID;
	FText nickName;
};

USTRUCT(BlueprintType)
struct CLIENTNET_API FClientAccountInfo
{
	GENERATED_USTRUCT_BODY()

	int64 accountID = 0;
	uint8 _platform_code = 0;
	FString _platform_id;
	FString _market;
	FString _os;
	FString _os_version;
	FString _device_model;
	FString _country;
	FString _language;
	uint32 _timeZone = 0;
	
	AccountType _account_type;
	uint8 _account_state;
	uint8 _ban_state;
	uint8 _ban_reason;
	FDateTime _ban_endTime;
	FDateTime _unreg_date;

	FString _provider;
	FString _gamebaseAccessToken;
	
	TMap<int32, FClientCharacterInfo> _characters;
	FClientCharacterInfo& GetCharInfo(int32 worldID) { return _characters.FindOrAdd(worldID); }
	FString ToString() const;
	void Pop(TSharedPacket& packet);
};

using SessionInfo = TPair<int32, TSharedPtr<FSession>>;

UCLASS()
class CLIENTNET_API UClientNet : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static inline ISocketSubsystem* _socketSubsystem = nullptr;
	static inline FHttpModule* _http = nullptr;
	static int32 GenerateSessionID();

private:
	FQueuedThreadPool* _workerThreadPooler = nullptr;
	FAsyncTask<FNetAsyncTask>* _mainWorker = nullptr;

	TQueue<SessionInfo> _connectionIssues;

	FEvent* _shutdownEvent = nullptr;
	FClientNetTimer _timer;

	TMap<uint16, FOnPacket> _handlers;
	FOnPacket _contentHandlers;

	TMap<int32, TSharedPtr<FSession>> _sessions;
	UPROPERTY()
	TMap<int32, UWebSession*> _webSessions;
	TMap<int32, TSharedPtr<FConnector>> _connectings;

	TQueue<TPair<int32, TSharedPacket>> _postPendingQueue;
	TQueue<TSharedPacket> _receivedQueue;
	TQueue<TSharedPacket> _contentMsgQueue;

	TOptional<FString> _addr;
	TOptional<int32> _port;
	TOptional<int32> _serverSession;
	bool _sessionEstablished = false;

	FClientAccountInfo* _accountInfo = nullptr;
	TSet<int32> _activeWorldIDs;

	TUniquePtr<Security> _security;
	
	FString _signinToken;
	FString _sessionToken;

	TQueue<TSharedWebSocketPacket> _receivedWebSocketQueue;;
	int32 _webSocketCurrentID = 0;
	FOnWebSocketPacket _webSocketContentHandler;
	FString _webSocketAddress;
	TSet<int32> _authorizedWebSessions;

	bool _simulateDisconnect = false;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void SetSessionInfo(FClientAccountInfo* info) { _accountInfo = info; }
	const FString& GetSigninToken();

	TSharedPtr<FConnector> StartClientNet(const FString& addr, int32 port);
	TSharedPtr<FConnector> RestartClientNet(float delay = 0, float timeout = 0);
	void Tick(float deltaTime);
	void CloseSession(int32 sessionID);
	void CloseClientNet();

	void StartContentService(int32 worldID);
	void ConfirmService(uint16 waitingNumber);
	void Handover();

	void RegisterPacketHandlers();
	void SetExternalHandler(FOnPacket handler);

	TSharedPtr<FConnector> CreateConnector(const FString& addr, int32 port);

	TSharedPacket AllocPacket(uint16 msgID, int32 sessionID = 0);
	bool SendPacket(TSharedPacket& packet, bool framework = false);

	bool HasConnection();
	const TSet<int32>& GetActiveServerIDs() { return _activeWorldIDs; }

	UFUNCTION()
	void Request_WORLD_LIST();

	//http
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> AllocHTTP(EHTTPRequestVerb verb, EHTTPContentType contentType, float timeOut);
	void PostRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& request);

	//websocket
	void SetWebSocketAddress(const FString& addr);
	bool OpenWebSocket(const FString& address, const FString& protocol);
	bool SendWebSocket(TSharedWebSocketPacket& packet);
	bool CloseWebSocket(int32 id);
	void StartWebSocketService();
	void SetExternalHandler(FOnWebSocketPacket handler);

	void TestDisconnect();

private:
	bool OpenConnection(TSharedPtr<FConnector> connector, float timeout = 0);
	void StartPumpNetIO();

	void OnConnectCompleted(TSharedPtr<FSession> session, ConnectResult result);
	void OnDisconnected(TSharedPtr<FSession> session);

	void SendHeartbeats();
	void ScheduleSessionTokenRenewal();

	bool ConsumePacket(TSharedPacket packet);
	void OnUnhandledMsg(TSharedPacket packet);

	void AddPacketHandler(uint16 msg, const FOnPacket& handler);

	void OnCLIENTNETMSG_INTERNAL_CONNECTED(TSharedPacket);
	void OnCLIENTNETMSG_INTERNAL_DISCONNECTED(TSharedPacket);

	void OnFRAMEWORKMSG_HEART_BEAT_ACK(TSharedPacket);
	void OnFRAMEWORKMSG_SETUP_CORD(TSharedPacket);
	void OnFRAMEWORKMSG_SIGN_IN_ACK(TSharedPacket);
	void OnFRAMEWORKMSG_WORLD_LIST_ACK(TSharedPacket);
	void OnFRAMEWORKMSG_QUEUE_UP_IMMIGRATION_WORLD_ACK(TSharedPacket);
	void OnFRAMEWORKMSG_IMMIGRATION_WORLD(TSharedPacket);
	void OnFRAMEWORKMSG_IMMIGRATION_COMPLETED(TSharedPacket);
	void OnFRAMEWORKMSG_START_ENTER_WORLD_ACK(TSharedPacket);

	void OnFRAMEWORKMSG_DISCONTINUE_SESSION(TSharedPacket);
	void OnFRAMEWORKMSG_RENEWAL_SESSION_ACK(TSharedPacket);
	void OnFRAMEWORKMSG_REPAIR_SESSION_ACK(TSharedPacket);
	void OnFRAMEWORKMSG_HANDOVER_ACK(TSharedPacket);

	void OnFRAMEWORKMSG_SECURITY_EXCHANGE_ACK(TSharedPacket);
	void OnFRAMEWORKMSG_SECURITY_ERROR(TSharedPacket);

	void OnFRAMEWORKMSG_CONTENT_SERVICE_SHUTDOWN(TSharedPacket);

	void Request_SETUP_CORD();
	
	void Request_LOGIN();
	void Request_RENEWAL_SESSION();
	void Request_REPAIR_SESSION();
	void Reqeust_SECURITY_EXCHANGE();
	
	// websocket internal
	bool SendWebSocket(TSharedWebSocketPacket& packet, int32 sessionID);
	void Request_AUTH_WEBSOCKET(int32 sessionID);
	void UpdateWebSocketAuthorization(int32 sessionID, TSharedWebSocketPacket& packet);
	bool ProcessInternalNetEvent(int32 sessionID, TSharedWebSocketPacket& packet);
	void WebSessionUnstabled(int32 sessionID);
};
