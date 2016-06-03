// LeLinkHLSServer.cpp : Implementation of WinMain


#include "stdafx.h"
#include "resource.h"
#pragma comment(lib, "libmp2tmux.lib")
#pragma comment(lib, "Dbghelp.lib")

#include "NetworkTCP.h"
//#include "UserSessions.h"
#include "SimpleLogger.h"
#include "Streaming.h"

#include "json/json.h"
#pragma comment(lib, "json_mtdx32.lib")

#include <stdio.h>

#include "Configure.h"

using namespace lelink;

static int OnRequest(SOCKET socket, void* sessions){
	CSimpleLogger::GetLogger()->Log("Net stream source in!\n");

	CStreaming *pStreaming = new CStreaming(socket);
	pStreaming->Start();

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

	CNetworkTCP network(20008);
	network.Open(OnRequest, NULL);

	network.Close();

	WSACleanup();

	return 0;

//	return _AtlModule.WinMain(nShowCmd);
}

