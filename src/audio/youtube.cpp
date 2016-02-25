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

void YoutubeEvent::RunSingleEvent(){
	if (!eventActive){
		return;
	}

	LuaManager* lM = LuaManager::GetLuaManager();

	if (ytUrl.length() != 11){
		int vpos = ytUrl.find("v=");
		ytUrl = ytUrl.substr(vpos + 2, 11);
	}

	std::string ytSavePath(std::string(getenv("APPDATA")) + "/tsbot/yt_vids/" + ytUrl + ".mp4");

	if (doesFileExist(ytSavePath)){
		DoPlayFile();
		return;
	}

	std::string ytWholeURL("http://www.youtube.com/watch?v=" + ytUrl);
	
	/*boost::network::http::client client;
	boost::network::http::client::request request("http://www.youtube.com/watch?v=" + ytUrl);
	request << boost::network::header("Connection", "close");
	boost::network::http::client::response response = client.get(request);
	lM->lineReader = response.body();*/

	std::string url("https://www.youtube.com/watch?v=" + ytUrl);

	std::string respData = GetHTTPContents(url);

	lM->lineReader = respData;


	lM->lineReaderChar = 0;
	lM->RunFile(std::string(getenv("APPDATA")) + "/tsbot/youtube.lua");
	lM->RunLuaFunc("SetYTURL", ytWholeURL);
	lM->RunLuaFunc("GDoFinalEVal");

	fullVideoUrl = lM->GetLuaFuncStr();

	lM->RunLuaFunc("GetVideoTitle");
	videoTitle = lM->GetLuaFuncStr();
	hasTitle = true;
	DoPlayFile();
}

void YoutubeEvent::DoPlayFile(){
	AudioM::getAudioManager()->PlayFile(fullVideoUrl, doLoop);
	/*AudioFileEvent* PlayEvent = new AudioFileEvent();
	if (doLoop){
		PlayEvent->doLoop = true;
	}
	PlayEvent->SetupEvent("yt_vids/" + ytUrl + ".mp4");
	EventManager::getEventManager()->AddEvent(PlayEvent);*/
}