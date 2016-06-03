#pragma once
#include "CBasePlaylist.h"

class CHistoryPlaylist : public CBasePlaylist
{
public:
	CHistoryPlaylist(unsigned int capacity = 3);
	virtual ~CHistoryPlaylist(void);

	virtual bool Ready(){ return Size() >= this->mCapacity; }

protected:
	virtual bool AllowCache(){ return true; }
};

