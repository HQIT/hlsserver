#pragma once

#include "CBasePlaylist.h"
#include "ProxySource.h"
class CUserSession;

using namespace com::cloume::cap::streaming;

struct FileWriteTask
{
	std::string fileName;
	std::vector<BYTE> data;
};

namespace lelink{

#pragma pack(push)
#pragma pack(1)
	///AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM
	typedef struct MP3FrameHeader {
		unsigned int sync : 11;                         //同步信息
		unsigned int version : 2;                       //版本
		unsigned int layer : 2;                         //层
		unsigned int errorProtection : 1;               // CRC校验
		unsigned int bitrateIndex : 4;             //位率
		unsigned int samplingFrequency : 2;        //采样频率
		unsigned int padding : 1;                  //帧长调节
		unsigned int private1: 1;				//保留字
		unsigned int mode : 2;                  //声道模式
		unsigned int modeExtension : 2;			//扩充模式
		unsigned int copyright : 1;             // 版权
		unsigned int original : 1;				//原版标志
		unsigned int emphasis : 2;				//强调模式
	}MP3HEADER, *LPMP3HEADER;
#pragma pack(pop)

	class CStreaming{
	public:
		enum StreamingType{
			ST_REALTIME = 0,
			ST_HISTORY
		};

	private:
		int mUsing;	//how many session is using this stream

		std::string mSourceId;	//history clip id | camera device id

		HANDLE mStreamingThread;
		bool mIsStreaming;
		StreamingType mStreamingType;
		int startTime;
		static int realPacketsDeliverer(const unsigned char*, const unsigned long, void*);
#if 0
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
		
		//skip HI_S_SysHeader

		struct HI_S_AVFrame
		{
			unsigned int u32AVFrameFlag;                /* 帧标志 */
			unsigned int u32AVFrameLen;                 /* 帧的长度 */
			unsigned int u32AVFramePTS;                 /* 时间戳 */
			unsigned int u32VFrameType;                 /* 视频的类型，I帧或P帧 */
		};
#endif
		CBasePlaylist *mpPlaylist;
		static const unsigned long MAX_FRAME_BUFFER_SIZE = 1 << 21;	//2M
		unsigned char *mpFrameBuffer;

		MP2TMuxer *mMuxer;	//muxer will do the program and streams cleaning
		Program::Stream *mStreamAudio;
		Program::Stream *mStreamVideo;
		Program *mProgram;

		std::vector<FileWriteTask*> _fileWriteTasks;
		HANDLE _fileWriteEvent;
		CRITICAL_SECTION _fileLock;
		CProxySource *mpSource;		///H264裸流来源

	private:
		//ProxyUserClient *UserProxy();
		CProxySource *Source(){ return mpSource; }

		std::string PlaylistFilename();

	public:
		//CStreaming(std::string srcId, ProxyUserClient *, StreamingType st = ST_REALTIME);
		//CStreaming(std::string srcId, CUserSession *, StreamingType st = ST_REALTIME);
		//CStreaming(std::string srcId, void *, StreamingType st = ST_REALTIME>
		CStreaming(SOCKET socket, StreamingType st = ST_REALTIME);
		virtual ~CStreaming();

		const std::string &SourceId(){ return mSourceId; }
		int Using(){ return mUsing; }

		std::string PlaylistURL();
		std::string StreamingAbsolutePath();
		std::string CurrentMediaFilename();

		void Enqueue(FileWriteTask* task);

		void FileWriteThread();
		///void Reset();
		void Start();
		void Stop();

		StreamingType Type(){ return mStreamingType; }

	protected:
		static DWORD WINAPI StreamingStarter(void* param);
		virtual unsigned long Streaming();	//thread body

		int FlushPlaylist();
		int ErasePlaylist();
	};
}