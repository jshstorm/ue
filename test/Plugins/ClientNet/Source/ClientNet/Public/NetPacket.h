// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include <vector>
#include <set>
#include <list>
#include <map>
#include <string>
#include <bitset>

#include "framework_msg_define.h"
#include "anu_msg_define.h"

class CLIENTNET_API FNetPacket
{
public:
	FNetPacket();
	FNetPacket(uint16 id);
	~FNetPacket();

private:
	std::vector<uint8> _buffer;
	uint16 _rdCur = 0;
	uint16 _wrCur = 0;

	int32 _sessionID = 0;
	int64 _timestamp = 0;

public:
	void Reset();
	void Resize(uint16 size);

	void WriteBuff(IFileHandle* fileHandle);
	bool ReadBuff(IFileHandle* fileHandle);

	////////////////////////////////////////////////////
	// header relatives
	void ClearHeader() { ::memset(&_buffer[0], 0, MSG_HEADER_SIZE); }

	void SetMsgID(uint16 wMsgID) { *((uint16*)&_buffer[MSG_OFFSET_ID]) = wMsgID; }
	uint16 GetMsgID() const { return *((uint16*)&_buffer[MSG_OFFSET_ID]); }

	uint16 GetBodySize() const { return *((uint16*)&_buffer[MSG_OFFSET_SIZE]); }
	uint16 GetPacketSize() const { return GetBodySize() + MSG_HEADER_SIZE; }
	uint16 GetAvailableBufSize() { return (uint16)(_buffer.size() - _wrCur); }
	void SetDataSizeAtHeader(uint16 size);

	bool IsReceivingPacketCompleted();

	////////////////////////////////////////////////////
	void SetSessionID(int32 sessionID) { _sessionID = sessionID; }
	int32 GetSessionID() { return _sessionID; }

	void RecordTimestamp();
	int64 GetTimestamp() { return _timestamp; }

	bool IsFromRemote() { return _sessionID != 0; }
	bool IsFromLocal() { return _sessionID == 0; }

	////////////////////////////////////////////////////
	// buffer offset
	bool SetRdPos(uint16 pos);
	bool SetWrPos(uint16 pos);

	uint16 GetRdPos() { return _rdCur; }
	uint16 GetWrPos() { return _wrCur; }

	void IncRdPos(uint16 pos) { _rdCur += pos; }
	void IncWrPos(uint16 pos) { _wrCur += pos; }

	void DecRdPos(uint16 pos) { _rdCur -= pos; }
	void DecWrPos(uint16 pos) { _wrCur -= pos; }

	bool AllDataRead() { return (_rdCur == _wrCur); }
	uint16 GetRemainBytesToRead() { return (_wrCur - _rdCur); }
	void ForceStopRead() { _rdCur = _wrCur; }

	////////////////////////////////////////////////////
	// buffer pointer
	void ReadBytes(void* buf, uint16 size);
	void WriteBytes(const void* buf, uint16 size);
	void ReadBytesReverse(void* buf, uint16 size);

	size_t GetCapacity() { return _buffer.size(); }
	uint8* GetRdBuffer() { return &_buffer.at(_rdCur); }
	uint8* GetWrBuffer() { return &_buffer.at(_wrCur); }

	uint8* GetBufferAt(uint16 offset)
	{
		return &_buffer.at(offset);
	}

	uint8* GetPacketBuffer()
	{
		return &_buffer.at(0);
	}

	uint16 CopyMsg(FNetPacket* src);

public:
	template <class T>
	void ReadBytesReverseEx(T& arg)
	{
		uint16 size = sizeof(T);
		if (size > (GetWrPos() - MSG_HEADER_SIZE)) {
			check(false);
			return;
		}

		DecWrPos(size);
		::memcpy((uint8*)&arg, GetWrBuffer(), size);

		SetDataSizeAtHeader(GetWrPos() - MSG_HEADER_SIZE);
	}

	template <class T>
	void ReadBytesEx(T& arg)
	{
		uint16 size = sizeof(T);
		if ((GetRdPos() + size) > GetWrPos()) {
			check(false);
			return;
		}

		::memcpy((uint8*)&arg, GetRdBuffer(), size);

		IncRdPos(size);
	}

