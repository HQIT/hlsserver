#pragma once

#include <string>
#include <map>
#include <queue>

#include "CriSection.h"

#include "ProxyUserClient.h"
#include "Streaming.h"
using lelink::CStreaming;

class CSessionTask{
public:
	SOCKET mSocket;
	std::string mMethod;
	std::string mBody;
	std::string mAccessToken;	//keep session id is better than CUserSession *mpSession

	static CSessionTask *New(SOCKET, const std::string &method, const std::string &body, std::string &);

	virtual ~CSessionTask();

	const unsigned int Index(){ return mIndex; }

private:
	unsigned int mIndex;
	static unsigned int TaskIndex;

	CSessionTask(SOCKET, const std::string &method, const std::string &body, std::string &);
};

typedef std::queue<CSessionTask *> SessionTaskQueue;

class CUserSession{
protected:
	CUserSession(ProxyUserClient* proxy, std::string &, unsigned int, unsigned int/* ttl = 30 * 1000*/);

public:
	virtual ~CUserSession();

	std::string &AccessToken(){ return mAccessToken; }
	ProxyUserClient &Proxy(){ return *mProxy; }

	void Reset();
	//void StartStreaming(CStreaming *streaming = NULL);
	CStreaming *GetRealTimeStreaming(const std::string &srcId);
	CStreaming *CreateStreaming(const std::string &srcId, void *, CStreaming::StreamingType st = CStreaming::ST_REALTIME);
	//CStreaming *CreateRealTimeStreaming(const std::string &srcId, CStreaming::StreamingType st = CStreaming::ST_REALTIME);
	void StopStreaming();

	bool Keepalive();
	unsigned int &Age();
	void UpdateAge();
	inline bool IsLiving(){ return this->Age() < mTTL; }

	void AddTask(CSessionTask *);
	//working thread
	void StartWorking();
	void StopWorking();

protected:
	//static DWORD WINAPI StreamingStarter(void* param);
	//virtual unsigned long Streaming();	//thread body

	//work thread
	static DWORD WINAPI WorkingStarter(void *param);
	virtual unsigned long Working();
	SessionTaskQueue mTaskQueue;
	
	//int FlushPlaylist();
	//int ErasePlaylist();

private:
	inline void generateAccessToekn();
	//std::string PlaylistFilename();

	//static int realPacketsDeliverer(const unsigned char*, const unsigned long, void*);
	/*
	struct TLVSTREAMINFO{
		char            cpSynHead[8];
		int             dwDeviceType;
		long            dwlongitude;
		long            dwlatitude;
		unsigned long   udwStreamType;
		long            dwTimeTag;
		long			dwMillisTag;
		unsigned long	udwFrameType;
		unsigned long	udwHeadSize;
		unsigned long	udwBuffSize;
		unsigned char	ucArrayBuffer[1];
	};
	*/
	int &ClipId(){ return mClipId; }

private:
	friend class CUserSessions;
	std::string mAccessToken;
	ProxyUserClient* mProxy;
	unsigned int mIndex;
	std::string mUsername;
	unsigned int mAge;		//milliseconds
	unsigned int mAgeBase;
	unsigned int mTTL;

	CStreaming *mpStreaming;

	//HANDLE mStreamingThread;
	//bool mIsStreaming;
	//StreamingType mStreamingType;
	int mClipId;	//used only in history playing

	HANDLE mWorkingThread;
	/*
	CBasePlaylist *mPlaylist;
	static const unsigned long MAX_FRAME_BUFFER_SIZE = 1 << 21;	//2M
	unsigned char *mFramesBuffer;

	MP2TMuxer *mMuxer;	//muxer will do the program and streams cleaning
	Program::Stream *mStreamAudio;
	Program::Stream *mStreamVideo;
	Program *mProgram;
	*/
};

class CUserSessions
{
private:
	CUserSessions(void);
	static CUserSessions *mpInstance;
public:
	static CUserSessions *GetInstance();

	virtual ~CUserSessions(void);

	CUserSession *CreateSession(std::string &);
	void RemoveSession(std::string &accessToken);

	CUserSession *operator[](std::string &accessToken);
	CUserSession *QueryUserSession(std::string &at);

	CStreaming *GetRealTimeStreaming(const std::string &srcId);
	CStreaming *CreateRealTimeStreaming(const std::string &srcId, ProxyDevice *pd);

	//remove dead session
	void Clearup();
	void CleanupStreaming();
	int Work();

	void AddTask(CSessionTask *);

	void Lock(){ mSessionLock.Lock(); }
	void Unlock() { mSessionLock.Unlock(); }

protected:
	virtual unsigned int WorkLoop();

private:
	//thread starter
	static DWORD WINAPI WorkStarter(void *param);

private:
	typedef std::map<std::string, CUserSession*> UserSessionMap;
	UserSessionMap mUserSessionMap;
	lelink::CriSection mSessionLock;

	typedef std::map<std::string, CStreaming *> StreamingMap;
	StreamingMap mStreamingMap;

	HANDLE mWorkThread;
	bool mWorking;

	static unsigned int UserIndex;
	unsigned int mTTL;	//milliseconds
};

