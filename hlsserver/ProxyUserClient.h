#ifndef _PROXY_USER_CLIENT_H_
#define _PROXY_USER_CLIENT_H_

#include <string>
using namespace std;

#define PROXY_USER_CLIENT_STATUS_OK			0
#define PROXY_USER_CLIENT_STATUS_ERROR		1

#ifdef DLL_EXPORT
	#define   DLLCLASS_API   __declspec(dllexport)
#else
	#define   DLLCLASS_API   __declspec(dllimport) 
#endif

#ifdef __cplusplus
extern "C" {
#endif

class DLLCLASS_API ProxyDevice
{
public:
	ProxyDevice();
	~ProxyDevice();

	int RecvData(char* buf, int inLength, int* rcvSize, int* frameCnt);
	int SendAudioData(char* buf, int sendSize);

	void SetData(void* data);

private:
	void* m_devInfo;
	int m_timeOutCnt;
};


class DLLCLASS_API ProxyUserClient
{
public:
	ProxyUserClient();
	~ProxyUserClient();
	
public:
	int Login(string& jsStrIn, string& jsStrOut);
	int Logout(string& jsStrIn, string& jsStrOut);
	int QueryDeviceList(string& jsStrIn, string& jsStrOut);
	//int RequestVideoPlay(string& jsStrIn, string& jsStrOut);
	int RequestVideoPlay(string& jsStrIn, string& jsStrOut, ProxyDevice** proxyDev);
	int OperatePtz(string& jsStrIn, string& jsStrOut);
	
	int IsOnline(string& jsStrIn, string& jsStrOut);
	int GetDeviceList(string& jsStrIn, string& jsStrOut);
	
	int RecvData(char* buf, int inLength, int* rcvSize, int* frameCnt);
	//int SendAudioData(char* buf, int sendSize);

	//clip
	int GetClipList(string& jsStrIn, string& jsStrOut);
	int QueryClipList(string& jsStrIn, string& jsStrOut);
	int RequestClipPlay(string& jsStrIn, string& jsStrOut);
	//int RequestClipPlay();
	
	//ProxyUserClient* CreateShadowUser();

	
private:
	void* m_userInfo;
	void* m_nStreamSocket;

	int m_playFlag;
	int m_timeOutCnt;

	//int SetInfoData(void* data);
};

#ifdef __cplusplus
}
#endif


#endif