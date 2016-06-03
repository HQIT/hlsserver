#pragma once

#include <vector>
#include <ostream>

class CMedia;

class CBasePlaylist
{
public:
	CBasePlaylist(unsigned int capacity = 2);
	virtual ~CBasePlaylist();

	virtual void AddMedia(CMedia*);
	//virtual void RemoveMedia(unsigned int index);
	
	void SetEnd(bool end = true){ mEnd = end; }
	bool End(){ return mEnd; }

	virtual void Clear();

	CMedia *LatestMedia();
	CMedia *FirstMedia();
	//virtual bool Ready(){ return mMedias.size() == mCapacity; }
	virtual bool Ready(){ return mMedias.size() > 0 || mEnd; }

	std::string ToString();

private:
	void Pop();

protected:
	std::string HeaderLine();
	std::string TargetDurationLine();
	std::string AllowCacheLine();
	std::string SequenceLine();
	std::string MediasLine();
	std::string VersionLine();

	virtual bool AllowCache(){ return false; }

	unsigned int Size(){ return mMedias.size(); }

private:
	typedef std::vector<CMedia*> MediaVector;
	MediaVector mMedias;

	bool mEnd;

	unsigned int mTargetDuration;
	unsigned int mSequence;
	unsigned int mVersion;

protected:
	unsigned int mCapacity;
};

