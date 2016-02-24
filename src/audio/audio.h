#ifndef __AUDIO_HEADER__
#define __AUDIO_HEADER__

#include <string>
#include <vector>
#include <list>
#include "../events/events.h"

#ifdef D2DEVICE
#define TARGET_SAMPLE_RATE 44100
#else
#define TARGET_SAMPLE_RATE 48000
#endif

struct DeviceDetails {
	std::string deviceName;
	int deviceNum;
};

class AudioPacket {
public:
	AudioPacket(int length);
	~AudioPacket();
	AudioPacket(AudioPacket&& arg);
	AudioPacket& operator=(AudioPacket&& arg);

	int getLength();
	unsigned char* getData();

	int packetID;
private:
	unsigned char* data;
	int length;
};

class AudioFile;

class AudioPacketQueue {
public:
	void ClearQueue();
	void SeekTo(int packet);
	int size();

	void AddPacket(AudioPacket& nPacket);

	short IncrementPacket();
	AudioPacketQueue(AudioFile* fileOwner);
	~AudioPacketQueue();

private:
	std::vector<AudioPacket> mPackets;
	AudioFile* ownerFile;
	int readHead;
	int currentPacket;
};

class FFMpegAudioFile;

class AudioFile {
public:
	AudioFile();
	virtual ~AudioFile();
	
	void GetNextData(short* outL, short* outR);
	void FinishTrack();

	void SeekTo(double seconds);

	bool looping;
protected:

	double duration;
	int frames;
	int currentFrame;
	int stream;
	AudioPacketQueue* lPacketQueue;
	AudioPacketQueue* rPacketQueue;

	bool TrackOver;
};

class AudioFileEncoded : public AudioFile {
public:
	AudioFileEncoded(std::string filepath);
	virtual ~AudioFileEncoded();
	bool readFrame();
private:
	FFMpegAudioFile* extDets;
};

class AudioFileData : public AudioFile {
public:
	AudioFileData(int num_samples, short* samples);
	virtual ~AudioFileData();
};

class AudioM {
public:
	AudioM();
	~AudioM();
	void AudioData(const void* input, void* output, unsigned long frameCount, double time);
	bool StartStream(DeviceDetails device);
	void StopStream();
	void StopTrack();
	void ClearQueue();
	void SetupFileRenderer();
	void PlayFile(std::string filepath, bool loop = false);
	void PlayData(int num_samples, short* samples);
	void SeekTo(double seconds);
	void NextTrack();

	static AudioM* getAudioManager();
private:
	void* StreamPtr;

	std::list<AudioFile*> fileQueue;
	AudioFile* currentFile;

	static AudioM* mainAudioManager;
};


class TrackDeleteEvent : public IThreadEvent {
public:
	TrackDeleteEvent(AudioFile* td);
	virtual void RunEvent();
private:
	AudioFile* toDelete;
};


#endif // __AUDIO_HEADER__