#include "../events/factory.h"
#include "../lua_m/lua_m.h"
#include "../util.h"
#include "../http/http.h"
#include "audio.h"
#include <memory>
#include <cstdio>
#include <iostream>
#include <fstream>

static EventsF::EventCreatorType<YoutubeEvent> yt("yt");

YoutubeEvent::YoutubeEvent(){
	eventActive = false;
	hasTitle = false;
	doLoop = false;
	ytFile = nullptr;
}

void YoutubeEvent::SetupArgs(std::deque<std::string>& args){
	if (args.size() <= 0){
		return;
	}
	ytUrl = args[0];
	eventActive = true;
	if (args.size() >= 2 && args[1] == "loop"){
		doLoop = true;
	}
}

std::string YoutubeEvent::GetEventMessage(){
	if (hasTitle){
		return "Playing: " + videoTitle;
	}
	if (!eventActive) {
		return "Video not received. Removed.";
	}
	return "Playing: " + ytUrl;
}

void YoutubeEvent::RunStartEvent(){
	if (!eventActive){
		return;
	}

	LuaManager* lM = LuaManager::GetLuaManager();

	if (ytUrl.length() != 11){
		int vpos = ytUrl.find("v=");
		ytUrl = ytUrl.substr(vpos + 2, 11);
	}

	std::string url("https://www.youtube.com/watch?v=" + ytUrl);
	std::string respData = GetHTTPContents(url);

	lM->lineReader = respData;
	lM->lineReaderChar = 0;
	lM->RunFile(std::string(getenv("APPDATA")) + "/tsbot/youtube.lua");
	lM->RunLuaFunc("SetYTURL", url);
	lM->RunLuaFunc("GDoFinalEVal");

	fullVideoUrl = lM->GetLuaFuncStr();

	lM->RunLuaFunc("GetVideoTitle");
	videoTitle = lM->GetLuaFuncStr();
	hasTitle = true;

	ytFile = new AudioFileEncoded(fullVideoUrl);
	// weird ownership transfer, there's probably a more RAII way to do this, but I'll worry about move semantics later
	ytFile->SetOriginator(this);
	AudioM::getAudioManager()->AddFile(ytFile);
}

void YoutubeEvent::RunEventLoop() {
	bool frameRead = ytFile->readFrame();
	if (!frameRead) {
		printf("Frame read. Stopping\n");
		StopEvent();
		printf("Stopped\n");
	}
}

YoutubeEvent::~YoutubeEvent() {
	// No-one owns me
	printf("Event destroy\n");
	ytFile->SetOriginator(nullptr);
	StopEvent();
}