#ifndef __AUDIO_HEADER__
#define __AUDIO_HEADER__

#include <string>
#include <vector>
#include <list>
#include <mutex>
#include <atomic>
#include "../events/events.h"

#define TARGET_SAMPLE_RATE 48000

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
	
	virtual void GetAllData(short* outData, unsigned long frameCount);
	void FinishTrack();
	void SeekTo(double seconds);

	bool IsTrackOver();

	bool looping;
protected:

	void GetNextData(short* outL, short* outR);

	double duration;
	int frames;
	int currentFrame;
	int stream;
	AudioPacketQueue* lPacketQueue;
	AudioPacketQueue* rPacketQueue;

	std::atomic<bool> TrackOver;
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
	void StopTrack();
	void ClearQueue();
	void SetupFileRenderer();
	void SeekTo(double seconds);
	void AddFile(AudioFile* file);
	AudioFile* GetCurrentTrack();

	static AudioM* getAudioManager();
private:
	void NextTrack();

	void* StreamPtr;
	std::mutex mainLock;

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