	template <class T>
	void Overwrite(T arg, long pos)
	{
		if ((pos + sizeof(T)) > GetWrPos()) {
			check(false);
			return;
		}

		::memcpy(GetBufferAt(pos), (uint8*)&arg, sizeof(T));
	}

	template <class T>
	FNetPacket& operator << (const T& arg)
	{
		WriteBytes((void*)&arg, sizeof(T));
		return *this;
	}

	template <class T>
	FNetPacket& operator >> (T& arg)
	{
		ReadBytesEx(arg);
		return *this;
	}

	FNetPacket& operator << (const char* rhs)
	{
		uint16 size = (!rhs) ? 0 : (uint16)(::strlen(rhs));
		WriteBytes(&size, sizeof(uint16));

		if (size < 1) {
			return *this;
		}

		WriteBytes((void*)rhs, size);
		return *this;
	}

	FNetPacket& operator << (const wchar_t* rhs)
	{
		uint16 size = (!rhs) ? 0 : (uint16)(::wcslen(rhs));
		WriteBytes(&size, sizeof(uint16));

		if (size < 1) {
			return *this;
		}

		WriteBytes((void*)rhs, size * sizeof(wchar_t));
		return *this;
	}

	//--------------------------------------------------------------
	FNetPacket& operator << (const std::wstring& wstr)
	{
		return *this << wstr.c_str();
	}

	FNetPacket& operator >> (std::wstring& str)
	{
		uint16 size = 0;
		ReadBytes(&size, sizeof(uint16));
		if (size > 0) {
			if (size > GetRemainBytesToRead()) {
				size = GetRemainBytesToRead();
			}

			str.resize(size);
			ReadBytes((void*)str.c_str(), size * sizeof(wchar_t));
		}
		else {
			str.clear();
		}
		return *this;
	}
	//--------------------------------------------------------------

	FNetPacket& operator << (const std::string& str)
	{
		return *this << str.c_str();
	}

	FNetPacket& operator >> (std::string& str)
	{
		uint16 size = 0;
		ReadBytes(&size, sizeof(uint16));
		if (size > 0) {
			if (size > GetRemainBytesToRead()) {
				size = GetRemainBytesToRead();
			}

			str.resize(size);
			ReadBytes((void*)str.c_str(), size);
		}
		else {
			str.clear();
		}
		return *this;
	}
	//--------------------------------------------------------------
	FNetPacket& operator << (const FString& str)
	{
		std::string string{ TCHAR_TO_UTF8(*str) };
		return *this << string.c_str();
	}

	FNetPacket& operator >> (FString& str)
	{
		std::string temp;
		*this >> temp;
		str = UTF8_TO_TCHAR(temp.c_str());
		return *this;
	}

	//--------------------------------------------------------------
	FNetPacket& operator << (const FName& str)
	{
		std::string string{ TCHAR_TO_UTF8(*str.ToString()) };
		return *this << string.c_str();
	}

	FNetPacket& operator >> (FName& str)
	{
		std::string temp;
		*this >> temp;
		str = UTF8_TO_TCHAR(temp.c_str());
		return *this;
	}

	//--------------------------------------------------------------
	template <typename T>
	FNetPacket& operator << (const std::vector<T>& vector)
	{
		uint16 count = vector.size();
		*this << count;
		for (uint16 idx = 0; idx < count; ++idx) {
			*this << vector[idx];
		}
		return *this;
	}

	template <typename T>
	FNetPacket& operator >> (std::vector<T>& vector)
	{
		uint16 count = 0;
		*this >> count;

		vector.resize(count);
		for (uint16 i = 0; i < count; ++i) {
			*this >> vector[i];
		}
		return *this;
	}

	template <typename T>
	FNetPacket& operator << (const TArray<T>& vector)
	{
		uint16 count = vector.Num();
		*this << count;
		for (uint16 idx = 0; idx < count; ++idx) {
			*this << vector[idx];
		}
		return *this;
	}

	template <typename T>
	FNetPacket& operator >> (TArray<T>& vector)
	{
		uint16 count = 0;
		*this >> count;

		vector.SetNum(count);
		for (uint16 i = 0; i < count; ++i) {
			*this >> vector[i];
		}
		return *this;
	}

