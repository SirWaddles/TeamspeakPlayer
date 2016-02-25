#ifndef __EVENT_HEADER__
#define __EVENT_HEADER__

#include <string>
#include <list>
#include <deque>

class IThreadEvent {
public:
	virtual void SetupArgs(std::deque<std::string>& args) ;
	virtual std::string GetEventMessage();
	virtual ~IThreadEvent(){}
	virtual void RunEvent() = 0;
};

class ISingleEvent : public IThreadEvent {
public:
	virtual void RunSingleEvent() = 0;
	void RunEvent();
};

class IWorkEvent : public IThreadEvent {
public:
	virtual void RunEventLoop() = 0;
	void RunEvent();
protected:
	void StopEvent();
private:
	bool eventWorking;
};

struct AudioEventData {
	std::string filePath;
};

class AudioFileEvent : public ISingleEvent {
public:
	virtual void RunSingleEvent();

	void SetupEvent(std::string filepath);
	void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();

	AudioFileEvent();
	~AudioFileEvent();

	bool doLoop;
private:
	bool eventActive;
	AudioEventData* mData;
};

class YoutubeEvent : public ISingleEvent {
public:
	virtual void RunSingleEvent();
	YoutubeEvent();

	void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();

	void DoPlayFile();
private:
	std::string ytUrl;
	std::string videoTitle;
	std::string fullVideoUrl;
	bool hasTitle;
	bool eventActive;
	bool doLoop;
};

class SeekToEvent : public ISingleEvent {
public:
	virtual void RunSingleEvent();
	void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();
private:
	double seekTo;
};

class NextTrackEvent : public ISingleEvent {
public:
	NextTrackEvent();
	virtual void RunSingleEvent();
	virtual void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();
	void OverrideExit();
private:
	bool overrideExit;
};

class StopTracksEvent : public ISingleEvent {
public:
	virtual void RunSingleEvent();
	std::string GetEventMessage();
private:
	
};

class TextToSpeechEvent : public ISingleEvent {
public:
	TextToSpeechEvent();
	virtual void RunSingleEvent();
	virtual void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();
private:
	short* samples;
	std::string textToSay;
};

class EventManager{
public:
	virtual void AddEvent(IThreadEvent* tEvent) = 0;
	virtual void RunEvents() = 0;

	static EventManager* getEventManager();
protected:
	std::list<IThreadEvent*> mEvents;
private:

	static EventManager* mEventManager;
};


#endif // __EVENT_HEADER__