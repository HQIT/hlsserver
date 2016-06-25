#include "WinTicker.h"

#ifdef _WIN32
#include <Windows.h>
#include <MMSystem.h>
#endif

using namespace com::cloume::cap;

WinTicker::WinTicker()
	: Ticker(GetCurrentTickCount())
{

}

unsigned int WinTicker::GetCurrentTickCount(){
	return timeGetTime();
}