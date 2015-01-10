#ifndef __EVENT_HEADER__
#define __EVENT_HEADER__

#include <string>
#include <list>
#include <deque>

class IThreadEvent {
public:
	virtual void RunEvent() = 0;
	virtual void SetupArgs(std::deque<std::string>& args) ;
	virtual std::string GetEventMessage();
	virtual ~IThreadEvent(){}
};

struct AudioEventData {
	std::string filePath;
};

class AudioFileEvent : public IThreadEvent {
public:
	virtual void RunEvent();

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

class YoutubeEvent : public IThreadEvent {
public:
	virtual void RunEvent();
	YoutubeEvent();

	void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();

	void DoPlayFile();
private:
	std::string ytUrl;
	std::string videoTitle;
	bool hasTitle;
	bool eventActive;
	bool doLoop;
};

class SeekToEvent : public IThreadEvent {
public:
	virtual void RunEvent();
	void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();
private:
	double seekTo;
};

class NextTrackEvent : public IThreadEvent {
public:
	NextTrackEvent();
	virtual void RunEvent();
	virtual void SetupArgs(std::deque<std::string>& args);
	std::string GetEventMessage();
	void OverrideExit();
private:
	bool overrideExit;
};

class StopTracksEvent : public IThreadEvent {
public:
	virtual void RunEvent();
	std::string GetEventMessage();
private:
	
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