#ifndef __FACTORY_HEADER__
#define __FACTORY_HEADER__

#include <map>
#include <string>
#include "events.h"

namespace EventsF {

	class EventCreator {
	public:
		virtual IThreadEvent* Create() = 0;
		virtual std::string GetType() = 0;
	};

	template<class T>
	class EventCreatorType : public EventCreator {
	public:
		EventCreatorType(std::string sType) {
			eventName = sType;
			DefineEvent(this);
		}

		IThreadEvent* Create(){
			T* newEvent = new T();
			return newEvent;
		}

		std::string GetType(){
			return eventName;
		}

	private:
		std::string eventName;
	};

	void DefineEvent(EventCreator* eType);
	IThreadEvent* CreateNewEvent(std::string sType);

}





#endif // __FACTORY_HEADER__