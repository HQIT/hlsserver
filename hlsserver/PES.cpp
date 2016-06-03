#include "Program.h"
#include "PES.h"

#include "ESDataType.h"

#include <stdint.h>

void PES::Fill(unsigned char* data, ESDataType type, bool keyFrame)
{
	mType = type;

	memcpy(mData + SIMPLE_HEADER_LENGTH + 6/*AUD*/, data, mPayloadLength);

	*mData = 0x00; *(mData + 1) = 0x00; *(mData + 2) = 0x01;	//Packet start code prefix

	//Stream id, only one audio and video, index == 0
	if(type == ESDT_VIDEO){
		*(mData + 3) = 0xE0 + (0 & 0x0F);		//0xE0-0xEF
	}else{	//AUDIO
		*(mData + 3) = 0xC0 + (0 & 0x1F);		//0xC0-0xDF
	}

	//bytes 4-5 is packet length
	unsigned long length = (mPayloadLength + 6/*AUD needs 6 bytes*/) + 8;
	if(length > 0xFFFF){ 
		length = 0;
	}
	*(mData + 4) = (unsigned char)(length >> 8);
	*(mData + 5) = (unsigned char)(length >> 0);

	//Optional PES header
	*(mData + 6) = 0x80;
	*(mData + 7) = 0x80;		//PTS only

	*(mData + 8) = 5;	//PTS needs 5 bytes

	//insert AUD 6bytes
	*(mData + 14) = 0;
	*(mData + 15) = 0;
	*(mData + 16) = 0;
	*(mData + 17) = 1;
	*(mData + 18) = 0x09;
	*(mData + 19) = keyFrame ? 0x10 : 0x30;	//0x10-I frame	//0x30-P frame
}

void PES::SetPTS(unsigned long ts){
#pragma warning(disable: 4244)
	// Fill in the PES PTS
	*(mData + 9) = 0x20 | ( ts >> 29 ) | 0x01;
	*(mData + 10) = ts >> 22;
	*(mData + 11) = (ts >> 14) | 0x01;
	*(mData + 12) = ts >> 7;
	*(mData + 13) = (ts << 1) | 0x01;
}