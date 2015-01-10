#include "audio.h"
#include <fstream>

#include <cstdio>
#ifdef D2DEVICE
#pragma comment(lib, "portaudio_x86")
#include <portaudio.h>
#endif

#include "../events/factory.h"

AudioM::AudioM(){
#ifdef D2DEVICE
	PaError error = Pa_Initialize();
	if (error != paNoError){
		printf("PortAudio Error: %s\n", Pa_GetErrorText(error));
		throw - 1;
	}
#endif
	currentFile = NULL;
}

void AudioM::PlayFile(std::string filepath, bool loop){
	{
		std::ifstream fileTest(filepath);
		if (!fileTest){
			printf("File could not be loaded.\n");
			return;
		}
	}

	printf("Decoding/Reading: %s\n", filepath.c_str());

	AudioFile* nFile = new AudioFile(filepath);
	nFile->looping = loop;
	if (fileQueue.size() <= 0){
		printf("Assigned current file.\n");
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
#ifdef D2DEVICE
	Pa_Terminate();
#endif
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

#ifdef D2DEVICE
int GblStreamCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData){
	AudioM* manager = (AudioM*)userData;
	manager->AudioData(input, output, frameCount, timeInfo->currentTime);
	return 0;
}

bool AudioM::StartStream(DeviceDetails details){
	//PaStreamParameters inputParams;
	PaStreamParameters outputParams;

	const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(details.deviceNum);
	printf("Input Channels: %i\n", devInfo->maxInputChannels);
	printf("Output Channels: %i\n", devInfo->maxOutputChannels);

	/*inputParams.channelCount = 1;
	inputParams.device = details.deviceNum;
	inputParams.hostApiSpecificStreamInfo = NULL;
	inputParams.sampleFormat = paFloat32;
	inputParams.suggestedLatency = devInfo->defaultLowInputLatency;*/

	outputParams.channelCount = 2;
	outputParams.device = details.deviceNum;
	outputParams.hostApiSpecificStreamInfo = NULL;
	outputParams.sampleFormat = paInt16;
	outputParams.suggestedLatency = devInfo->defaultLowOutputLatency;

	PaError error = Pa_IsFormatSupported(NULL, &outputParams, TARGET_SAMPLE_RATE);
	if (error != paFormatIsSupported){
		printf("Not Supported: %s\n", Pa_GetErrorText(error));
		return false;
	}

	PaStream* stream;
	error = Pa_OpenStream(&stream, NULL, &outputParams, TARGET_SAMPLE_RATE, paFramesPerBufferUnspecified, paNoFlag, GblStreamCallback, (void*) this);
	if (error != paNoError){
		printf("Audio Stream Failed: %s\n", Pa_GetErrorText(error));
	}

	Pa_StartStream(stream);
	Pa_Sleep(2000);

	StreamPtr = stream;
	
	return true;
}

void AudioM::StopStream(){
	PaStream* stream = (PaStream*)StreamPtr;
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
}

#else

bool AudioM::StartStream(DeviceDetails details){
	return false;

}

void AudioM::StopStream(){

}

#endif

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

#ifdef D2DEVICE
std::vector<DeviceDetails> AudioM::GetDeviceList(){
	int numDevices = Pa_GetDeviceCount();
	std::vector<DeviceDetails> dList;
	dList.reserve(numDevices);
	for (int i = 0; i < numDevices; i++){
		const PaDeviceInfo *deviceInfo;
		deviceInfo = Pa_GetDeviceInfo(i);
		DeviceDetails nDetails;
		nDetails.deviceName = std::string(Pa_GetHostApiInfo(deviceInfo->hostApi)->name) + std::string(deviceInfo->name);
		nDetails.deviceNum = i;
		dList.push_back(nDetails);
	}
	return dList;
}

#endif

static EventsF::EventCreatorType<AudioFileEvent> snd("snd");

AudioFileEvent::AudioFileEvent(){
	eventActive = false;
	doLoop = false;
}

AudioFileEvent::~AudioFileEvent(){
	if (!eventActive) return;
	delete mData;
}

void AudioFileEvent::RunEvent(){
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

void SeekToEvent::RunEvent(){
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

void NextTrackEvent::RunEvent(){
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

void StopTracksEvent::RunEvent(){
	AudioM::getAudioManager()->StopTrack();
}