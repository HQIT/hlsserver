#include "StdAfx.h"
#include "CBasePlaylist.h"

#include "Media.h"

CBasePlaylist::CBasePlaylist(unsigned int capacity)
	: mSequence(0), mTargetDuration(1), mVersion(3)	//version 3 can use float duration in EXTINF tag
	, mCapacity(capacity),
	mEnd(false)
{
	;
}

CBasePlaylist::~CBasePlaylist(){
	Clear();
}

void CBasePlaylist::Clear(){
	MediaVector::iterator it = mMedias.begin();

	while(!mMedias.empty()){
		this->Pop();
	}
	mMedias.clear();
}

void CBasePlaylist::AddMedia(CMedia* pMedia){
	pMedia->Index(mSequence++);

	mMedias.push_back(pMedia);
	if(mMedias.size() > mCapacity){
		this->Pop();
	}
}

CMedia *CBasePlaylist::LatestMedia(){
	if(mMedias.empty())
		return NULL;

	return mMedias.back();
}

CMedia *CBasePlaylist::FirstMedia(){
	if(mMedias.empty())
		return NULL;

	return mMedias.front();
}

void CBasePlaylist::Pop(){

	//delete the file
	CMedia *m = *(mMedias.begin());

	std::string fullname
		= CConfigure::GetInstance()->MediasAbsolutePath + "/"
		//+ m->AccessToken() + "/"
		+ m->SourceId() + "/"
		+ m->Filename();

	DeleteFileA(fullname.c_str());

	mMedias.erase(mMedias.begin());
}

std::string CBasePlaylist::TargetDurationLine(){
	char line[128] = {0};
	//int size = _snprintf(line, sizeof(line), "#EXT-X-TARGETDURATION:%d", mTargetDuration);
	CMedia *pm = FirstMedia();
	int size = _snprintf(line, sizeof(line), "#EXT-X-TARGETDURATION:%d", pm ? pm->Duration() : 0);
	
	line[size] = 0;

	return std::string(line);
}

std::string CBasePlaylist::HeaderLine(){
	return std::string("#EXTM3U");
}

std::string CBasePlaylist::AllowCacheLine(){
	char line[128] = {0};
	int size = _snprintf(line, sizeof(line), "#EXT-X-ALLOW-CACHE:%s", 
		AllowCache() ? "YES" : "NO");

	line[size] = 0;

	return std::string(line);
}

std::string CBasePlaylist::SequenceLine(){
	char line[128] = {0};

	int size = _snprintf(line, sizeof(line), 
		"#EXT-X-MEDIA-SEQUENCE:%d", 
		mMedias.empty() ? 0 : mMedias.front()->Index());

	line[size] = 0;

	return std::string(line);
}

std::string CBasePlaylist::MediasLine(){
	std::string ret = "";

	MediaVector::iterator it = mMedias.begin();
	while(it != mMedias.end()){

		ret += (*it)->ToString();
		ret += std::string("\n\r");

		++it;
	}

	return ret;
}

std::string CBasePlaylist::VersionLine(){
	char line[128] = {0};
	int size = _snprintf(line, sizeof(line), "#EXT-X-VERSION:%d", 
		mVersion);

	line[size] = 0;

	return std::string(line);
}

std::string CBasePlaylist::ToString(){
	return HeaderLine() + "\n\r"
		+ AllowCacheLine() + "\n\r"
		+ VersionLine() + "\n\r"
		+ TargetDurationLine() +"\n\r"
		+ SequenceLine() + "\n\r"
		+ MediasLine() + "\n\r"
		+ (mEnd ? "#EXT-X-ENDLIST\n\r" : "");
}