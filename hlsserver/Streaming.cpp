#include "StdAfx.h"
#include <ImageHlp.h>

#include "HistoryMedia.h"
#include "HistoryPlaylist.h"

#include "MP2TMuxer.h"
#include "Streaming.h"
#include "Configure.h"
//#include "UserSessions.h"
#include "ProxySource.h"
#include "MutableHeaderBuffer.h"
#include "WinTicker.h"

#include <fstream>
#include <thread>

using lelink::CStreaming;
using namespace com::cloume::cap;

void FileWriteThreadEntry(CStreaming* This)
{
	This->FileWriteThread();
}
//CStreaming::CStreaming(std::string srcId, CUserSession *session, StreamingType st /* = ST_REALTIME */)
CStreaming::CStreaming(SOCKET socket, StreamingType st /* = ST_REALTIME */)
	: mpSource(new CProxySource(socket)),/*mSourceId(srcId), */mStreamingType(st), mSourceId("1"),
	///mpProxy(proxy), 
	mpPlaylist(NULL),
	mUsing(0),
	mMuxer(NULL), mStreamVideo(NULL), mStreamAudio(NULL), mProgram(NULL),
	mpFrameBuffer(NULL)
{
	//mpProxy = new ProxyUserClient(proxy);
	mpFrameBuffer = new unsigned char[MAX_FRAME_BUFFER_SIZE];
	
	//prepare the streaming folder!
	MakeSureDirectoryPathExists((this->StreamingAbsolutePath() + "\\").c_str());

	WIN32_FIND_DATAA find_data;
	HANDLE h = FindFirstFileA((this->StreamingAbsolutePath() + "\\*.ts").c_str(), &find_data);
	if(h != INVALID_HANDLE_VALUE){
		DeleteFileA((this->StreamingAbsolutePath() + "\\" + find_data.cFileName).c_str());
		while(FindNextFileA(h, &find_data)){
			DeleteFileA((this->StreamingAbsolutePath() + "\\" + find_data.cFileName).c_str());
		}
	}
	FindClose(h);
	startTime = ::timeGetTime();
	_fileWriteEvent = CreateEvent(0, 0, 0, 0);
	InitializeCriticalSection(&_fileLock);
	//initialize multiplexer
	mMuxer = new MP2TMuxer(true, 30.);
	mMuxer->SetTicker(new WinTicker());
	mProgram = new Program();
	mStreamVideo = new Program::StreamVideo(Program::Stream::STREAMSUBTYPE_H264_VIDEO, 100);
	mStreamAudio = new Program::StreamAudio(Program::Stream::STREAMSUBTYPE_ISO_IEC_13818_3_AUDIO, 101);
	mProgram->AddStream(mStreamVideo);
	mProgram->AddStream(mStreamAudio);
	mProgram->PCRPID(mStreamAudio->ElementaryPID());
	mMuxer->AddProgram(mProgram);
	mMuxer->SetPacketsDeliverer(realPacketsDeliverer, this);

	new std::thread(FileWriteThreadEntry, this);
}

CStreaming::~CStreaming(){
	mUsing = 0;

	if(this->mStreamingType == ST_REALTIME && mpSource != NULL){
		delete mpSource;
	}

	if(mMuxer){
		delete mMuxer;
		mMuxer = NULL;
	}

	if(mpFrameBuffer){
		delete[] mpFrameBuffer;
		mpFrameBuffer = NULL;
	}

	ErasePlaylist();
	RemoveDirectoryA(this->StreamingAbsolutePath().c_str());
}

#if 0
ProxyUserClient *CStreaming::UserProxy(){
	if(this->mStreamingType != ST_HISTORY)
		return NULL;

	if(this->mpProxy == NULL)
		return NULL;
		
	return (ProxyUserClient *)(this->mpProxy);
}

ProxyDevice *CStreaming::DeviceProxy(){
	if(this->mStreamingType != ST_REALTIME)
		return NULL;

	if(this->mpProxy == NULL)
		return NULL;

	return (ProxyDevice *)(this->mpProxy);
}
#endif

void CStreaming::FileWriteThread()
{
	FILE* stream = 0;
	std::string fileName;

	while (true) {
		::WaitForSingleObject(_fileWriteEvent, -1);
		::EnterCriticalSection(&_fileLock);
		std::vector<FileWriteTask*> tasks = _fileWriteTasks;
		_fileWriteTasks.clear();
		::LeaveCriticalSection(&_fileLock);
		for (size_t i = 0; i < tasks.size(); i++)
		{
			auto task = tasks[i];
			if (fileName != task->fileName) {
				if (stream)
					fclose(stream);
				fileName = task->fileName;
				stream = fopen(fileName.c_str(), "wb");
			}
			fwrite(&task->data[0], 1, task->data.size(), stream);
		}
	}
}

