#pragma once

class CProxySource{
private:
	SOCKET mSocket;

public:
	CProxySource(SOCKET socket) : mSocket(socket) {}

	int RecvData(void *pFramesBuffer, int MAX_FRAME_BUFFER_SIZE, int size[], int *count){
		*count = 0;
		return 0;
	}
};