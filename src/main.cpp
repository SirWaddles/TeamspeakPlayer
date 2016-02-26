#include <string>
#include <cerrno>
#include "audio/audio.h"
#include "events/factory.h"
#include <curl/curl.h>
#include "util.h"
#include <cstdio>
#include <cstdlib>
#include "lua_m/lua_m.h"
#include <memory>
#include <thread>
#include <iostream>
#include <sstream>
#include <string>
#include <flite.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

typedef void(*CaptureCallback)(void*);

DLLEXPORT void StartCapture();
DLLEXPORT void StopCapture();
DLLEXPORT void SetCaptureCallback(CaptureCallback);
DLLEXPORT void SetTextCallback(CaptureCallback);
DLLEXPORT void TextMessageParse(const char* message);

std::thread* AudioThread;

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
	if (!textCallback) return;
	textCallback((void*)msg.c_str());
}

void TextMessageParse(const char* message){

	printf("Message: %s\n", message);

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
	curl_global_init(CURL_GLOBAL_ALL);
	flite_init();
	LuaManager::GetLuaManager();
	AudioM* mMan = AudioM::getAudioManager();
	mMan->SetupFileRenderer();

	StartTimeCounter();

	EventManager::getEventManager()->BuildEvents();
	AudioThread = new std::thread(AudioThreadF);
}

void StopCapture(){
	// TODO: I'll fix the race condition later.
	printf("Audio Stopping\n");
	currentCallback = NULL;
	AudioThread->join();
	delete AudioThread;
	printf("Audio stopped\n");

	curl_global_cleanup();
	delete EventManager::getEventManager();
	delete AudioM::getAudioManager();
	delete LuaManager::GetLuaManager();
}

#ifdef TSPLAYER_DEBUG
void playAudioData(void* audioData) {
	//printf("Sending data\n");
}

void printTextMessage(void* msg) {
	char* messageContents = (char*)msg;
	printf("Text: %s\n", messageContents);
}

int main(int argc, char** argv) {
	SetCaptureCallback(playAudioData);
	SetTextCallback(printTextMessage);
	StartCapture();

	char entry[255];
	while (strcmp("exit", entry) != 0) {
		std::cin.getline(entry, 255);
		TextMessageParse(entry);
	}

	StopCapture();
	return 0;
}
#endif