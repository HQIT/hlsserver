#include "StdAfx.h"
#include "UserSessions.h"
#include "Configure.h"

#include <iostream>

CUserSessions *CUserSessions::mpInstance = NULL;
CUserSessions *CUserSessions::GetInstance(){
	if(mpInstance == NULL){
		mpInstance = new CUserSessions();
	}
	return mpInstance;
}

CUserSessions::CUserSessions(void) 
	: /*mTTL(30 * 1000), */mWorkThread(INVALID_HANDLE_VALUE),
	mWorking(false)
{
	mTTL = CConfigure::GetInstance()->SessionTTL;
}

CUserSessions::~CUserSessions(void){
	lelink::CSimpleLogger::GetLogger()->Log("[INFO]Session pool shutting down !");

	mWorking = false;
	WaitForSingleObject(mWorkThread, INFINITE);

	UserSessionMap::iterator it = mUserSessionMap.begin();

	while(it != mUserSessionMap.end()){
		delete it->second;
		++it;
	}

	mUserSessionMap.clear();
}

CStreaming *CUserSessions::GetRealTimeStreaming(const std::string &srcId){
	CStreaming *streaming = NULL;

	Lock();
	if(mStreamingMap.count(srcId) != 0){
		streaming = mStreamingMap[srcId];
	}
	Unlock();

	return streaming;
}

CStreaming *CUserSessions::CreateRealTimeStreaming(const std::string &srcId, ProxyDevice *pd){
	CStreaming *streaming = NULL;

	Lock();
	streaming = new CStreaming(srcId, pd, CStreaming::ST_REALTIME);
	mStreamingMap[srcId] = streaming;
	Unlock();

	return streaming;
}

unsigned int CUserSessions::UserIndex = 0;

CUserSession *CUserSessions::CreateSession(std::string &username){
	//one user can have multi-sessions
	CUserSession *session = NULL;
	
	ProxyUserClient *proxy = new ProxyUserClient();
	printf("Session proxy: %p \n", proxy);
	session = new CUserSession(proxy, username, UserIndex++, mTTL);

	Lock();
	mUserSessionMap[session->mAccessToken] = session;
	Unlock();

	lelink::CSimpleLogger::GetLogger()->Log("[INFO]Create session [%d][%s]", UserIndex, session->mAccessToken.c_str());

	session->StartWorking();

	return session;
}

void CUserSessions::RemoveSession(std::string &accessToken){
	
	CUserSession *session = NULL;
	
	mSessionLock.Lock();
	if(mUserSessionMap.count(accessToken) != 0){
		session = mUserSessionMap[accessToken];
		mUserSessionMap.erase(accessToken);
	}
	mSessionLock.Unlock();

	if(session){
		//session->StopStreaming();
		delete session;
	}
}

CUserSession *CUserSessions::operator [](std::string &at){

	CUserSession *session = NULL;

	if(mUserSessionMap.count(at) != 0)
		session = mUserSessionMap[at];
	
	return session;
}

CUserSession *CUserSessions::QueryUserSession(std::string &at){
	return (*this)[at];
}

void CUserSessions::CleanupStreaming(){
	Lock();

	StreamingMap::iterator it = mStreamingMap.begin();
	while(it != mStreamingMap.end()){

		CStreaming *streaming = it->second;
		if(streaming->Using() <= 0){
			lelink::CSimpleLogger::GetLogger()->Log("[INFO]Streaming [%s] destroyed!", streaming->SourceId().c_str());

			delete streaming;
			it = mStreamingMap.erase(it);
		}else{
			++it;
		}
	}

	Unlock();
}

void CUserSessions::Clearup(){

	this->Lock();

	UserSessionMap::iterator it = mUserSessionMap.begin();
	while(it != mUserSessionMap.end()){

		CUserSession *session = it->second;
		if(session->Age() >= mTTL){
			lelink::CSimpleLogger::GetLogger()->Log("[INFO]Session [%s] dead!", session->AccessToken().c_str());

			//session->StopStreaming();
			delete session;
			it = mUserSessionMap.erase(it);
		}else{
			++it;
		}
	}

	this->Unlock();
}

int CUserSessions::Work(){

	mWorking = true;
	mWorkThread = CreateThread(NULL, 0, WorkStarter, this, 0, NULL);

	return 0;
}

DWORD WINAPI CUserSessions::WorkStarter(void *param){
	CUserSessions *sessions = (CUserSessions *)param;
	return sessions->WorkLoop();
}

unsigned int CUserSessions::WorkLoop(){

	while(mWorking){
		this->Clearup();
		Sleep(2000);
	}

	return 0;
}

//SessionTask

unsigned int CSessionTask::TaskIndex = 0;

CSessionTask::CSessionTask(SOCKET sock, const std::string &method, const std::string &body, std::string &at){
	this->mSocket = sock;
	this->mMethod.assign(method);
	this->mBody.assign(body);
	this->mAccessToken.assign(at);
	this->mIndex = TaskIndex++;
}

CSessionTask::~CSessionTask(){
	closesocket(mSocket);
}

CSessionTask *CSessionTask::New(SOCKET sock, const std::string &method, const std::string &body, std::string &at){
	return new CSessionTask(sock, method, body, at);
}

void CUserSessions::AddTask(CSessionTask *pTask){
	this->Lock();
	lelink::CSimpleLogger::GetLogger()->Log("[INFO]AddTask[%d] [%s] into [%s]",
		pTask->Index(),
		pTask->mMethod.c_str(), pTask->mAccessToken.c_str());

	CUserSession *pSession = this->QueryUserSession(pTask->mAccessToken);
	pSession->AddTask(pTask);

	this->Unlock();
}