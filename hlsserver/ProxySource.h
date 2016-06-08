#pragma once

#include <cassert>
#include <iostream>

class CProxySource{
private:
	SOCKET mSocket;

public:
	CProxySource(SOCKET socket) : mSocket(socket) {}

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