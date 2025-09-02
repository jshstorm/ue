// Copyright 2021 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "NetPacket.h"
#include <vector>
#include <optional>

#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
THIRD_PARTY_INCLUDES_END
#undef UI

struct SecurityContext {
	std::optional<std::vector<uint8>> userKey;
	std::vector<uint8> dec_iv;
	std::vector<uint8> enc_iv;

	bool isActivated() {
		if (userKey.has_value()) {
			return true;
		}

		return false;
	}
};

class CLIENTNET_API Security
{
public:
	Security();
	~Security();

	bool GenerateKey();
	void Clear();
	void BuildSecurityExchangePubKey(TSharedPacket& packet);
	bool LoadSecurityExchangeContext(TSharedPacket& packet);

	bool EncryptPacket(TSharedPacket& packet);
	bool DecryptPacket(TSharedPacket& packet);

private:
	bool DecryptRSA(TSharedPacket& packet);

	RSA* _rsa = nullptr;
	BIGNUM* _bigNum = nullptr;
	std::optional<SecurityContext> _securityContext;
	std::vector<uint8> _pubKey;
};
