#include "events.h"
#include "../util.h"
#include <mutex>
#include <vector>

class EventWorker {
public:
	EventWorker();
	EventWorker(const EventWorker&);
	~EventWorker();
	void Work();
private:
	std::thread workerThread;
	bool threadWorking;
};

EventWorker::EventWorker() : workerThread(&EventWorker::Work, this) {
	printf("Default\n");
	threadWorking = true;
}

EventWorker::~EventWorker() {
	printf("Joining thread\n");
	workerThread.join();
}

EventWorker::EventWorker(const EventWorker& workerClone) : workerThread(&EventWorker::Work, this) {
	printf("Copy\n");
	threadWorking = true;
}

class ThreadedEventManager : public EventManager {
public:

	ThreadedEventManager();
	virtual void AddEvent(IThreadEvent* tEvent);
	virtual void BuildEvents();
	virtual IThreadEvent* GetNextEvent();
	virtual ~ThreadedEventManager();
protected:
	std::mutex lockMtx;
	std::vector<EventWorker> workerThreads;
};

ThreadedEventManager::ThreadedEventManager() {

}

ThreadedEventManager::~ThreadedEventManager() {

}

EventManager* EventManager::mEventManager = NULL;

EventManager* EventManager::getEventManager(){
	if (!mEventManager){
		mEventManager = new ThreadedEventManager();
	}
	return mEventManager;
}

IThreadEvent* ThreadedEventManager::GetNextEvent() {
	IThreadEvent* firstEvent = nullptr;
	lockMtx.lock();
	if (mEvents.size() > 0) {
		firstEvent = mEvents.front();
		mEvents.pop_front();
	}
	lockMtx.unlock();
	return firstEvent;
}

void EventWorker::Work() {
	IThreadEvent* nextEvent = nullptr;
	while (threadWorking) {
		nextEvent = EventManager::getEventManager()->GetNextEvent();
		if (nextEvent) {
			nextEvent->RunEvent();
			delete nextEvent;
		}
		ThreadSleep(20);
	}
}

void ThreadedEventManager::AddEvent(IThreadEvent* tEvent){
	printf("Adding event.\n");
	lockMtx.lock();
	mEvents.push_back(tEvent);
	lockMtx.unlock();
}

/*void ThreadedEventManager::RunEvents(){
	std::list<IThreadEvent*> cloneEvents;
	lockMtx.lock();
	cloneEvents = mEvents;
	mEvents.clear();
	lockMtx.unlock();
	std::list<IThreadEvent*>::iterator it;
	for (it = cloneEvents.begin(); it != cloneEvents.end(); it++){
		(*it)->RunEvent();
		SendTextMessage((*it)->GetEventMessage());
		delete (*it);
	}
}*/

void ThreadedEventManager::BuildEvents() {
	workerThreads.resize(3); // Start 3 workers. Yay for magic numbers.
}

void IThreadEvent::SetupArgs(std::deque<std::string>& args){

}

std::string IThreadEvent::GetEventMessage(){
	return "";
}