#include <Windows.h>
#include "util.h"
#include <iostream>
#include <fstream>

std::string get_file_contents(const char *filename)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

bool doesFileExist(std::string path){
	FILE* test = fopen(path.c_str(), "r");
	if (test) {
		fclose(test);
		return true;
	}
	return false;
}

void ThreadSleep(int ms){
	Sleep(ms);
}

void AddToLog(std::string logmsg){
	std::string logpath(getenv("APPDATA"));
	logpath += "/tsbot/log.txt";

	FILE* log = fopen(logpath.c_str(), "a");
	fputs(logmsg.c_str(), log);
	fclose(log);
}

double pcFreq = 0.0;
unsigned long counterStart = 0;

double GetTimeSinceStart(){
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - counterStart) / pcFreq;
}

void StartTimeCounter(){
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);

	pcFreq = double(li.QuadPart) / 1000.0;
	QueryPerformanceCounter(&li);
	counterStart = li.QuadPart;
}