#ifndef __EVENT_HEADER__
#define __EVENT_HEADER__

#include <string>
#include <list>
#include <deque>
#include <mutex>

class IThreadEvent {
public:
	virtual void SetupArgs(std::deque<std::string>& args) ;
	virtual std::string GetEventMessage();
	virtual ~IThreadEvent(){}
	virtual void RunEvent() = 0;
	virtual void StopEvent() = 0;
};

class ISingleEvent : public IThreadEvent {
public:
	virtual void RunSingleEvent() = 0;
	void RunEvent();
	void StopEvent();
};

class IWorkEvent : public IThreadEvent {
public:
	virtual void RunEventLoop() = 0;
	virtual void RunStartEvent() = 0;
	virtual ~IWorkEvent() {}
	virtual void RunEvent();
	virtual void StopEvent();
protected:
	bool eventWorking;
private:
	std::mutex eventLock;
};

class AudioFileEvent : public ISingleEvent {
public:
	virtual void RunSingleEvent();

	void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();

	AudioFileEvent();
	~AudioFileEvent();
private:
	bool eventActive;
	bool doLoop;
	std::string filePath;
};

class AudioFileEncoded;

class YoutubeEvent : public IWorkEvent {
public:
	virtual void RunStartEvent();
	virtual void RunEventLoop();
	YoutubeEvent();
	virtual ~YoutubeEvent();

	virtual void StopEvent();

	void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();
private:
	std::string ytUrl;
	std::string videoTitle;
	std::string fullVideoUrl;
	bool hasTitle;
	bool eventActive;
	bool doLoop;

	AudioFileEncoded* ytFile;
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

class EventManager {
public:
	virtual void AddEvent(IThreadEvent* tEvent) = 0;
	virtual void BuildEvents() = 0;
	virtual IThreadEvent* GetNextEvent() = 0;
	virtual ~EventManager() {}

	static EventManager* getEventManager();
protected:
	std::list<IThreadEvent*> mEvents;
private:

	static EventManager* mEventManager;
};


#endif // __EVENT_HEADER__