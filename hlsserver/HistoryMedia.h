#pragma once
#include "Media.h"

class CHistoryMedia :
	public CMedia
{
public:
	//CHistoryMedia(int clipId);
	explicit CHistoryMedia(const std::string &clipId);
	virtual ~CHistoryMedia(void);

	virtual std::string Filename(){
		char name[64] = {0};

		int n = _snprintf(name, sizeof(name), "clip%s_%d.ts", SourceId().c_str(), Index());
		name[n] = 0;

		return std::string(name);
	}

private:
	//int mClipId;
	std::string mClipId;
};

