#pragma once

#include <cassert>
#include <iostream>

#if 1
struct AudioFormatPacketData
{
	WORD        channels;          /* number of channels (i.e. mono, stereo...) */
	DWORD       samplesPerSec;     /* sample rate */
	DWORD       avgBytesPerSec;    /* for buffer estimation */
	WORD        bitsPerSample;     /* number of bits per sample of mono data */
	WORD        blockSize;        /* block size of data */
};

struct VideoFormatPacketData
{
	WORD	width;
	WORD	height;
	WORD	encodeType;	//1: h.264, 2: h.265
	WORD	fps;
	DWORD	bitrate;	//kbps
	int		gopSize;
};
#endif

class CProxySource{
private:
	SOCKET mSocket;

public:
	CProxySource(SOCKET socket) : mSocket(socket) {}

	//@version: 协议版本
	//@return: 0-OK;其他-错误
	int RecvData(void *pFrameBuffer, unsigned int MAX_FRAME_BUFFER_SIZE, unsigned int &frameLength, unsigned int &version, unsigned int &type){
		///read 4 byte - frame length
		unsigned char head[6] = { 0 };
		int wanted = sizeof(head);
		do {
			int len = recv(mSocket, (char *) head, sizeof(head), 0);
			if (len < 0) break;
			wanted -= len;
		} while (wanted > 0);

		frameLength = (head[3] << 24 | head[2] << 16 | head[1] << 8 | head[0]) - 2;
		
		std::cout << "frameLength: " << frameLength << "\n";

		if (frameLength > MAX_FRAME_BUFFER_SIZE) {
			return -2;
			///throw std::runtime_error("bad frame length, overfollow");
		}

		version = head[4], type = head[5];
		wanted = frameLength;

		assert(pFrameBuffer != NULL);
		
		do {
			int len = recv(mSocket, (char *) pFrameBuffer + frameLength - wanted, wanted, 0);
			if (len < 0) break;
			wanted -= len;
		} while (wanted > 0);

		return 0;
	}
};