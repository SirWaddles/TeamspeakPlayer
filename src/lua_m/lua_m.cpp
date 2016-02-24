#include "lua_m.h"
#include "../http/http.h"

int printConsole(lua_State* L){
	int argc = lua_gettop(L);
	const char* stuff = lua_tostring(L, 1);
	printf("Msg: %s\n", stuff);
	return 0;
}

int readLuaLine(lua_State* L){
	int argc = lua_gettop(L);

	LuaManager* LLM = LuaManager::GetLuaManager();
	int lastChar = LLM->lineReaderChar;
	LLM->lineReaderChar = LLM->lineReader.find_first_of("\n", LLM->lineReaderChar + 1);
	if (LLM->lineReaderChar == std::string::npos){
		lua_pushboolean(L, false);
		return 1;
	}

	std::string lineData = LLM->lineReader.substr(lastChar + 1, LLM->lineReaderChar - lastChar - 1);
	lua_pushstring(L, lineData.c_str());
	return 1;
}

LineRead LuaManager::ReadStreamLine(int id){
	LuaStream* cStream = mStreams[id];
	
	int lastChar = cStream->lineRead;
	cStream->lineRead = cStream->reader.find_first_of("\n", cStream->lineRead + 1);
	if (cStream->lineRead == std::string::npos){
		LineRead eof;
		eof.status = false;
		return eof;
	}

	LineRead line;
	line.status = true;
	line.line = cStream->reader.substr(lastChar + 1, cStream->lineRead - lastChar - 1);

	return line;
}

int createLuaStream(lua_State* L){
	int argc = lua_gettop(L);

	LuaManager* LLM = LuaManager::GetLuaManager();
	std::string url = lua_tostring(L, 1);
	int StreamID = LLM->CreateLuaStream(url);
	
	lua_pushinteger(L, StreamID);
	return 1;
}

int readLineFromStream(lua_State* L){
	int argc = lua_gettop(L);

	LuaManager* LLM = LuaManager::GetLuaManager();
	int streamid = lua_tointeger(L, 1);
	
	LineRead lRead = LLM->ReadStreamLine(streamid);
	if (!lRead.status){
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushstring(L, lRead.line.c_str());
	return 1;
}

LuaManager::LuaManager(){
	_L = lua_open();
	luaopen_base(_L);
	luaopen_io(_L);
	luaopen_string(_L);
	luaopen_math(_L);
	luaopen_table(_L);

	lua_register(_L, "printConsole", printConsole);
	lua_register(_L, "readMainLine", readLuaLine);
	lua_register(_L, "readLineFromStream", readLineFromStream);
	lua_register(_L, "createLuaStream", createLuaStream);

	lineReaderChar = 0;
}

LuaManager::~LuaManager(){
	std::vector<LuaStream*>::iterator it;
	for (it = mStreams.begin(); it < mStreams.end(); it++){
		delete *it;
	}

	lua_close(_L);
}

void LuaManager::reportErrors(int status){
	if (status != 0){
		printf("ERR: %s\n", lua_tostring(_L, -1));
		lua_pop(_L, 1);
	}
}

void LuaManager::RunLuaFunc(std::string fname){
	lua_getglobal(_L, fname.c_str());
	int s = lua_pcall(_L, 0, 1, 0);
	reportErrors(s);
	if (s != 0){
		return;
	}
}

void LuaManager::RunLuaFunc(std::string fname, std::string arg1){
	lua_getglobal(_L, fname.c_str());
	lua_pushstring(_L, arg1.c_str());
	int s = lua_pcall(_L, 1, 0, 0);
	reportErrors(s);
	if (s != 0) return;
}

std::string LuaManager::GetLuaFuncStr(){
	std::string lStr(lua_tostring(_L, -1));
	lua_pop(_L, 1);
	return lStr;
}

void LuaManager::RunFile(std::string filepath){
	luaL_loadfile(_L, filepath.c_str());
	int s = lua_pcall(_L, 0, LUA_MULTRET, 0);
	reportErrors(s);
}

int LuaManager::CreateLuaStream(std::string url){

	printf("Creating lua stream: %s\n", url.c_str());

	LuaStream* nStream = new LuaStream();
	int lId = mStreams.size();
	mStreams.push_back(nStream);

	/*boost::network::http::client client;
	boost::network::http::client::request request(url);
	request << boost::network::header("Connection", "close");
	boost::network::http::client::response response = client.get(request);
	nStream->reader = response.body();

	nStream->lineRead = 0;*/

	std::string respData;
	respData = GetHTTPContents(url);
	nStream->reader = respData;
	nStream->lineRead = 0;

	return lId;
}

LuaManager* LuaManager::mainLuaManager = NULL;

LuaManager* LuaManager::GetLuaManager(){
	if (!mainLuaManager){
		mainLuaManager = new LuaManager();
	}
	return mainLuaManager;
}