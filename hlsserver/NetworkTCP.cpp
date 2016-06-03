#include "StdAfx.h"

#include "NetworkTCP.h"

#pragma comment (lib, "ws2_32.lib")

CNetworkTCP::CNetworkTCP(unsigned short port) 
	: mPort(port), 
	mSocketLocal(NULL),
	mLiving(false),
	mCallbackHandler(NULL),
	mOnRequest(NULL),
	mThread(NULL)
{
}

CNetworkTCP::~CNetworkTCP(void)
{
	if(mLiving)
		Close();
}

DWORD
CNetworkTCP::Run(void *param){
	CNetworkTCP *thiz = (CNetworkTCP *)param;
	thiz->LoopBody();

	return 0;
}

void
CNetworkTCP::LoopBody(){

	mSocketLocal = socket(AF_INET, SOCK_STREAM, 0);

	mAddrLocal.sin_family = AF_INET;
	mAddrLocal.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	mAddrLocal.sin_port = htons(mPort);

	int rc = 0;
	rc = bind(mSocketLocal,(SOCKADDR *)&mAddrLocal, sizeof(SOCKADDR));
	rc = listen(mSocketLocal, 1000);

	mLiving = true;

	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);
	char recvBuf[1024] = {0};
	while(mLiving){
		SOCKET sock = accept(mSocketLocal, (SOCKADDR *)&addrClient, &len);

		memset(recvBuf, 0, sizeof(recvBuf));
		int rc = recv(sock, recvBuf, sizeof(recvBuf), 0);

		if(rc > 0 && mOnRequest){
			mOnRequest(sock, recvBuf, rc, mCallbackHandler);
		}

		//closesocket(sock);
	}
}

int CNetworkTCP::Open(CallbackOnRequest callback, void* handler){
	mOnRequest = callback;
	mCallbackHandler = handler;

	//mThread = (HANDLE)_beginthreadex(NULL, 0, Run, this, 0, NULL);
	mThread = CreateThread(NULL, 0, Run, this, 0, NULL);

	return 0;
}

int CNetworkTCP::Close(){
	mLiving = false;

	shutdown(mSocketLocal, SD_BOTH);
	closesocket(mSocketLocal);

	WaitForSingleObject(mThread, INFINITE);

	return 0;
}