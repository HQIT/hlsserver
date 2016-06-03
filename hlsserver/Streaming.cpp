#include "StdAfx.h"
#include <ImageHlp.h>

#include "HistoryMedia.h"
#include "HistoryPlaylist.h"

#include "Streaming.h"
#include "PES.h"
#include "Configure.h"
//#include "UserSessions.h"
#include "ProxySource.h"

#include <fstream>

using lelink::CStreaming;

//CStreaming::CStreaming(std::string srcId, CUserSession *session, StreamingType st /* = ST_REALTIME */)
CStreaming::CStreaming(SOCKET socket, StreamingType st /* = ST_REALTIME */)
	: mpSource(new CProxySource(socket)),/*mSourceId(srcId), */mStreamingType(st), 
	///mpProxy(proxy), 
	mpPlaylist(NULL),
	mUsing(0),
	mMuxer(NULL), mStreamVideo(NULL), mStreamAudio(NULL), mProgram(NULL),
	mpFramesBuffer(NULL)
{
	//mpProxy = new ProxyUserClient(proxy);
	mpFramesBuffer = new unsigned char[MAX_FRAME_BUFFER_SIZE];
	
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

	//initialize multiplexer
	mMuxer = new MP2TMuxer(false, 25.);
	mProgram = new Program();
	mStreamVideo = new Program::StreamVideo(Program::Stream::STREAMSUBTYPE_H264_VIDEO, 100);
	//mStreamAudio = new Program::StreamAudio(Program::Stream::STREAMSUBTYPE_ISO_IEC_13818_3_AUDIO, 101);
	mProgram->AddStream(mStreamVideo);
	//mProgram->AddStream(mStreamAudio);
	mProgram->PCRPID(mStreamVideo->ElementaryPID());
	mMuxer->AddProgram(mProgram);
	mMuxer->SetPacketsDeliverer(realPacketsDeliverer, this);
}

