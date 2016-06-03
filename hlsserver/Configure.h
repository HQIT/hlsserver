#pragma once

#include <string>

class CConfigure
{
public:
	std::string WebContextAddress;
	std::string PlaylistPath;
	std::string MediasPath;
	std::string PlaylistAbsolutePath;
	std::string MediasAbsolutePath;
	int SessionTTL;

	int PlaylistCapacity;

	bool LoggerEnable;

public:
	static CConfigure *GetInstance();

public:
	int Load(std::string &) throw();

private:
	CConfigure(void);
	virtual ~CConfigure(void);

private:
	static CConfigure *mInstance;
};

