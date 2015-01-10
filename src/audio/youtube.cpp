#include "../events/factory.h"
#include "../lua_m/lua_m.h"
#include "../util.h"
//#include <boost/network.hpp>
#include "../client/http_client.h"
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

void YoutubeEvent::RunEvent(){
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

	std::unique_ptr<HTTPClient> httpClient(GetNewClient());
	
	/*boost::network::http::client client;
	boost::network::http::client::request request("http://www.youtube.com/watch?v=" + ytUrl);
	request << boost::network::header("Connection", "close");
	boost::network::http::client::response response = client.get(request);
	lM->lineReader = response.body();*/

	httpClient->RunRequest("http://www.youtube.com/watch?v=" + ytUrl);
	httpClient->RunBody();
	lM->lineReader = httpClient->WriteToString();

	lM->lineReaderChar = 0;
	lM->RunFile(std::string(getenv("APPDATA")) + "/tsbot/youtube.lua");
	lM->RunLuaFunc("SetYTURL", ytWholeURL);
	lM->RunLuaFunc("GDoFinalEVal");

	std::string networkPath = lM->GetLuaFuncStr();
	printf("Downloading Video\n");

	lM->RunLuaFunc("GetVideoTitle");
	videoTitle = lM->GetLuaFuncStr();
	hasTitle = true;

	int bytesWritten = 0;
	{
		/*boost::network::http::client client;
		boost::network::http::client::request request(networkPath);
		boost::network::http::client::response response = client.get(request);*/

		httpClient->RunRequest(networkPath);
		//httpClient->RunHeaders();

		//int totalContentSize = httpClient->GetContentSize();
		//if (totalContentSize > 52428800){
			// Run message stream
		//	return;
		//}

		httpClient->RunBody();

		std::ofstream ofs(ytSavePath.c_str(), std::ios_base::binary | std::ios_base::out);
		//ofs << (boost::network::http::body(response));
		httpClient->WriteToStream(ofs);

		bytesWritten = ofs.tellp();
	}

	if (bytesWritten <= 5){
		AddToLog("Failed to load: " + ytUrl + ". Now removing\n");
		AddToLog("URL was:" + networkPath + "\n");
		remove(ytSavePath.c_str());
		return;
	}

	DoPlayFile();
}

void YoutubeEvent::DoPlayFile(){
	AudioFileEvent* PlayEvent = new AudioFileEvent();
	if (doLoop){
		PlayEvent->doLoop = true;
	}
	PlayEvent->SetupEvent("yt_vids/" + ytUrl + ".mp4");
	EventManager::getEventManager()->AddEvent(PlayEvent);
}