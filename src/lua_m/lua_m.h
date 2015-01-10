#ifndef __LUA_MANAGER__
#define __LUA_MANAGER__

#include <lua.hpp>
#include <string>
#include <vector>

struct LuaStream {
	std::string reader;
	int lineRead = 0;
};

struct LineRead {
	std::string line;
	bool status;
};

class LuaManager {
public:
	LuaManager();
	~LuaManager();
	lua_State* getLuaState();

	void RunFile(std::string filepath);
	void RunLuaFunc(std::string fName);
	void RunLuaFunc(std::string fName, std::string arg1);
	std::string GetLuaFuncStr();

	int CreateLuaStream(std::string url);
	LineRead ReadStreamLine(int id);

	static LuaManager* GetLuaManager();

	std::string lineReader;
	int lineReaderChar;

private:

	void reportErrors(int status);

	lua_State* _L;
	static LuaManager* mainLuaManager;

	std::vector<LuaStream*> mStreams;
};





#endif // __LUA_MANAGER__