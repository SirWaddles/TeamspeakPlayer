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

void IWorkEvent::RunEvent() {
	eventWorking = true;
	RunStartEvent();
	for (;;) {
		std::lock_guard<std::mutex> loopLock(eventLock);
		if (!eventWorking) {
			printf("Breaking loop\n");
			break;
		}
		RunEventLoop();
	}
}

void IWorkEvent::StopEvent() {
	printf("Acquiring Lock\n");
	std::lock_guard<std::mutex> stopLock(eventLock);
	printf("Locked\n");
	eventWorking = false;
}

