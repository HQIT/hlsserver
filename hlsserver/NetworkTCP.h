#pragma once

#include <WinSock2.h>

#include <string>

#include "json/json.h"

class CNetworkTCP
{
public:
	typedef int (*CallbackOnRequest)(SOCKET sock, void*);

	CNetworkTCP(unsigned short port);
	virtual ~CNetworkTCP(void);

	virtual int Open(CallbackOnRequest, void*);
	virtual int Close();

protected:
	virtual void LoopBody();

private:
	static DWORD WINAPI Run(void *param);

private:
	unsigned short mPort;

	SOCKET mSocketLocal;
	SOCKADDR_IN mAddrLocal;

	bool mLiving;
	HANDLE mThread;

	CallbackOnRequest mOnRequest;
	void* mCallbackHandler;
};

class CResponser{
public:
	explicit CResponser(SOCKET conn) : mSocket(conn){}

	int Response(const char* data, const unsigned int size){
		return send(mSocket, data, size, 0);
	}

	int Response(const wchar_t *data, const unsigned int size){
		return send(mSocket, (const char *)data, size * sizeof(wchar_t), 0);
	}

	int Response(std::string &data){
		return Response(data.c_str(), data.length());
	}

	int Response(const Json::Value &json){
		Json::FastWriter w;
		std::string &result = w.write(json);
		return this->Response(result);
	}

	int Response(std::string &status, std::string &message){
		Json::Value j;
		j["Status"] = status;
		j["ErrMsg"] = message;

		return this->Response(j);
	}

private:
	SOCKET mSocket;
};