void CStreaming::Enqueue(FileWriteTask* task)
{
	::EnterCriticalSection(&_fileLock);
	_fileWriteTasks.push_back(task);
	::LeaveCriticalSection(&_fileLock);
	SetEvent(_fileWriteEvent);
}

int CStreaming::realPacketsDeliverer(const unsigned char* data, const unsigned long size, void* streaming){
	CStreaming *thiz = (CStreaming *)streaming;
	FileWriteTask* task = new FileWriteTask();
	task->fileName = thiz->CurrentMediaFilename();

	task->data.resize(size);
	memcpy(&task->data[0], data, size);
	thiz->Enqueue(task);

	return 0;
}

std::string CStreaming::PlaylistURL(){ 
	CConfigure *configure = CConfigure::GetInstance();
	return configure->WebContextAddress 
		+ configure->PlaylistPath
		+ std::string("/") + PlaylistFilename(); 
}

std::string CStreaming::CurrentMediaFilename(){
	std::string filename
		= StreamingAbsolutePath() + "\\" + mpPlaylist->LatestMedia()->Filename();

	return filename;
}

std::string CStreaming::StreamingAbsolutePath(){
	return CConfigure::GetInstance()->MediasAbsolutePath + "\\" + mSourceId;
}

inline std::string CStreaming::PlaylistFilename(){
	return mSourceId + std::string(".m3u8");
}

int CStreaming::FlushPlaylist(){

	MakeSureDirectoryPathExists((CConfigure::GetInstance()->PlaylistAbsolutePath + "\\").c_str());

	std::string filename 
		= CConfigure::GetInstance()->PlaylistAbsolutePath 
		+ std::string("\\") + PlaylistFilename();

	//std::ofstream f(filename.c_str(), std::ios::app | std::ios::out);
	std::ofstream f(filename.c_str());
	if(!f.is_open()){
		return -1;
	}

	f << mpPlaylist->ToString();
	f.flush();
	f.close();

	return 0;
}

int CStreaming::ErasePlaylist(){

	if(!mpPlaylist)
		return 0;

	std::string filename 
		= CConfigure::GetInstance()->PlaylistAbsolutePath 
		+ std::string("\\") + PlaylistFilename();

	mpPlaylist->Clear();
	delete mpPlaylist;
	mpPlaylist = NULL;

	return DeleteFileA(filename.c_str());
}

DWORD CStreaming::StreamingStarter(void* param){
	CStreaming *streaming = (CStreaming *)param;

	return streaming->Streaming();
}

/*
版本 00-MPEG 2.5   01-未定义     10-MPEG 2     11-MPEG 1
层	00-未定义      01-Layer 3     10-Layer 2      11-Layer 1
*/

int calMP3DataLength(lelink::LPMP3HEADER pHeader) {
	unsigned int base = 0, samplingFrequency = 0;
	switch (pHeader->version)
	{
	case 0x2:
	case 0x0: base = pHeader->layer == 0x3 ? 24000 : 72000;
		break;
	case 0x3: base = pHeader->layer == 0x3 ? 48000 : 144000;
		break;
	}

	///FIXME: 查表获得bitrate
	unsigned int bitrate = 320;

	switch (pHeader->version)
	{
	case 0x3: ///version 1
		switch (pHeader->samplingFrequency)
		{
		case 0x0: samplingFrequency = 44100; break;
		case 0x1: samplingFrequency = 48000; break;
		case 0x2: samplingFrequency = 32000; break;
		}
		break;
	}

	return base * bitrate / samplingFrequency + pHeader->padding;
}

/*
mpeg1.0
layer1 :   帧长 = (48000 * bitrate) / sampling_freq + padding
layer2 & 3 : 帧长 = (144000 * bitrate) / sampling_freq + padding
mpeg2.0
layer1 : 帧长 = (24000 * bitrate) / sampling_freq + padding
layer2 & 3 : 帧长 = (72000 * bitrate) / sampling_freq + padding
对于MPEG-1：  00-44.1kHz    01-48kHz    10-32kHz      11-未定义
对于MPEG-2：  00-22.05kHz   01-24kHz    10-16kHz      11-未定义
对于MPEG-2.5： 00-11.025kHz 01-12kHz    10-8kHz       11-未定义
*/

