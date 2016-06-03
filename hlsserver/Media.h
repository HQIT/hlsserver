#pragma once

#include "Configure.h"

#pragma warning (disable: 4996)

class CMedia{
public:
	explicit CMedia(const std::string &srcId) : mIndex(0), 
		mStartTimestamp(0), mEndTimestamp(0),
		mGOPCount(0),
		mFrameCount(0),
		mSourceId(srcId)
	{}
	virtual ~CMedia(){}

public:
	void Index(unsigned int index){ mIndex = index; }
	unsigned int Index(){ return mIndex; }

	unsigned int &GOP(){ return mGOPCount; }
	unsigned int &FrameCount(){ return mFrameCount; }

	unsigned int Duration() { 
//#pragma warning (disable: 4244)
		//return /*mDuration*/(unsigned int)(mEndTimestamp - mStartTimestamp); 
		//printf("segment duration: %f \n", mFrameDuration * mFrameCount);
		return (int)ceil(mFrameDuration * mFrameCount);
	}
	std::string URL(std::string &urlBase){ return urlBase + Filename(); }
	//void AccessToken(std::string &at){ mAccessToken = at; }
	//std::string &AccessToken(){ return mAccessToken; }
	std::string &SourceId(){ return mSourceId; }

	void StartTimestamp(unsigned long long ts){ mStartTimestamp = ts; }
	void EndTimestamp(unsigned long long ts){ mEndTimestamp = ts; }

	std::string ToString(){

		std::string urlBase 
			= CConfigure::GetInstance()->WebContextAddress 
			+ CConfigure::GetInstance()->MediasPath + "/"
			//+ mAccessToken + "/";
			+ mSourceId + "/";

		char line[1024] = {0};
		int size = _snprintf(line, sizeof(line), 
			"#EXTINF:%d,\n\r%s",
			this->Duration(),
			this->URL(urlBase).c_str());

		line[size] = 0;
		return std::string(line);
	}

	virtual std::string Filename(){
		char name[64] = {0};

		int n = _snprintf(name, sizeof(name), "media_%d.ts", mIndex);
		name[n] = 0;

		return std::string(name);
	}

private:
	static const double mFrameDuration;

	unsigned int mGOPCount;
	unsigned int mFrameCount;
	unsigned int mIndex;
	//unsigned int mDuration;
	unsigned long long mStartTimestamp, mEndTimestamp;
	//std::string mURL;
	//std::string mAccessToken;
	std::string mSourceId;
};