#include "factory.h"

namespace EventsF {

	std::map<std::string, EventCreator*> *mainMap;

	void DefineEvent(EventCreator* eventType){
		if (!mainMap){
			mainMap = new std::map<std::string, EventCreator*>();
		}
		(*mainMap)[eventType->GetType()] = eventType;
	}

	IThreadEvent* CreateNewEvent(std::string eventType){
		std::map<std::string, EventCreator*>::iterator it;
		it = mainMap->find(eventType);
		if (it == mainMap->end()){
			return NULL;
		}
		return it->second->Create();
	}

}

void ISingleEvent::RunEvent() {
	RunSingleEvent();
}

void ISingleEvent::StopEvent() {

}

void IWorkEvent::RunEvent() {
	eventWorking = true;
	RunStartEvent();
	for (;;) {
		std::lock_guard<std::mutex> loopLock(eventLock);
		if (!eventWorking) {
			break;
		}
		RunEventLoop();
	}
}

void IWorkEvent::StopEvent() {
	std::lock_guard<std::mutex> stopLock(eventLock);
	eventWorking = false;
}

