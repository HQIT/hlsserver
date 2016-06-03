// LeLinkHLSServer.cpp : Implementation of WinMain


#include "stdafx.h"
#include "resource.h"
#include "LeLinkHLSServer_i.h"

#include "ProxyUserClient.h"
//#pragma comment(lib, "ProxyUserClient.lib")
#pragma comment(lib, "LelinkProxyUser.lib")
#pragma comment(lib, "mp2tmux.lib")

#include "NetworkTCP.h"
#include "UserSessions.h"

#include "json/json.h"
#pragma comment(lib, "json_mtdx32.lib")

#include <stdio.h>

#include "Configure.h"

class CLeLinkHLSServerModule : public ATL::CAtlServiceModuleT< CLeLinkHLSServerModule, IDS_SERVICENAME >
	{
public :
	DECLARE_LIBID(LIBID_LeLinkHLSServerLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_LELINKHLSSERVER, "{6B22D22D-71FA-4001-8082-2D7B0EE4F9CB}")
		HRESULT InitializeSecurity() throw()
	{
		// TODO : Call CoInitializeSecurity and provide the appropriate security settings for your service
		// Suggested - PKT Level Authentication, 
		// Impersonation Level of RPC_C_IMP_LEVEL_IDENTIFY 
		// and an appropiate Non NULL Security Descriptor.

		return S_OK;
	}
	};

CLeLinkHLSServerModule _AtlModule;

static int OnRequest(SOCKET sock, char* data, unsigned short size, void* sessions){
	if(size <= 0){
		return -1;
	}
	
	CUserSessions *Sessions = (CUserSessions *)sessions;

	std::string &req = std::string(data);
	std::string::size_type ipos = req.find_first_of(':');
	std::string method = req.substr(0, ipos);
	std::string json = req.substr(ipos + 1);

	//parse data into json request
	Json::Reader reader;
	Json::Value mRoot;
	if(!reader.parse(json, mRoot)){
		CResponser resp(sock);
		resp.Response(std::string("1000"), std::string("Invalid JSON object in request"));
		return -2;
	}

	CUserSession *session = NULL;
	if(method != "login"){
		if(mRoot["access_token"].isNull()){
			CResponser resp(sock);
			resp.Response(std::string("1001"), std::string("Forbidden, missing assess token"));
			return -21;
		}else{
			Sessions->Lock();
			session = (*Sessions)[mRoot["access_token"].asString()];
			if(!session){
				CResponser resp(sock);
				resp.Response(std::string("1002"), std::string("Forbidden, session is not exists"));
				Sessions->Unlock();
				return -22;
			}
			Sessions->Unlock();
		}
	}else{	//login need a session !
		std::string username = mRoot["Username"].asString();
		session = Sessions->CreateSession(username);
	}

	Sessions->AddTask(CSessionTask::New(sock, method, json, session->AccessToken()));

	return 0;
}

//
long __stdcall __CxxUnhandledExceptionFilter(struct _EXCEPTION_POINTERS *); 
long __stdcall MyUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo){
	printf( "caught ...\n" ); 
	ExitProcess( 0 ); 
	return EXCEPTION_CONTINUE_SEARCH; 
}

//extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int nShowCmd)
int main()
{
	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

	DWORD oldProtect; 
	VirtualProtect(__CxxUnhandledExceptionFilter, 5, PAGE_EXECUTE_READWRITE, &oldProtect); 
	*(char*)__CxxUnhandledExceptionFilter = '\xe9';// far jmp 
	*(unsigned int*)((char*)__CxxUnhandledExceptionFilter + 1) = 
		(unsigned int)MyUnhandledExceptionFilter - ((unsigned int)__CxxUnhandledExceptionFilter + 5); 
	VirtualProtect(__CxxUnhandledExceptionFilter, 5, oldProtect, &oldProtect);

	//
	std::wstring CmdLine = GetCommandLine();	//lpCmdLine;
	std::wstring::size_type pos = CmdLine.find(_T("/config"));
	std::wstring::size_type pos2 = CmdLine.find(_T(" /"), pos + 7);

	std::string path = std::string("lelink.cfg");
	if(pos == std::wstring::npos){
		printf("[WARNNING]USE /config to indicate config file! now DEFAULT config used\n");
		//return -1;
	}else{
		//USES_CONVERSION;
		std::wstring wpath = CmdLine.substr(pos + 8, pos2 - pos - 8);
		//std::string path;// = W2A(wpath.c_str());
		path.assign(wpath.begin(), wpath.end());
	}

	if(0 > CConfigure::GetInstance()->Load(path)){
		printf("[ERROR]CAN NOT find any config file, FAIL to launch!\n");
		return -2;
	}
	
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	CUserSessions *pSessions = CUserSessions::GetInstance();
	pSessions->Work();

	CNetworkTCP network(20008);
	network.Open(OnRequest, pSessions);

	network.Close();

	WSACleanup();

	return 0;

//	return _AtlModule.WinMain(nShowCmd);
}

