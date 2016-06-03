#pragma once

#include "CBasePlaylist.h"
#include "MP2TMuxer.h"
#include "ProxySource.h"

class CUserSession;

namespace lelink{
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
		unsigned char *mpFramesBuffer;

		MP2TMuxer *mMuxer;	//muxer will do the program and streams cleaning
		Program::Stream *mStreamAudio;
		Program::Stream *mStreamVideo;
		Program *mProgram;

		CProxySource *mpSource;		///H264裸流来源

	private:
		//ProxyUserClient *UserProxy();
		CProxySource *Source(){ return mpSource; }

		std::string PlaylistFilename();

	public:
		//CStreaming(std::string srcId, ProxyUserClient *, StreamingType st = ST_REALTIME);
		//CStreaming(std::string srcId, CUserSession *, StreamingType st = ST_REALTIME);
		//CStreaming(std::string srcId, void *, StreamingType st = ST_REALTIME);
		CStreaming(SOCKET socket, StreamingType st = ST_REALTIME);
		virtual ~CStreaming();

		const std::string &SourceId(){ return mSourceId; }
		int Using(){ return mUsing; }

		std::string PlaylistURL();
		std::string StreamingAbsolutePath();
		std::string CurrentMediaFilename();

		void Reset();
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