CStreaming::~CStreaming(){
	mUsing = 0;

	//回看时没有影子Proxy,因此不需要delete
	//if(this->mStreamingType == ST_HISTORY){
	//	delete (ProxyUserClient *)mpProxy;
	//}

	if(this->mStreamingType == ST_REALTIME){
		//delete (ProxyDevice *)mpProxy;
	}

	if(mMuxer){
		delete mMuxer;
		mMuxer = NULL;
	}

	if(mpFramesBuffer){
		delete[] mpFramesBuffer;
		mpFramesBuffer = NULL;
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
int CStreaming::realPacketsDeliverer(const unsigned char* data, const unsigned long size, void* streaming){
	CStreaming *thiz = (CStreaming *)streaming;

	MakeSureDirectoryPathExists(thiz->CurrentMediaFilename().c_str());

	std::ofstream of(thiz->CurrentMediaFilename(), std::ios::app | std::ios::out | std::ios::binary);
	if(!of.is_open())
		return -1;

	of.write((const char *)(const void *)data , size);
	of.flush();
	of.close();

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

	static const int ARRAY_LENGTH = 1024;
	//int size[ARRAY_LENGTH], count[ARRAY_LENGTH], flag[ARRAY_LENGTH] = {0};
	int size[ARRAY_LENGTH], count = 0;

	bool first_key_frame_found = false;

	const char fan[] = {'|', '/', '-', '\\'};
	int ifan = 0;

	//int err_cnt = 0;
	//int empty_cnt = 0;
	//int befor_i_cnt = 0;

	//int only_4_debug = 1;

	while(this->mUsing > 0 && !mpPlaylist->End()){

		int ret = 0;
		if(this->mStreamingType == ST_REALTIME){
			//if((only_4_debug--) == 0){
			//	ret = -2;
				//ret = DeviceProxy()->RecvData((char *)mpFramesBuffer, -1, size, &count);
			//}else
				ret = Source()->RecvData((char *)mpFramesBuffer, MAX_FRAME_BUFFER_SIZE, size, &count);
		}else{
			//if((only_4_debug--) == 0)
			//	ret = -2;
			//else
				ret = Source()->RecvData((char *)mpFramesBuffer, MAX_FRAME_BUFFER_SIZE, size, &count);
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
			//printf("[ERROR]RecvData return %d \n", ret); fflush(stdout);
			//err_cnt += 1;
			Sleep(5);
			continue;
		}

		if(count <= 0){
			//printf("[WARNING]RecvData got empty \n"); fflush(stdout);
			//empty_cnt += 1;
			Sleep(5);
			continue;
		}

		printf("\r %c", fan[(ifan++) % sizeof(fan)]); fflush(stdout);

		unsigned pos = 0;
		for(int i = 0; i < count; i++){
			///TLVSTREAMINFO* stream = (TLVSTREAMINFO*)(mpFramesBuffer + pos);
			do{
#if 0
				if(strcmp("LELINKLE", stream->cpSynHead) != 0){
					break;//continue;
				}

				if(!first_key_frame_found && stream->udwFrameType != 1){
					//befor_i_cnt += 1;
					break;
				}
#endif
				//printf("st: %d , mills: %d \n", stream->dwTimeTag, stream->dwMillisTag);

				if (true /*stream->udwFrameType == 1*/){	//I
					if(!first_key_frame_found){
						first_key_frame_found = true;
						//lelink::CSimpleLogger::GetLogger()->Log("[INFO]FIRST I FRAME REACH, START WRITE TS CLIPS");
						//printf(" !!! %d frames befor first I \n", befor_i_cnt);
					}

					//if mPlaylist is ready, flush it to web dir
					if(mpPlaylist->Ready()){
						FlushPlaylist();
						//printf("error: %d, empty: %d \n", err_cnt, empty_cnt);
						//lelink::CSimpleLogger::GetLogger()->Log("[INFO]M3U8 IS READY NOW!");
					}

					if(!mpPlaylist->LatestMedia() || mpPlaylist->LatestMedia()->GOP() > 0){
						CMedia *media 
							= (mStreamingType == ST_HISTORY ? new CHistoryMedia(mSourceId) : new CMedia(mSourceId));

						mpPlaylist->AddMedia(media);

						if(mpPlaylist->LatestMedia()->GOP() == 0){
							//deliver PAT/PMT at beginning of each GOP
							mMuxer->DeliverPATPMT();
						}
					}

					mpPlaylist->LatestMedia()->GOP() += 1;

				}/*else if (stream->udwFrameType == 2){	//P
					;
				}*/else{
					break;//continue;
				}

				//
				//HI_S_AVFrame *pFrame = (HI_S_AVFrame *)(stream->ucArrayBuffer + 16);	//skip HI_S_SysHeader(16 bytes)
#if 0
				PES pes(stream->udwBuffSize - 32);	//equal to pFrame->u32AVFrameLen?
				pes.Fill(stream->ucArrayBuffer + 32, ESDT_VIDEO, stream->udwFrameType == 1);
				//pes.SetPTS(mMuxer->FrameCount() * 4000);
				//pes.SetPTS((stream->dwTimeTag * 1000000 + stream->dwMillisTag) * 0.09 /*90000.0 / 1000000.*/);
				//printf("pFrame->u32AVFramePTS: %d(%x) \n", pFrame->u32AVFramePTS, pFrame->u32AVFramePTS);
				pes.SetPTS(pFrame->u32AVFramePTS * 90);
				this->mMuxer->Mux(mStreamVideo, pes.Data(), pes.Size());
#endif
				mpPlaylist->LatestMedia()->FrameCount() += 1;
				mMuxer->FrameCount()++;

			}while(false);

			///pos += (stream->udwBuffSize + sizeof(TLVSTREAMINFO) - 4);	//size[i];
		}

		Sleep(5);
	}

	//ErasePlaylist();

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