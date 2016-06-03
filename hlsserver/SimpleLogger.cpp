#include "StdAfx.h"
#include "SimpleLogger.h"

#include "Configure.h"
#include "CriSection.h"

#include <time.h>

using lelink::CSimpleLogger;
using lelink::CriSection;

CSimpleLogger::CSimpleLogger()
	:mpFile(NULL), mpLoggerLock(NULL)
{
#pragma warning(disable: 4996)
	CConfigure *pConfig = CConfigure::GetInstance();
	if(pConfig->LoggerEnable){

		char timestr[128] = {0};
		time_t now = time(NULL);
		size_t pos = strftime(timestr, sizeof(timestr), "%y%m%d_%H%M", localtime(&now));
		timestr[pos] = 0;

		mpFile = fopen((pConfig->MediasAbsolutePath + "\\hlserver_" + timestr + ".log").c_str(), "w+b");
		fclose(mpFile);
		mpFile = fopen((pConfig->MediasAbsolutePath + "\\hlserver_" + timestr + ".log").c_str(), "a+b");
		if(!mpFile){
			pConfig->LoggerEnable = false;
		}
	}

	mpLoggerLock = new CriSection();
}

CSimpleLogger::~CSimpleLogger(){
	if(mpFile){
		fclose(mpFile);
		mpFile = NULL;
	}

	if(mpLoggerLock){
		mpLoggerLock->Unlock();
		delete mpLoggerLock;
	}
}

void CSimpleLogger::Log(const char *format, ...){
	CConfigure *pConfig = CConfigure::GetInstance();

	char timestr[128] = {0};
	time_t now = time(NULL);
	size_t pos = strftime(timestr, sizeof(timestr), "%y/%m/%d %H:%M:%S", localtime(&now));
	timestr[pos] = 0;

	if(pConfig->LoggerEnable){
		mpLoggerLock->Lock();

		va_list params;
		va_start(params, format);
		fprintf(mpFile, "[%s]", timestr);
		vfprintf(mpFile, format, params);
		va_end(params);

		fprintf(mpFile, "\r\n");
		fflush(mpFile);

		mpLoggerLock->Unlock();
	}
}

CSimpleLogger *CSimpleLogger::GetLogger(){
	if(mpInstance == NULL)
		mpInstance = new CSimpleLogger();

	return mpInstance;
}

CSimpleLogger *CSimpleLogger::mpInstance = NULL;