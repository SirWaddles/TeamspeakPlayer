#ifndef __UTIL_HEADER__
#define __UTIL_HEADER__

#include <string>

std::string get_file_contents(const char *filename);
bool doesFileExist(std::string path);
void ThreadSleep(int ms);
void AddToLog(std::string logmsg);

void SendTextMessage(std::string msg);

double GetTimeSinceStart();
void StartTimeCounter();


#endif // __UTIL_HEADER__