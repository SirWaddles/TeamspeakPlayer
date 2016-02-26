#include "audio.h"
#include <fstream>

#include <cstdio>
#ifdef D2DEVICE
#pragma comment(lib, "portaudio_x86")
#include <portaudio.h>
#endif

#include "../events/factory.h"

AudioM::AudioM(){
	currentFile = nullptr;
}

void AudioM::AddFile(AudioFile* file) {
	std::lock_guard<std::mutex> lock(mainLock);
	if (fileQueue.size() <= 0) {
		printf("Assigned current file.\n");
		currentFile = file;
	}
	fileQueue.push_back(file);
}

AudioFile* AudioM::GetCurrentTrack() {
	if (fileQueue.size() <= 0) {
		return nullptr;
	}
	return fileQueue.front();
}

void AudioM::NextTrack(){
	if (fileQueue.size() <= 0){
		return;
	}

	std::lock_guard<std::mutex> lock(mainLock);
	AudioFile* lastTrack = fileQueue.front();
	fileQueue.pop_front();
	currentFile = nullptr;
	EventManager::getEventManager()->AddEvent(new TrackDeleteEvent(lastTrack));
	if (fileQueue.size() <= 0){
		return;
	}
	currentFile = fileQueue.front();
}

void AudioM::StopTrack(){
	if (fileQueue.size() <= 0){
		return;
	}
	currentFile = nullptr;

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

bool AudioFile::IsTrackOver() {
	return TrackOver;
}

void AudioFile::GetNextData(short* outL, short* outR){
	*outL = lPacketQueue->IncrementPacket();
	*outR = rPacketQueue->IncrementPacket();
}

void AudioFile::GetAllData(short* outData, unsigned long frameCount) {
	for (unsigned int i = 0; i < frameCount; i++) {
		if (TrackOver) {
			std::memset(outData, 0, (frameCount - i) * 4);
			break;
		}

		short* outL = outData++;
		short* outR = outData++;
		GetNextData(outL, outR);
	}
}

void AudioFile::FinishTrack() {
	if (TrackOver) return;
	TrackOver = true;
}

void AudioFile::OutOfData() {
	if (looping) {
		lPacketQueue->SeekTo(0);
		rPacketQueue->SeekTo(0);
	} else {
		FinishTrack();
	}
}

void AudioFile::SeekTo(double seconds) {
	double target = (double(frames) / duration) * seconds;
	lPacketQueue->SeekTo((int)target);
	rPacketQueue->SeekTo((int)target);
}

void AudioM::AudioData(const void* input, void* output, unsigned long frameCount, double time){
	short* out = (short*)output;

	if (!currentFile){
		std::memset(output, 0, frameCount * 4);
		return;
	}

	currentFile->GetAllData(out, frameCount);

	if (currentFile->IsTrackOver()) {
		NextTrack();
	}
}

short AudioPacketQueue::IncrementPacket() {
	std::unique_lock<std::mutex> lockGuard(packetLock, std::try_to_lock);
	if (!lockGuard.owns_lock()) {
		return 0;
	}
	if (currentPacket >= mPackets.size()) {
		ownerFile->OutOfData();
		return 0;
	}
	AudioPacket* packet = &mPackets[currentPacket];
    readHead += 2;
	if (readHead >= packet->getLength()) {
		currentPacket++;
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
	std::lock_guard<std::mutex> lockGuard(packetLock);
	mPackets.push_back(std::move(nPacket));
}

static EventsF::EventCreatorType<AudioFileEvent> snd("snd");

AudioFileEvent::AudioFileEvent(){
	eventActive = false;
	doLoop = false;
}

AudioFileEvent::~AudioFileEvent(){

}

void AudioFileEvent::RunSingleEvent(){
	if (!eventActive) return;
	AudioFileEncoded* nFile = new AudioFileEncoded(filePath);
	nFile->looping = doLoop;
	while (nFile->readFrame()) continue;
	AudioM::getAudioManager()->AddFile(nFile);
}

void AudioFileEvent::SetupArgs(std::deque<std::string>& args){
	if (args.size() <= 0){
		return;
	}
	filePath = std::string("storage/") + std::string(getenv("APPDATA")) + "/tsbot/" + args[0];
	{
		std::ifstream fileTest(filePath);
		if (!fileTest) {
			printf("File could not be loaded.\n");
			return;
		}
	}
	if (args.size() > 1 && args[1] == "loop") {
		doLoop = true;
	}
	eventActive = true;
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
	AudioFile* currentFile = AudioM::getAudioManager()->GetCurrentTrack();
	if (currentFile) {
		currentFile->FinishTrack();
	}
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
