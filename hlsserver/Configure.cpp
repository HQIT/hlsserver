#include "StdAfx.h"
#include "Configure.h"

#include "json/json.h"

#include <fstream>

CConfigure::CConfigure(void) : LoggerEnable(false)
{
}

CConfigure::~CConfigure(void)
{
}

CConfigure *CConfigure::mInstance = NULL;

CConfigure *CConfigure::GetInstance(){
	if(mInstance == NULL){
		mInstance = new CConfigure();
	}

	return mInstance;
}

int CConfigure::Load(std::string &path){

	Json::Value root;
	Json::Reader reader;

	std::ifstream ifs;
	ifs.open(path);
	if(!ifs.is_open()){
		return -1;
	}

	if(!reader.parse(ifs, root)){
		return -2;
	}

	WebContextAddress = root["web_context_address"].asString();
	PlaylistPath = root["playlist_path"].asString();
	MediasPath = root["medias_path"].asString();
	PlaylistAbsolutePath = root["playlist_absolute_path"].asString();
	MediasAbsolutePath = root["medias_absolute_path"].asString();
	
	LoggerEnable = false;
	Json::Value val = root["logger_enable"];
	if(!val.isNull()){
		LoggerEnable = val.asBool();
	}

	PlaylistCapacity = 2;	//default value
	val = root["playlist_capacity"];
	if(!val.isNull()){
		PlaylistCapacity = val.asInt();
	}

	SessionTTL = 30 * 1000;
	val = root["session_ttl"];
	if(!val.isNull()){
		SessionTTL = val.asInt() * 1000;
	}

	ifs.close();

	return 0;
}