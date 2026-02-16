#pragma once

#include "../angelscript/sdk/angelscript/include/angelscript.h" //#include "angelscript.h"

#include <string>
#include <list>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <atomic>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>

class DebuggerServer
{
	friend std::string ToJsonString(const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod);
	friend void SetToStringCallback(asITypeInfo* t, std::function<std::string(const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)> func);
public:
	DebuggerServer();
	~DebuggerServer();

	void CheckConnection();

	bool bind(uint16_t port);
	bool listen(int backlog = 1);

	SOCKET accept(struct sockaddr_in& clientAddr);

	void closeSocket(SOCKET socket);
	void closeSocketVS();
	void closeSocketMy();

	int send(const std::string& str) const;
	int send(SOCKET socket, const void* buffer, size_t length) const;
	int receive(SOCKET socket, void* buffer, size_t length) const;

	void outputDebugText(const char* category, const std::string& str, bool printToLocalWindow = true);
	void setToStringCallback(asITypeInfo* t, std::function<std::string(const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)> func)
	{
		mToStringCallbacks[t] = func;
	}

protected:
	SOCKET mMySockfd;
	struct sockaddr_in mLocalAddr;
	std::mutex mMutex;
	std::unordered_map<asITypeInfo*, std::function<std::string(const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)>> mToStringCallbacks;

public:
	volatile int mConnectedCount = 0;
	volatile bool mReady = false;
	std::atomic<SOCKET> mSocketVS = -1;
};

class ThreadSignal
{
private:
	HANDLE eventHandle;
	std::atomic_bool threadWaiting;
	std::atomic_bool preWaiting;
	std::atomic_bool signal;
	std::mutex signalMutex;

public:
	ThreadSignal() : threadWaiting(false)
	{
		eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	~ThreadSignal()
	{
		CloseHandle(eventHandle);
	}

	void waitForSignal(std::atomic<SOCKET>& socket)
	{
		threadWaiting = true;

		if (signal)
		{
			resetSignal();

			if (socket == SOCKET(-1))
			{
				signal = false;
				threadWaiting = false;
				preWaiting = false;
				return;
			}
		}

		WaitForSingleObject(eventHandle, INFINITE);

		signal = false;
		threadWaiting = false;
		preWaiting = false;
	}

	void setSignal()
	{
		if (!signal)
		{
			signalMutex.lock();

			if (!signal)
			{
				signal = true;
				SetEvent(eventHandle);
			}

			signalMutex.unlock();
		}
	}

	void resetSignal()
	{
		if (signal)
		{
			signalMutex.lock();

			if (signal)
			{
				signal = false;
				ResetEvent(eventHandle);
			}

			signalMutex.unlock();
		}
	}

	void clearPreWaitingSignal()
	{
		if (isPreWaiting())
		{
			setPreWaiting(false);
			threadWaiting = false;
			resetSignal();
		}
	}

	bool getSignal() const
	{
		return signal;
	}

	bool isThreadWaiting() const
	{
		return threadWaiting;
	}

	void setPreWaiting(bool value)
	{
		preWaiting = value;
	}

	bool isPreWaiting() const
	{
		return preWaiting;
	}
};

enum class VisualStudioCommand
{
	VS_COMMAND_INVALID,
	VS_COMMAND_CONTINUE,
	VS_COMMAND_STEP_INTO,
	VS_COMMAND_STEP_OVER,
	VS_COMMAND_STEP_OUT,
	VS_COMMAND_PAUSE,
	VS_COMMAND_SET_BREAKPOINT,
	VS_COMMAND_REMOVE_BREAKPOINT,
	VS_COMMAND_GET_STACKTRACE,
	VS_COMMAND_GET_VARIABLES,
	VS_COMMAND_EVALUATE,
};

struct ThreadInfo
{
	ThreadSignal signal;
	std::atomic<VisualStudioCommand> command = VisualStudioCommand::VS_COMMAND_CONTINUE;
	std::atomic<asIScriptContext*> context = nullptr;
	std::string currCodeFile;
	uint64_t currCodeLine = 0;
	uint64_t currStackLevel = 0;
};

struct BreakpointInfo
{
	std::string mFile;
	int mLine;
	std::string mFunction;
	int mFunctionLineOffset;
	std::string mConditionType;
	std::string mConditionExp; //condition expression
	bool mEnabled;

	bool conditionValid = false;
	int  conditionOpCode = -1;
	std::string conditionOpValue{};
	std::vector<std::string> variableName{};
};

extern DebuggerServer gDbgSvr;
void CreateScriptEngineCallback(asIScriptEngine* pScriptEngine);
void RunDebuggerServer(volatile bool& bTerminated, uint16_t uServerPort);
void PrepareExecutionCallback(asIScriptContext* ctx);

std::string ToJsonString(const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod);
void SetToStringCallback(asITypeInfo* t, std::function<std::string(const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)> func);

#pragma region Custom extension instructions
// Attention!!! The use of the continue instruction within a SWITCH statement will result in behavior exceptions, 
// which must be done using CONTINUE and followed by SWITCH-INTINUE outside the statement;
#define SWITCH(_p_) bool _switch_have_continue_instruct=false;for(std::string _cmd_(_p_),_pass_,_cmd_str_list_,_is_print_case_="0",_once_;\
					_pass_.empty()&&_is_print_case_[0]<='2';\
					_is_print_case_[0]+=1)

// CASE needs to follow {} to work properly, even if it's empty {}
#define CASE(_p_) if(_is_print_case_[0]=='1')\
					_cmd_str_list_+=std::string(_p_)+"\n";\
				else\
					if(_pass_.empty())if(_cmd_ == _p_)_pass_="1";if(!_pass_.empty())\

#define DEFAULT() if(_is_print_case_[0]!='1')if(_once_.empty())if(_once_="1";true)
#define BREAK break
#define CONTINUE {_switch_have_continue_instruct=true;break;}_switch_have_continue_instruct
#define SWITCH_CONTINUE if(_switch_have_continue_instruct) continue

#define PRINT_CASE_LIST(out)  if(_is_print_case_[0]=='0') \
						{\
							_once_="";\
							_pass_="";\
							continue;\
						}else out=std::move(_cmd_str_list_)
#pragma endregion

#define	STR(s) #s
#define	STR_JSON(...) STR(__VA_ARGS__);
#define	JSON(...) STR(__VA_ARGS__);
#define	JSON_FMT(...) STR({)STR(__VA_ARGS__)STR(});
#define	JSON_LINE_FMT(...) STR({)STR(__VA_ARGS__)STR(}\n);

#include <iostream>
#define SYS_LOG(s) std::cout << s << std::endl;
#define DEBUG_LOG(s) std::cout << s << std::endl;

std::pair<std::string, size_t> GetFileLineByModuleLine(asIScriptModule* module, size_t inLine);
