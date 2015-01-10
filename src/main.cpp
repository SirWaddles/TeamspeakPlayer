#include <string>
#include <cerrno>
#include "server/server.hpp"
#include "audio/audio.h"
#include "events/factory.h"
#include "util.h"
#include <cstdio>
#include <cstdlib>
#include <boost/thread.hpp>
#include "lua_m/lua_m.h"
#include "client/http_client.h"
#include <memory>

#define DLLEXPORT extern "C" __declspec(dllexport)

typedef void(*CaptureCallback)(void*);

DLLEXPORT void StartCapture();
DLLEXPORT void StopCapture();
DLLEXPORT void SetCaptureCallback(CaptureCallback);
DLLEXPORT void SetTextCallback(CaptureCallback);
DLLEXPORT void TextMessageParse(const char* message);

class StopStreamEvent : public IThreadEvent {
public:
	void RunEvent();
	std::string GetEventMessage();
	void SetupArgs(std::deque<std::string>& args);
};

bool eventsRunning = false;

void StopStreamEvent::RunEvent(){
	eventsRunning = false;
}

std::string StopStreamEvent::GetEventMessage(){
	return "";
}

void StopStreamEvent::SetupArgs(std::deque<std::string>& args){

}

boost::thread* EventThread;
boost::thread* AudioThread;


void DecoderThreadF(){
	while (eventsRunning){
		ThreadSleep(20);
		EventManager::getEventManager()->RunEvents();
	}
}

CaptureCallback currentCallback = NULL;
CaptureCallback textCallback = NULL;

double lastAudioFrame = 0.0;

void AudioThreadF(){
	lastAudioFrame = GetTimeSinceStart();
	while (currentCallback){
		short* captureBuffer = new short[48 * 100 * 2];
		AudioM::getAudioManager()->AudioData(NULL, captureBuffer, 48 * 100, 0.0);
		currentCallback(captureBuffer);
		delete[] captureBuffer;

		double newTime = GetTimeSinceStart();
		int sleepTime = int(100.0 - (newTime - lastAudioFrame));
		lastAudioFrame = newTime + double(sleepTime);
		ThreadSleep(sleepTime);
	}
}

void SetCaptureCallback(CaptureCallback nBack){
	currentCallback = nBack;
}

void SetTextCallback(CaptureCallback nBack){
	textCallback = nBack;
}

void SendTextMessage(std::string msg){
	if (msg.length() <= 0) return;
#ifdef D2DEVICE
	printf("Text: %s\n", msg.c_str());
	return;
#endif
	if (!textCallback) return;
	textCallback((void*)msg.c_str());
}

void TextMessageParse(const char* message){

	if (message[0] != '!') return;
	message++;

	std::deque<std::string> args;
	std::istringstream f(message);
	std::string arg;
	while (std::getline(f, arg, ' ')){
		if (arg == "") continue;
		args.push_back(arg);
	}

	IThreadEvent* nEvent = EventsF::CreateNewEvent(args[0]);
	if (!nEvent){
		SendTextMessage("Could not find command: " + args[0]);
		return;
	}
	args.pop_front();
	nEvent->SetupArgs(args);
	EventManager::getEventManager()->AddEvent(nEvent);
	// TODO: Send chat messsage
}

void StartCapture(){
	// Opening lua
	LuaManager::GetLuaManager();
	AudioM* mMan = AudioM::getAudioManager();
	mMan->SetupFileRenderer();
	eventsRunning = true;

	EventThread = new boost::thread(DecoderThreadF);
	AudioThread = new boost::thread(AudioThreadF);

	StartTimeCounter();

	//YoutubeEvent* nEvent = new YoutubeEvent("5pidokakU4I");
	//EventManager::getEventManager()->AddEvent(nEvent);
}

void StopCapture(){
	// TODO: I'll fix the race condition later.
	printf("Audio Stopping\n");
	currentCallback = NULL;
	AudioThread->join();
	delete AudioThread;
	printf("Audio stopped\n");

	printf("Events stopping\n");
	EventManager::getEventManager()->AddEvent(new StopStreamEvent());
	EventThread->join();
	delete EventThread;
	printf("Events stopped\n");

	delete EventManager::getEventManager();
	delete AudioM::getAudioManager();
	delete LuaManager::GetLuaManager();
}

#ifdef D2DEVICE

int main(int argc, char** argv){

	StartCapture();

	AudioM* audioManager = AudioM::getAudioManager();
	std::vector<DeviceDetails> dList = audioManager->GetDeviceList();
	std::vector<DeviceDetails>::iterator it;
	for (it = dList.begin(); it < dList.end(); it++){
		printf("Device %i Name: %s\n", it->deviceNum, it->deviceName.c_str());
	}
	int selectDevice = 0;
	printf("Please select a device number.\n");
	std::cin >> selectDevice;
	while (audioManager->StartStream(dList[selectDevice]) != true){
		printf("Sorry, the audio device could not start. Please choose another.\n");
		std::cin >> selectDevice;
	}

	std::string message;
	while (message != "EXIT"){
		std::getline(std::cin, message);
		TextMessageParse(message.c_str());
	}
	return 0;
}

#endif