#include "StdAfx.h"
#include "UserSessions.h"

#include <assert.h>
#include <fstream>

#include <ImageHlp.h>
#pragma comment(lib, "ImageHlp.lib")

#include "Configure.h"

#include "json/json.h"

//#include "Media.h"
#include "HistoryMedia.h"
#include "HistoryPlaylist.h"

#include "NetworkTCP.h"

CUserSession::CUserSession(ProxyUserClient* proxy, std::string &username, unsigned int index, unsigned int ttl)
	: mpStreaming(NULL),
	mClipId(-1),
	mWorkingThread(INVALID_HANDLE_VALUE),
	mAge(0), mAgeBase(0), mTTL(ttl)
{
	mProxy = proxy;
	mIndex = index;
	mUsername = username;

	generateAccessToekn();

	Keepalive();
}

CUserSession::~CUserSession(){
	StopWorking();

	if(mProxy){
		delete mProxy;
	}
}

#pragma warning(disable: 4996)

void CUserSession::generateAccessToekn(){
	static char table[] = {
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 
		's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 
		'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 
		'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
	};

	SYSTEMTIME st;
	GetLocalTime(&st);

	char result[64] = {0};
	memset(result, 0, sizeof(result));

	int seed = st.wMilliseconds;
	srand(seed);

	for(int i = 0; i < 6; i++){
		result[i] = table[rand() % sizeof(table)];
	}

	_snprintf(result + 6, 58, "%04d", mIndex);

	mAccessToken.assign(result);
}

void CUserSession::Reset(){

	//stop the Streaming thread
	StopStreaming();
}

CStreaming *CUserSession::GetRealTimeStreaming(const std::string &srcId){
	CUserSessions *mgr = CUserSessions::GetInstance();
	return mgr->GetRealTimeStreaming(srcId);

}

CStreaming *CUserSession::CreateStreaming(const std::string &srcId, void *proxy, CStreaming::StreamingType st /* = CStreaming::ST_REALTIME */){
	this->Reset();

	if(st == CStreaming::ST_REALTIME){
		CUserSessions *mgr = CUserSessions::GetInstance();
		this->mpStreaming = mgr->GetRealTimeStreaming(srcId);
		if(NULL == this->mpStreaming){
			this->mpStreaming = mgr->CreateRealTimeStreaming(srcId, (ProxyDevice *)proxy);
		}
	}else if(st == CStreaming::ST_HISTORY){
		this->mpStreaming = new CStreaming(this->AccessToken() + srcId, (ProxyUserClient *)proxy, CStreaming::ST_HISTORY);
	}

	return this->mpStreaming;
}

void CUserSession::StopStreaming(){
	if(this->mpStreaming){
		this->mpStreaming->Stop();
		if(this->mpStreaming->Type() == CStreaming::ST_HISTORY){
			delete this->mpStreaming;
		}else if(this->mpStreaming->Type() == CStreaming::ST_REALTIME){
			CUserSessions *mgr = CUserSessions::GetInstance();
			mgr->CleanupStreaming();
		}
		this->mpStreaming = NULL;
	}
}

bool CUserSession::Keepalive(){
	if(mAge < 0)
		return false;

	mAgeBase = GetTickCount();
	mAge = 0;

	return true;
}

unsigned int &CUserSession::Age(){
	return mAge;
}

void CUserSession::UpdateAge(){
	mAge = GetTickCount() - mAgeBase;
}

void CUserSession::AddTask(CSessionTask *pTask){
	mTaskQueue.push(pTask);
}

void CUserSession::StartWorking(){
	mWorkingThread = CreateThread(NULL, 0, WorkingStarter, this, 0, NULL);
}

void CUserSession::StopWorking(){
	this->StopStreaming();

	this->Age() = mTTL * 2;

	WaitForSingleObject(mWorkingThread, INFINITE);
}

DWORD WINAPI CUserSession::WorkingStarter(void *param){
	CUserSession *session = (CUserSession *)param;

	return session->Working();
}

