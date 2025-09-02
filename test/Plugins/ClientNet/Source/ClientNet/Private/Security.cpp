// Copyright 2021 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "Security.h"


Security::Security()
{
	
}

Security::~Security()
{
	Clear();
}

bool Security::GenerateKey()
{
	Clear();

	_bigNum = BN_new();
	if (BN_set_word(_bigNum, RSA_F4) != 1) {
		Clear();
		return false;
	}

	_rsa = RSA_new();
	if (RSA_generate_key_ex(_rsa, 2048, _bigNum, nullptr) != 1) {
		Clear();
		return false;
	}

	BIO* bio = BIO_new(BIO_s_mem());
	if (PEM_write_bio_RSAPublicKey(bio, _rsa) != 1) {
		Clear();
		BIO_free_all(bio);
		return false;
	}

	auto len = BIO_pending(bio);
	_pubKey.resize(len + 1, 0);
	BIO_read(bio, &_pubKey[0], len);
	BIO_free_all(bio);

	return true;
}

void Security::Clear()
{	
	_pubKey.clear();

	if (_bigNum != nullptr) {
		BN_free(_bigNum);
		_bigNum = nullptr;
	}

	if (_rsa != nullptr) {
		::RSA_free(_rsa);
		_rsa = nullptr;
	}

	_securityContext.reset();
}

void Security::BuildSecurityExchangePubKey(TSharedPacket& packet)
{
	*packet << _pubKey;
}

bool Security::LoadSecurityExchangeContext(TSharedPacket& packet)
{
	SecurityContext context;
	
	if (packet->GetRemainBytesToRead() == 0) {
		_securityContext.emplace(context);
		return true;
	}

	if (DecryptRSA(packet) == false) {
		return false;
	}
	
	std::vector<uint8> userKey;
	std::vector<uint8> vi;
	*packet >> userKey;
	*packet >> vi;

	context.userKey.emplace(userKey);
	context.dec_iv = vi;
	context.enc_iv = vi;

	_securityContext.emplace(context);

	return true;
}

bool Security::EncryptPacket(TSharedPacket& packet)
{
	if (_securityContext.has_value() == false) {
		check(false);
		return false;
	}

	if (_securityContext->isActivated() == false) {
		return true;
	}

	std::vector<uint8>& userKey = _securityContext->userKey.value();
	
	AES_KEY encKey;
	int len = (int)userKey.size() * 8;
	int rt = AES_set_encrypt_key(&userKey[0], len, &encKey);
	if (rt < 0) {
		return false;
	};

	uint16 dataSize = packet->GetBodySize();
	
	std::vector<uint8> buffer;
	buffer.resize(dataSize);
	int return_iv_length = 0;

	AES_cfb128_encrypt(packet->GetBufferAt(MSG_HEADER_SIZE), &buffer[0], buffer.size(), &encKey, &_securityContext->enc_iv[0], &return_iv_length, AES_ENCRYPT);
	
	packet->SetWrPos(0);
	packet->WriteBytes(&buffer[0], dataSize);

	return true;
}

bool Security::DecryptPacket(TSharedPacket& packet)
{
	if (_securityContext.has_value() == false) {
		check(false);
		return false;
	}

	if (_securityContext->isActivated() == false) {
		return true;
	}

	std::vector<uint8>& userKey = _securityContext->userKey.value();
	
	AES_KEY encKey;
	int len = (int)userKey.size() * 8;
	int rt = AES_set_encrypt_key(&userKey[0], len, &encKey);
	if (rt < 0) {
		return false;
	};

	uint16 dataSize = packet->GetBodySize();

	std::vector<uint8> buffer;
	buffer.resize(dataSize);
	int return_iv_length = 0;

	AES_cfb128_encrypt(packet->GetBufferAt(MSG_HEADER_SIZE), &buffer[0], buffer.size(), &encKey, &_securityContext->dec_iv[0], &return_iv_length, AES_DECRYPT);
	
	packet->SetWrPos(0);
	packet->WriteBytes(&buffer[0], dataSize);

	return true;
}

bool Security::DecryptRSA(TSharedPacket& packet)
{
	if (_rsa == nullptr) {
		check(false);
		return false;
	}

	uint16 size = packet->GetBodySize();

	std::vector<uint8> buffer;
	buffer.resize(size * 2);

	int length = RSA_private_decrypt(size, (const unsigned char*)packet->GetRdBuffer(), &buffer[0], _rsa, RSA_PKCS1_OAEP_PADDING);
	if (length == -1) {
		return false;
	}

	packet->SetWrPos(0);
	packet->WriteBytes(&buffer[0], length);

	return true;
}
