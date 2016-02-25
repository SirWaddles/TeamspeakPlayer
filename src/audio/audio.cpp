#include "audio.h"
#include <fstream>

#include <cstdio>
#ifdef D2DEVICE
#pragma comment(lib, "portaudio_x86")
#include <portaudio.h>
#endif

#include "../events/factory.h"

AudioM::AudioM(){
	currentFile = NULL;
}

void AudioM::PlayFile(std::string filepath, bool loop){
	/*{
		std::ifstream fileTest(filepath);
		if (!fileTest){
			printf("File could not be loaded.\n");
			return;
		}
	}*/

	printf("Decoding/Reading: %s\n", filepath.c_str());

	AudioFileEncoded* nFile = new AudioFileEncoded(filepath);
	nFile->looping = loop;
	if (fileQueue.size() <= 0){
		printf("Assigned current file.\n");
		currentFile = nFile;
	}
	fileQueue.push_back(nFile);

	int frames = 0;
	while (nFile->readFrame()) {
		frames++;
	}
}

void AudioM::PlayData(int num_samples, short* samples) {
	AudioFileData* nFile = new AudioFileData(num_samples, samples);
	nFile->looping = false;
	if (fileQueue.size() <= 0) {
		currentFile = nFile;
	}
	fileQueue.push_back(nFile);
}

void AudioM::NextTrack(){
	if (fileQueue.size() <= 0){
		return;
	}
	AudioFile* lastTrack = fileQueue.front();
	fileQueue.pop_front();
	if (fileQueue.size() <= 0){
		currentFile = NULL;
		return;
	}
	// Extremely bloody hacky, not sure what to do here.
	EventManager::getEventManager()->AddEvent(new TrackDeleteEvent(lastTrack));
	currentFile = fileQueue.front();
}

void AudioM::StopTrack(){
	if (fileQueue.size() <= 0){
		return;
	}
	currentFile = NULL;
	
	std::list<AudioFile*>::iterator it;
	for (it = fileQueue.begin(); it != fileQueue.end(); it++){
		AudioFile* file = (*it);
		delete file;
	}
	fileQueue.clear();
}

void AudioM::SeekTo(double seconds){
	if (!currentFile) return;
	currentFile->SeekTo(seconds);
}

AudioM::~AudioM(){
	std::list<AudioFile*>::iterator it;
	for (it = fileQueue.begin(); it != fileQueue.end(); it++){
		delete (*it);
	}
	fileQueue.clear();
}

AudioM* AudioM::mainAudioManager = NULL;

AudioM* AudioM::getAudioManager(){
	if (!mainAudioManager){
		mainAudioManager = new AudioM();
	}
	return mainAudioManager;
}

void AudioM::ClearQueue(){

}

bool AudioM::StartStream(DeviceDetails details){
	return false;

}

void AudioM::StopStream(){

}


void AudioFile::GetNextData(short* outL, short* outR){
	*outL = lPacketQueue->IncrementPacket();
	*outR = rPacketQueue->IncrementPacket();
}

void AudioM::AudioData(const void* input, void* output, unsigned long frameCount, double time){
	// MAY BE RUNNING ON SECONDARY THREAD. VERY TIME-CRITICAL, PRE-PROCESS AS MUCH AS POSSIBLE.
	short* out = (short*)output;

	if (!currentFile){
		std::memset(output, 0, frameCount * 4);
		return;
	}

	for (unsigned int i = 0; i < frameCount; i++){
		short* outL = out++;
		short* outR = out++;
		currentFile->GetNextData(outL, outR);
	}
}

short AudioPacketQueue::IncrementPacket(){
	AudioPacket* packet = &mPackets[currentPacket];
	readHead += 2;
	if (readHead >= packet->getLength()){
		currentPacket++;
		if (currentPacket >= mPackets.size()){
			currentPacket = 0;
			ownerFile->FinishTrack();
		}
		packet = &mPackets[currentPacket];
		readHead = 0;
	}
	return *((short*)(&(packet->getData()[readHead])));
}

int AudioPacketQueue::size() {
	return mPackets.size();
}

static int mainPacketID = 0;

AudioPacket::AudioPacket(int alength){
	packetID = mainPacketID;
	mainPacketID++;
	data = new unsigned char[alength + 2];
	length = alength;
}

AudioPacket::AudioPacket(AudioPacket&& arg){
	data = arg.getData();
	length = arg.getLength();
	packetID = arg.packetID;
	arg.data = NULL;
}

AudioPacket& AudioPacket::operator=(AudioPacket&& arg){
	data = arg.getData();
	length = arg.getLength();
	arg.data = NULL;
	return *this;
}

AudioPacket::~AudioPacket(){
	if (!data) return;
	delete[] data;
}

int AudioPacket::getLength(){
	return length;
}

unsigned char* AudioPacket::getData(){
	return data;
}

AudioPacketQueue::AudioPacketQueue(AudioFile* fileOwner){
	readHead = 0;
	currentPacket = 0;
	ownerFile = fileOwner;
}

AudioPacketQueue::~AudioPacketQueue(){

}

void AudioPacketQueue::ClearQueue(){
	mPackets.clear();
}

void AudioPacketQueue::SeekTo(int packet){
	if (packet >= mPackets.size()){
		return;
	}
	currentPacket = packet;
	readHead = 0;
}

void AudioPacketQueue::AddPacket(AudioPacket& nPacket){
	mPackets.push_back(std::move(nPacket));
}

static EventsF::EventCreatorType<AudioFileEvent> snd("snd");

AudioFileEvent::AudioFileEvent(){
	eventActive = false;
	doLoop = false;
}

AudioFileEvent::~AudioFileEvent(){
	if (!eventActive) return;
	delete mData;
}

void AudioFileEvent::RunSingleEvent(){
	if (!eventActive) return;
	AudioM::getAudioManager()->PlayFile(mData->filePath, doLoop);
}

void AudioFileEvent::SetupEvent(std::string filepath){
	eventActive = true;
	AudioEventData* data = new AudioEventData();
	data->filePath = std::string(getenv("APPDATA")) + "/tsbot/" + filepath;
	mData = data;
}

void AudioFileEvent::SetupArgs(std::deque<std::string>& args){
	if (args.size() <= 0){
		return;
	}
	SetupEvent("storage/" + args[0]);
}

std::string AudioFileEvent::GetEventMessage(){
	if (!eventActive) return "Audio event failed.";
	return "Playing audio file.";
}

static EventsF::EventCreatorType<SeekToEvent> seekto("seekto");

void SeekToEvent::RunSingleEvent(){
	AudioM::getAudioManager()->SeekTo(seekTo);
}

void SeekToEvent::SetupArgs(std::deque<std::string>& args){
	seekTo = std::stoi(args[0]);
}

std::string SeekToEvent::GetEventMessage(){
	return "Seeking";
}

static EventsF::EventCreatorType<NextTrackEvent> next("next");

NextTrackEvent::NextTrackEvent(){
	overrideExit = false;
}

void NextTrackEvent::RunSingleEvent(){
	AudioM::getAudioManager()->NextTrack();
}

void NextTrackEvent::SetupArgs(std::deque<std::string>& args){
	overrideExit = true;
}

std::string NextTrackEvent::GetEventMessage(){
	return "Next Track";
}

void NextTrackEvent::OverrideExit(){
	overrideExit = true;
}

static EventsF::EventCreatorType<StopTracksEvent> stoptracks("stop");

std::string StopTracksEvent::GetEventMessage(){
	return "Stopping Tracks";
}

void StopTracksEvent::RunSingleEvent(){
	AudioM::getAudioManager()->StopTrack();
}