unsigned long CStreaming::Streaming(){
	if(mpPlaylist){
		ErasePlaylist();
	}

	int capacity = CConfigure::GetInstance()->PlaylistCapacity;
	if(mStreamingType == ST_REALTIME){
		mpPlaylist = new CBasePlaylist(capacity);
	}else if(mStreamingType == ST_HISTORY){
		//mpPlaylist = new CHistoryPlaylist(capacity + 1);
		mpPlaylist = new CHistoryPlaylist(capacity);
	}

	//提前输出m3u8, 便于app端处理. 2013/02/06
	this->FlushPlaylist();

	const char fan[] = {'|', '/', '-', '\\'};
	int ifan = 0, frameIndex = 0;
	bool iFrameFound = false, isAudioReady = false, isVideoReady = false;

	while(this->mUsing > 0 && !mpPlaylist->End()){

		int ret = 0;
		unsigned int size = 0, version = 0, type = 0;
		if(this->mStreamingType == ST_REALTIME){
			ret = Source()->RecvData((char *)mpFrameBuffer, MAX_FRAME_BUFFER_SIZE, size, version, type);
		}else{
			ret = Source()->RecvData((char *)mpFrameBuffer, MAX_FRAME_BUFFER_SIZE, size, version, type);
		}

		//uplink stream server in trap
		if(-2 == ret){
			//this->mUsing -= 1;
			this->mUsing = -100;	//set an error state!
			mpPlaylist->SetEnd();
			FlushPlaylist();
			continue;
		}

		if(0 != ret)
		{
			///Sleep(5);
			continue;
		}

		///必须先确定a/v format参数
		if (!isAudioReady && type == 10) {
			AudioFormatPacketData *pFormat = (AudioFormatPacketData *)mpFrameBuffer;
			///pFormat->
			isAudioReady = true;
		}

		if (!isVideoReady && type == 30) {
			VideoFormatPacketData *pFormat = (VideoFormatPacketData *)mpFrameBuffer;
			isVideoReady = true;
		}

		if (type != 1 && type != 2) continue;

		do{
			if (frameIndex == 0 && type == 1/*stream->udwFrameType == 1*/){	//I
				iFrameFound = true;
				//if mPlaylist is ready, flush it to web dir
				if(mpPlaylist->Ready()){
					FlushPlaylist();
				}

				if(!mpPlaylist->LatestMedia() || mpPlaylist->LatestMedia()->GOP() > 0){
					CMedia *media 
						= (mStreamingType == ST_HISTORY ? new CHistoryMedia(mSourceId) : new CMedia(mSourceId));

					mpPlaylist->AddMedia(media);

					if(mpPlaylist->LatestMedia()->GOP() == 0){
						//deliver PAT/PMT at beginning of each GOP
						///mMuxer->DeliverPATPMT();
					}
				}

				mpPlaylist->LatestMedia()->GOP() += 1;

				printf("gop: %d \n", mpPlaylist->LatestMedia()->GOP());
			}

			if (!iFrameFound) break;

			if (type == 1) {		///H264 VIDEO
				MutableHeaderBuffer buf(size);
				buf.AppendData((const char *) mpFrameBuffer, size);
				this->mMuxer->Mux(mStreamVideo, &buf, 33);
				
				mpPlaylist->LatestMedia()->FrameCount() += 1;
				mMuxer->FrameCount()++;

				frameIndex += 1;
				frameIndex %= 150;		///应该从流中读取GOP大小
			
			} else if(type == 2) {		//MP3 AUDIO
				int remain = size;
				unsigned int frameLength = 0, position = 0;
				while (remain > 0) {
					MP3HEADER header;
					unsigned char *bytes = mpFrameBuffer + position;
					
					header.sync = (bytes[0] << 3) | (bytes[1] >> 5);
					header.version = (bytes[1] >> 3) & 0x3;
					header.layer = (bytes[1] >> 1) & 0x3;
					header.bitrateIndex = (bytes[2] >> 4);
					header.samplingFrequency = (bytes[2] >> 2) & 0x3;
					header.padding = (bytes[2] >> 1) & 0x1;

					frameLength = calMP3DataLength(&header);

					remain -= frameLength;
					position += frameLength;

					///每帧的时间固定24ms; 1152 / 48000
					MutableHeaderBuffer buf(frameLength);
					buf.AppendData((const char *) bytes, frameLength);
					this->mMuxer->Mux(mStreamAudio, &buf, 24);	//26.122 == 1152 * 1000 / 44100
				}
			}

			this->mMuxer->Deliver();
		}while(false);
	}

	return 0;
}

void CStreaming::Start(){
	this->mUsing += 1;

	//if first reference, start the thread
	if(this->mUsing == 1){
		mStreamingThread = CreateThread(NULL, 0, StreamingStarter, this, 0, NULL);
	}
}

void CStreaming::Stop(){

	mUsing -= 1;

	if(mUsing > 0){
		return;
	}

	WaitForSingleObject(mStreamingThread, INFINITE);
}