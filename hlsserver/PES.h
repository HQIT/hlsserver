#pragma once

#include "ESDataType.h"

class PES
{
public:

	const static unsigned char SIMPLE_HEADER_LENGTH = 14;

	explicit PES(unsigned long payloadLength)
		: mData(NULL), mType(ESDT_INVALID), mPayloadLength(0)
	{
		mPayloadLength = payloadLength;
		//mData = new unsigned char[payloadLength + SIMPLE_HEADER_LENGTH];
		mData = new unsigned char[payloadLength + SIMPLE_HEADER_LENGTH + 6/*AUD*/];
	}

	virtual ~PES(void){
		if(mData){
			delete[] mData;
			mData = NULL;
		}
	}

public:
	//@data [out]: need 14 bytes at least
	//void MakeHeader(unsigned char* data, Program::Stream* stream);
	void Fill(unsigned char* data, ESDataType type, bool keyFrame = false);
//	void SetPTS(unsigned long long ts);
	void SetPTS(unsigned long ts);
	unsigned char *Data(){ return mData; }
	unsigned long Size(){ return mPayloadLength + SIMPLE_HEADER_LENGTH + 6/*AUD*/; }
	ESDataType Type(){ return mType; }

private:
	unsigned long mPayloadLength;
	unsigned char *mData;
	ESDataType mType;
};