	template <typename T>
	FNetPacket& operator << (const std::set<T>& set)
	{
		uint16 count = set.size();
		*this << count;

		auto it = set.begin();
		for (uint16 i = 0; i < count; ++i, ++it) {
			*this << (*it);
		}
		return *this;
	}

	template <typename T>
	FNetPacket& operator >> (std::set<T>& set)
	{
		uint16 count = 0;
		*this >> count;
		for (uint16 idx = 0; idx < count; ++idx) {
			T value;
			*this >> value;
			set.insert(value);
		}
		return *this;
	}

	template <typename T1, typename T2>
	FNetPacket& operator << (const std::map<T1, T2>& map)
	{
		uint16 count = (uint16)map.size();
		*this << count;
		auto it = map.begin();
		for (uint16 i = 0; i < count; ++i, ++it) {
			*this << it->first;
			*this << it->second;
		}
		return *this;
	}

	template <typename T1, typename T2>
	FNetPacket& operator >> (std::map<T1, T2>& map)
	{
		uint16 count = 0;
		*this >> count;
		for (uint16 idx = 0; idx < count; ++idx) {
			T1 key;
			*this >> key;
			T2& val = map[key];
			*this >> val;
		}
		return *this;
	}

	template <typename T>
	FNetPacket& operator << (const std::list<T>& list)
	{
		uint16 count = list.size();
		*this << count;
		auto it = list.begin();
		for (uint16 i = 0; i < count; ++i, ++it) {
			*this << *it;
		}
		return *this;
	}

	template <typename T>
	FNetPacket& operator >> (std::list<T>& list)
	{
		uint16 count = 0;
		*this >> count;
		for (uint16 idx = 0; idx < count; ++idx) {
			T value;
			*this >> value;
			list.push_back(value);
		}
		return *this;
	}

	template <size_t Bits, typename Type>
	void UnpackMask(TFunction<void(size_t)>&& unpacker) {
		check(Bits > 0 && Bits / 8 <= sizeof(Type));
		std::bitset<Bits> mask((Type)*this);
		for (size_t i = 0; i < mask.size(); ++i) {
			if (mask.test(i)) {
				unpacker(i);
			}
		}
	}

#define _GET_TYPED_VALUE(type)              \
	uint16 length = sizeof(type);           \
    if ((GetRdPos() + length) > GetWrPos()) { \
		return 0;                           \
	}                                       \
	type temp;                              \
	::memcpy((uint8*)&temp, GetRdBuffer(), length); \
	IncRdPos(length);                       \
	return temp;

#define _GET_STRING_VALUE(type)              \
	uint16 length = *this;                   \
    if ((GetRdPos() + length) > GetWrPos()) { \
		return 0;                           \
	}                                       \
	std::string temp;                       \
	temp.assign(GetRdBuffer(), GetRdBuffer() + length);\
	IncRdPos(length);                       \
	return temp;

	operator bool() { _GET_TYPED_VALUE(bool); }
	operator int8() { _GET_TYPED_VALUE(int8); }
	operator uint8() { _GET_TYPED_VALUE(uint8); }
	operator int16() { _GET_TYPED_VALUE(int16); }
	operator uint16() { _GET_TYPED_VALUE(uint16); }
	operator int32() { _GET_TYPED_VALUE(int32); }
	operator uint32() { _GET_TYPED_VALUE(uint32); }
	operator int64() { _GET_TYPED_VALUE(int64); }
	operator uint64() { _GET_TYPED_VALUE(uint64); }
	operator float() { _GET_TYPED_VALUE(float); }
	operator double() { _GET_TYPED_VALUE(double); }
	operator std::string() { _GET_STRING_VALUE(std::string); }

	template<class TEnum, typename TEnableIf<std::is_enum_v<TEnum>, void*>::Type = nullptr>
	operator TEnum() { return (TEnum)(std::underlying_type_t<TEnum>) * this; }
};

using TSharedPacket = TSharedPtr<FNetPacket, ESPMode::Fast>;
using TSharedWebSocketPacket = TSharedPtr<FJsonObject, ESPMode::Fast>;
