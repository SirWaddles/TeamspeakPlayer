#include "events.h"
#include "../util.h"
#include <mutex>

class ThreadedEventManager : public EventManager {
public:
	virtual void AddEvent(IThreadEvent* tEvent);
	virtual void RunEvents();
protected:
	std::mutex lockMtx;
};

EventManager* EventManager::mEventManager = NULL;

EventManager* EventManager::getEventManager(){
	if (!mEventManager){
		mEventManager = new ThreadedEventManager();
	}
	return mEventManager;
}

void ThreadedEventManager::AddEvent(IThreadEvent* tEvent){
	printf("Adding event.\n");
	lockMtx.lock();
	mEvents.push_back(tEvent);
	lockMtx.unlock();
}

void ThreadedEventManager::RunEvents(){
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
}

void IThreadEvent::SetupArgs(std::deque<std::string>& args){

}

std::string IThreadEvent::GetEventMessage(){
	return "";
}