unsigned long CUserSession::Working(){
	while(this->Age() < mTTL){

		this->UpdateAge();

		if(mTaskQueue.empty()){
			Sleep(10);
			continue;
		}

		CSessionTask *pTask = mTaskQueue.front();
		string &method = pTask->mMethod;
		string &json = pTask->mBody;
		SOCKET sock = pTask->mSocket;

		Json::Value mRoot;
		Json::Reader r;
		//here json format is OK!
		r.parse(json, mRoot);

		if(method == "login"){
			std::string out;
			int rc = this->Proxy().Login(json, out);

			Json::Reader r;
			Json::Value j;
			r.parse(out, j);
			j["access_token"] = this->AccessToken();

			CResponser resp(sock);
			resp.Response(j);

		}else if(method == "logout"){
			std::string out;

			this->Proxy().Logout(json, out);
			this->Age() = mTTL * 2;

			CResponser resp(sock);
			resp.Response(out);

		}else if(method == "query_device_list"){
			std::string out;
			this->Proxy().QueryDeviceList(json, out);

			CResponser resp(sock);
			resp.Response(out);

		}else if(method == "get_device_list"){
			std::string out;
			this->Proxy().GetDeviceList(json, out);

			CResponser resp(sock);
			resp.Response(out);

		}else if(method == "request_video_play"){

			if(mpStreaming){
				this->Reset();
			}

			std::string out;

			//如果播放Stream已经存在 不需要再发送这个请求!
			Json::Reader rin;
			Json::Value jin;
			rin.parse(json, jin);
			//"{"access_token":"DcRKlQ0000","Serial":"00afdb0001f1"}"

			std::string serial = jin["Serial"].asString();
			this->mpStreaming = this->GetRealTimeStreaming(serial);

			//if Using < 0 then streaming is in error state!
			if(this->mpStreaming && this->mpStreaming->Using() < 0){
				this->StopStreaming();	//StopStreaming will set mpStreaming to NULL
			}

			if(this->mpStreaming){

				Json::Value j;
				r.parse(out, j);
				j["Status"] = "0";
				j["ErrMsg"] = "Already Streaming";
				j["media_url"] = this->mpStreaming->PlaylistURL();	//add media url into response json
				this->mpStreaming->Start();

				CResponser resp(sock);
				resp.Response(j);
			}else{

				ProxyDevice *pDevProxy = NULL;
				int ret = this->Proxy().RequestVideoPlay(json, out, &pDevProxy);
				if(ret != 0){
					CResponser resp(sock);
					resp.Response(out);
				}else{

					this->mpStreaming = this->CreateStreaming(serial, (void *)pDevProxy);
					this->mpStreaming->Start();

					Json::Value j;
					r.parse(out, j);
					j["media_url"] = this->mpStreaming->PlaylistURL();	//add media url into response json

					CResponser resp(sock);
					resp.Response(j);
				}
			}

			lelink::CSimpleLogger::GetLogger()->Log("[INFO]request_video_play [%s][%s]", 
				this->mAccessToken.c_str(), serial.c_str());

		}else if(method == "request_video_stop"){
			//stop streaming
			this->StopStreaming();

			CResponser resp(sock);
			resp.Response(std::string("0"), std::string("OK"));

			lelink::CSimpleLogger::GetLogger()->Log("[INFO]request_video_stop [%s]", 
				this->mAccessToken.c_str());

		}else if(method == "operate_ptz"){
			std::string out;
			this->Proxy().OperatePtz(json, out);

			CResponser resp(sock);
			resp.Response(out);

		}else if(method == "get_clip_list"){
			std::string out;
			this->Proxy().GetClipList(json, out);

			CResponser resp(sock);
			resp.Response(out);

		}else if(method == "query_clip_list"){
			std::string out;
			this->Proxy().QueryClipList(json, out);

			CResponser resp(sock);
			resp.Response(out);

		}else if(method == "request_clip_play"){
			std::string out;

			int ret = this->Proxy().RequestClipPlay(json, out);
			if(ret != 0){
				CResponser resp(sock);
				resp.Response(out);
			}else{
				Json::Reader r;
				Json::Value j;
				r.parse(json, j);

				this->mpStreaming = this->CreateStreaming(j["Id"].asString(), &(Proxy()), CStreaming::ST_HISTORY);
				this->mpStreaming->Start();
				j.clear();

				//response
				r.parse(out, j);
				j["media_url"] = this->mpStreaming->PlaylistURL();	//add media url into response json

				CResponser resp(sock);
				resp.Response(j);

				lelink::CSimpleLogger::GetLogger()->Log("[INFO]request_clip_play [%s][%s]", 
					this->mAccessToken.c_str(), j["Id"].asString().c_str());
			}

		}else if(method == "keep_alive"){
			CResponser resp(sock);
			if(!this->Keepalive()){
				resp.Response(std::string("1003"), std::string("Session is dead"));
			}else{
				resp.Response(std::string("0"), std::string("OK"));
			}
		}
#if 0 //NOT HERE, there should be a callback named as OnData
		else if(method == "send_audio_data"){
			//TODO: get audio data from post data
			//session->Proxy().SendAudioData("", "");
		}
#endif
		else{
			CResponser resp(sock);
			resp.Response(std::string("2000"), std::string("UnImplemented method"));
		}

		lelink::CSimpleLogger::GetLogger()->Log("[INFO]Task[%d] [%s] for [%s] dispatched!", pTask->Index(), pTask->mMethod.c_str(), pTask->mAccessToken.c_str());

		delete pTask;
		mTaskQueue.pop();
	}

	return NULL;
}