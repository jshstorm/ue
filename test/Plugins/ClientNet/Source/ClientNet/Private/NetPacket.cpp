// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "NetPacket.h"
#include "GenericPlatform/GenericPlatformFile.h"

FNetPacket::FNetPacket()
{
	_buffer.resize(MAX_MSGBUF_SIZE, 0);
}

FNetPacket::FNetPacket(uint16 id)
{
	Reset();
	SetMsgID(id);
}

FNetPacket::~FNetPacket()
{
	_buffer.clear();
}

void FNetPacket::Resize(uint16 size)
{
	size += MSG_HEADER_SIZE;

	uint16 block = (size / MAX_MSGBUF_SIZE) + 1;
	size = MAX_MSGBUF_SIZE * block;

	_buffer.resize(size);
}

void FNetPacket::WriteBuff(IFileHandle* fileHandle)
{
	fileHandle->Write(&_buffer.at(0), GetWrPos());
}

bool FNetPacket::ReadBuff(IFileHandle* fileHandle)
{
	if (false == fileHandle->Read(&_buffer.at(0), MSG_HEADER_SIZE)) {
		return false;
	}

	uint16 bodySize{ GetBodySize() };
	if (bodySize == 0) {
		return true;
	}

	_wrCur = bodySize + MSG_HEADER_SIZE;
	_buffer.resize(_wrCur);
	return fileHandle->Read(&_buffer.at(MSG_HEADER_SIZE), bodySize);
}

bool FNetPacket::SetRdPos(uint16 pos)
{
	if (MSG_HEADER_SIZE + pos > GetCapacity()) {
		return false;
	}

	_rdCur = (MSG_HEADER_SIZE + pos);
	return true;
}

bool FNetPacket::SetWrPos(uint16 pos)
{
	if (MSG_HEADER_SIZE + pos > GetCapacity()) {
		return false;
	}

	_wrCur = (MSG_HEADER_SIZE + pos);
	return true;
}

void FNetPacket::Reset()
{
	_rdCur = _wrCur = MSG_HEADER_SIZE;

	_buffer.clear();
	_buffer.resize(MAX_MSGBUF_SIZE, 0);

	ClearHeader();
}

void FNetPacket::SetDataSizeAtHeader(uint16 size)
{
	::memcpy(&_buffer.at(MSG_OFFSET_SIZE), (uint8*)&size, sizeof(uint16));
	//*((uint16*)&_buffer[MSG_OFFSET_SIZE]) = size;
}

bool FNetPacket::IsReceivingPacketCompleted()
{
	auto written = GetWrPos();
	return (written >= MSG_HEADER_SIZE && written == GetPacketSize());
}

void FNetPacket::ReadBytesReverse(void* buf, uint16 size)
{
	if (size > _wrCur) {
		return;
	}

	_wrCur = (uint16)(_wrCur - size);

	uint8* src = GetWrBuffer();
	::memcpy(buf, src, size);

	SetDataSizeAtHeader(_wrCur - MSG_HEADER_SIZE);
}

void FNetPacket::ReadBytes(void* buf, uint16 size)
{
	if (size <= 0 || (_rdCur + size) > _wrCur) {
		return;
	}

	uint8* src = GetRdBuffer();
	::memcpy(buf, src, size);

	_rdCur = (uint16)(_rdCur + size);
}

void FNetPacket::WriteBytes(const void* buf, uint16 size)
{
	uint16 availableBufSize = GetAvailableBufSize();

	if (availableBufSize < size) {
		uint16 chunkCount = (size - availableBufSize) / MAX_MSGBUF_SIZE + 1;
		uint16 required = chunkCount * MAX_MSGBUF_SIZE;
		size_t resizing = GetCapacity() + required;
		if (resizing > MAX_MESSIVE_BUF_SIZE) {
			return;
		}

		_buffer.resize(resizing);
	}

	uint8* cur = GetWrBuffer();
	::memcpy(cur, buf, size);

	_wrCur = (uint16)(_wrCur + size);

	SetDataSizeAtHeader(_wrCur - MSG_HEADER_SIZE);
}

uint16 FNetPacket::CopyMsg(FNetPacket* src)
{
	uint8* srcBuffer = nullptr;
	uint16 dataSizeToCopy = 0;

	dataSizeToCopy = src->GetBodySize();
	srcBuffer = src->GetBufferAt(MSG_HEADER_SIZE);
	//src->ForceStopRead();

	uint16 availableBufSize = GetAvailableBufSize();
	if (availableBufSize < dataSizeToCopy) {
		uint16 bufferCount = dataSizeToCopy / MAX_MSGBUF_SIZE + 1;
		if (GetCapacity() + bufferCount * MAX_MSGBUF_SIZE > MAX_MESSIVE_BUF_SIZE) {
			return ERR_OUT_OF_MAX_SIZE;
		}
	}

	WriteBytes(srcBuffer, dataSizeToCopy);
	return dataSizeToCopy;
}

void FNetPacket::RecordTimestamp()
{
	_timestamp = FDateTime::Now().ToUnixTimestamp();
}
