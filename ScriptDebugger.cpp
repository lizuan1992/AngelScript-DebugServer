#include "ScriptDebugger.h"

std::string StringSymbalEscape(std::string str)
{
	// Escape backslash first to avoid double-escaping
	for (auto pos = str.find("\\"); pos != std::string::npos; pos = str.find("\\", pos + 2))
	{
		str.replace(pos, 1, "\\\\");
	}

	// Escape double quotes
	for (auto pos = str.find("\""); pos != std::string::npos; pos = str.find("\"", pos + 2))
	{
		str.replace(pos, 1, "\\\"");
	}

	// Escape newline
	for (auto pos = str.find("\n"); pos != std::string::npos; pos = str.find("\n", pos + 2))
	{
		str.replace(pos, 1, "\\n");
	}

	// Escape carriage return
	for (auto pos = str.find("\r"); pos != std::string::npos; pos = str.find("\r", pos + 2))
	{
		str.replace(pos, 1, "\\r");
	}

	// Escape tab
	for (auto pos = str.find("\t"); pos != std::string::npos; pos = str.find("\t", pos + 2))
	{
		str.replace(pos, 1, "\\t");
	}

	// Escape backspace
	for (auto pos = str.find("\b"); pos != std::string::npos; pos = str.find("\b", pos + 2))
	{
		str.replace(pos, 1, "\\b");
	}

	// Escape form feed
	for (auto pos = str.find("\f"); pos != std::string::npos; pos = str.find("\f", pos + 2))
	{
		str.replace(pos, 1, "\\f");
	}

	// Optionally escape forward slash
	for (auto pos = str.find("/"); pos != std::string::npos; pos = str.find("/", pos + 2))
	{
		str.replace(pos, 1, "\\/");
	}

	return str;
}

std::string StringSymbalUnescape(std::string str)
{
	// Unescape in reverse order to avoid conflicts
	for (auto pos = str.find("\\\\"); pos != std::string::npos; pos = str.find("\\\\", pos))
	{
		str.replace(pos, 2, "\\");
	}

	for (auto pos = str.find("\\\""); pos != std::string::npos; pos = str.find("\\\"", pos))
	{
		str.replace(pos, 2, "\"");
	}

	for (auto pos = str.find("\\n"); pos != std::string::npos; pos = str.find("\\n", pos))
	{
		str.replace(pos, 2, "\n");
	}

	for (auto pos = str.find("\\r"); pos != std::string::npos; pos = str.find("\\r", pos))
	{
		str.replace(pos, 2, "\r");
	}

	for (auto pos = str.find("\\t"); pos != std::string::npos; pos = str.find("\\t", pos))
	{
		str.replace(pos, 2, "\t");
	}

	for (auto pos = str.find("\\b"); pos != std::string::npos; pos = str.find("\\b", pos))
	{
		str.replace(pos, 2, "\b");
	}

	for (auto pos = str.find("\\f"); pos != std::string::npos; pos = str.find("\\f", pos))
	{
		str.replace(pos, 2, "\f");
	}

	for (auto pos = str.find("\\/"); pos != std::string::npos; pos = str.find("\\/", pos))
	{
		str.replace(pos, 2, "/");
	}

	return str;
}

DebuggerServer::DebuggerServer()
{
	mMySockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&mLocalAddr, 0, sizeof(mLocalAddr));
}

DebuggerServer::~DebuggerServer()
{
	if (mMySockfd != -1)
		::shutdown(mMySockfd, SD_BOTH);
}

void DebuggerServer::checkConnection()
{
	if (mSocketVS == SOCKET(-1))
		return;

	static auto last = std::chrono::high_resolution_clock::now();
	static auto lastTime = std::chrono::duration_cast<std::chrono::milliseconds>(last.time_since_epoch()).count();

	auto now = std::chrono::high_resolution_clock::now();
	auto nowTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

	if (nowTime - lastTime > 1000)
	{
		lastTime = nowTime;
		auto testStr = "test connection\n";
		auto r = send(mSocketVS, testStr, strlen(testStr));
		if (r == -1 || r == 0)
			closeSocketVS();
	}
}

bool DebuggerServer::bind(uint16_t port)
{
	memset(&mLocalAddr, 0, sizeof(mLocalAddr));
	mLocalAddr.sin_family = AF_INET;
	mLocalAddr.sin_port = htons(port);
	mLocalAddr.sin_addr.s_addr = INADDR_ANY;

	if (::bind(mMySockfd, (struct sockaddr*)&mLocalAddr, sizeof(mLocalAddr)) < 0)
		return false;

	return true;
}

bool DebuggerServer::listen(int backlog)
{
	if (mMySockfd != -1 && ::listen(mMySockfd, backlog) < 0)
		return false;

	return true;
}

SOCKET DebuggerServer::accept(struct sockaddr_in& clientAddr)
{
	socklen_t addrLen = sizeof(clientAddr);

	return ::accept(mMySockfd, (struct sockaddr*)&clientAddr, &addrLen);
}

void DebuggerServer::closeSocket(SOCKET socket)
{
	if (socket != -1)
	{
		shutdown(socket, SD_BOTH);
		::closesocket(socket);
	}
}

void DebuggerServer::closeSocketVS()
{
	mMutex.lock();

	if (mSocketVS != -1)
	{
		shutdown(mSocketVS, SD_BOTH);
		::closesocket(mSocketVS);
		mSocketVS = -1;
	}

	mMutex.unlock();
}

void DebuggerServer::closeSocketMy()
{
	mMutex.lock();

	if (mMySockfd != -1)
	{
		shutdown(mMySockfd, SD_BOTH);
		::closesocket(mMySockfd);
		mMySockfd = -1;
	}

	mMutex.unlock();
}

int DebuggerServer::send(const std::string& str) const
{
	return ::send(mSocketVS, str.c_str(), int(str.size()), 0);
}

int DebuggerServer::send(SOCKET socket, const void* buffer, size_t length) const
{
	return ::send(socket, (char*)buffer, int(length), 0);
}

int DebuggerServer::receive(SOCKET socket, void* buffer, size_t length) const
{
	return ::recv(socket, (char*)buffer, int(length), 0);
}

void DebuggerServer::outputDebugText(const char* category, const std::string& str, bool printToLocalWindow)
{
	if (mSocketVS != SOCKET(-1))
	{
		constexpr auto response = JSON_LINE_FMT(
			{
				"type" : "event",
				"event" : "output",
				"category" : "{}",
				"output" : "{}\n"
			});

		gDbgSvr.send(std::format(response, StringSymbalEscape(category), StringSymbalEscape(str)));
	}

	if (printToLocalWindow)
		DEBUG_LOG(str.c_str());
}

DebuggerServer gDbgSvr;
static std::vector<std::pair<int, asIScriptEngine*>> gThreadEngineList;
static std::unordered_map<int, ThreadInfo> gThreadInfoList;
static std::unordered_map<std::string, std::unordered_map<size_t, BreakpointInfo>> gBreakpointList;
constexpr auto BASE_DIRECTORY = "Script\\";

namespace
{
	auto split = [](const std::string& str, char delimiter)->std::vector<std::string>
		{
			std::vector<std::string> results;

			std::stringstream ss(str);
			std::string item;

			while (std::getline(ss, item, delimiter))
			{
				results.push_back(item);
			}

			return results;
		};

	bool MatchVariableName(const std::vector<std::string>& names, void*& ptr, int& typeId)
	{
		bool _bool = true;
		for (size_t i = 1; i < names.size(); i++)
		{
			if (typeId & asTYPEID_SCRIPTOBJECT)
			{
				auto obj = (asIScriptObject*)ptr;
				if (typeId & asTYPEID_OBJHANDLE)
					obj = *(asIScriptObject**)ptr;

				if (obj != nullptr)
				{
					_bool = false;
					for (asUINT n = 0; n < obj->GetPropertyCount(); n++)
					{
						auto name = obj->GetPropertyName(n);
						if (name == names[i])
						{
							ptr = obj->GetAddressOfProperty(n);
							typeId = obj->GetPropertyTypeId(n);
							_bool = true;
							break;
						}
					}

					if (_bool)
						continue;
				}
			}

			ptr = nullptr;
			break;
		}

		return _bool;
	}

	std::string GetJsonValue(const std::string& json, std::string key)
	{
		key = "\"" + key + "\"";
		auto keyPos = json.find(key);
		if (keyPos == std::string::npos)
			return "";

		auto colonPos = json.find(":", keyPos);
		if (colonPos == std::string::npos)
			return "";

		auto valueStartPos = json.find_first_not_of(" \t\n\r", colonPos + 1);
		if (valueStartPos == std::string::npos)
			return "";

		if (json[valueStartPos] == '\"')
		{
			auto valueEndPos = valueStartPos + 1;
			while (valueEndPos < json.size())
			{
				if (json[valueEndPos] == '\"' && json[valueEndPos - 1] != '\\')
					break;
				else
					valueEndPos++;
			}

			return StringSymbalUnescape(json.substr(valueStartPos + 1, valueEndPos - valueStartPos - 1));
		}
		else
		{
			auto valueEndPos = json.find_first_of(",}\n\r", valueStartPos + 1);
			if (valueEndPos == std::string::npos)
				return json.substr(valueStartPos);

			return json.substr(valueStartPos, valueEndPos - valueStartPos);
		}
	}

	auto GetScriptEngine(int threadId) -> asIScriptEngine*
	{
		for (auto& [_threadId, engine] : gThreadEngineList)
		{
			if (_threadId == threadId)
				return engine;
		}

		return nullptr;
	}

	auto GetEngineThreadId(asIScriptEngine* engine) -> DWORD
	{
		for (auto& [threadId, _engine] : gThreadEngineList)
		{
			if (_engine == engine)
				return threadId;
		}

		return 0;
	}

	void ClearBreakpoints()
	{
		if (gBreakpointList.size())
		{
			gBreakpointList.clear();
		}

		for (auto& [threadId, threadInfo] : gThreadInfoList)
		{
			if (threadInfo.signal.isThreadWaiting() || threadInfo.signal.isPreWaiting())
			{
				threadInfo.command = VisualStudioCommand::VS_COMMAND_CONTINUE;
				threadInfo.signal.setSignal();
			}
		}
	}

	void SetVariable(asIScriptContext* ctx, int64_t seq, int threadId, asUINT stackLevel, int64_t addr, asUINT typeId, std::string& value)
	{
		if (stackLevel >= ctx->GetCallstackSize())
		{
			constexpr auto response = JSON_LINE_FMT(
				{
					"type" : "response",
					"requestSeq" : {},
					"command" : "set_variable",
					"success" : false,
					"threadId" : {},
					"frameId" : {},
					"addr" : {},
					"typeId" : {},
					"value" : "{}",
					"message" : "Invalid frameId"
				});

			auto str = std::format(response, seq, threadId, stackLevel, addr, typeId, StringSymbalEscape(value));
			gDbgSvr.send(str);
			return;
		}

		auto SetVarValue = [](void* addr, asUINT typeId, const char* value, asIScriptEngine* engine)->bool
			{
				if (typeId == asTYPEID_BOOL)
					*(bool*)addr = std::string(value) == "true" ? true : false;
				else if (typeId == asTYPEID_INT8)
					*(signed char*)addr = (signed char)std::atoi(value);
				else if (typeId == asTYPEID_INT16)
					*(signed short*)addr = (signed short)std::atoi(value);
				else if (typeId == asTYPEID_INT32)
					*(signed int*)addr = (signed int)std::atoi(value);
				else if (typeId == asTYPEID_INT64)
					*(asINT64*)addr = (asINT64)std::atoll(value);
				else if (typeId == asTYPEID_UINT8)
					*(unsigned char*)addr = (unsigned char)std::atoi(value);
				else if (typeId == asTYPEID_UINT16)
					*(unsigned short*)addr = (unsigned short)std::atoi(value);
				else if (typeId == asTYPEID_UINT32)
					*(unsigned int*)addr = (unsigned int)std::atoi(value);
				else if (typeId == asTYPEID_UINT64)
					*(asQWORD*)addr = (asQWORD)std::atoll(value);
				else if (typeId == asTYPEID_FLOAT)
					*(float*)addr = (float)std::atof(value);
				else if (typeId == asTYPEID_DOUBLE)
					*(double*)addr = (double)std::atof(value);
				else if ((typeId & asTYPEID_MASK_OBJECT) == 0)
					*(asQWORD*)addr = (asQWORD)std::atoll(value);
				else
				{
					if (auto typeInfo = engine->GetTypeInfoById(typeId))
					{
						if (typeInfo->GetName() == std::string("string"))
						{
							if (typeId == typeInfo->GetTypeId())
							{
								*(std::string*)addr = value;
								return true;
							}
						}
					}

					return false;
				}

				return true;
			};

		constexpr auto response = JSON_LINE_FMT(
			{
				"type":"response",
				"requestSeq" : {},
				"command" : "set_variable",
				"success" : {},
				"message" : "{}",
				"threadId" : {},
				"frameId" : {},
				"addr" : {},
				"typeId" : {},
				"value" : "{}"
			});

		auto engine = ctx->GetEngine();
		auto ret = SetVarValue((void*)addr, typeId, value.c_str(), engine);

		if (ret == true)
		{
			auto str = std::format(response, seq, true, "", threadId, stackLevel, addr, typeId, StringSymbalEscape(value));
			gDbgSvr.send(str);
		}
		else
		{
			auto str = std::format(response, seq, false, "Invalid typeId " + std::to_string(typeId), threadId, stackLevel, addr, typeId, StringSymbalEscape(value));
			gDbgSvr.send(str);
		}
	}

	void GetEvaluation(asIScriptContext* ctx, int64_t seq, int threadId, asUINT stackLevel, std::string& expression, asIScriptEngine* engine)
	{
		auto returnError = [&](const char* error)
			{
				constexpr auto response = JSON_LINE_FMT(
					{
						"type" : "response",
						"requestSeq" : {},
						"command" : "get_evaluation",
						"success" : false,
						"threadId" : {},
						"frameId" : {},
						"expression" : "{}",
						"message" : "{}"
					});

				auto str = std::format(response, seq, threadId, stackLevel, StringSymbalEscape(expression), error);
				gDbgSvr.send(str);
			};

		// Tokenize the input string to get the variable scope and name
		asUINT len = 0;
		std::string scope;
		std::string name;
		std::string str = expression;
		asETokenClass t = engine->ParseToken(str.c_str(), 0, &len);
		while (t == asTC_IDENTIFIER || (t == asTC_KEYWORD && len == 2 && str.compare(0, 2, "::") == 0))
		{
			if (t == asTC_KEYWORD)
			{
				if (scope == "" && name == "")
					scope = "::";			// global scope
				else if (scope == "::" || scope == "")
					scope = name;			// namespace
				else
					scope += "::" + name;	// nested namespace
				name = "";
			}
			else if (t == asTC_IDENTIFIER)
				name.assign(str.c_str(), len);

			// Skip the parsed token and get the next one
			str = str.substr(len);
			t = engine->ParseToken(str.c_str(), 0, &len);
		}

		if (name.size())
		{
			// Find the variable
			void* ptr = 0;
			int typeId = 0;
			asETypeModifiers typeMod = asTM_NONE;

			asIScriptFunction* func = ctx->GetFunction(stackLevel);
			if (!func)
			{
				returnError("Invalid expression. No function in the current stack frame");
				return;
			}

			// skip local variables if a scope was informed
			if (scope == "")
			{
				// We start from the end, in case the same name is reused in different scopes
				for (asUINT n = func->GetVarCount(); n-- > 0; )
				{
					const char* varName = 0;
					ctx->GetVar(n, stackLevel, &varName, &typeId, &typeMod);
					if (ctx->IsVarInScope(n, stackLevel) && varName != 0 && varName == name)
					{
						ptr = ctx->GetAddressOfVar(n, stackLevel);
						break;
					}
				}

				// Look for class members, if we're in a class method
				if (!ptr && func->GetObjectType())
				{
					if (name == "this")
					{
						ptr = ctx->GetThisPointer(stackLevel);
						typeId = ctx->GetThisTypeId(stackLevel);
					}
					else
					{
						if (asITypeInfo* type = engine->GetTypeInfoById(ctx->GetThisTypeId(stackLevel)))
						{
							for (asUINT n = 0; n < type->GetPropertyCount(); n++)
							{
								const char* propName = 0;
								int offset = 0;
								bool isReference = 0;
								int compositeOffset = 0;
								bool isCompositeIndirect = false;
								type->GetProperty(n, &propName, &typeId, 0, 0, &offset, &isReference, 0, &compositeOffset, &isCompositeIndirect);
								if (name == propName)
								{
									ptr = (void*)(((asBYTE*)ctx->GetThisPointer(stackLevel)) + compositeOffset);
									if (isCompositeIndirect) ptr = *(void**)ptr;
									ptr = (void*)(((asBYTE*)ptr) + offset);
									if (isReference) ptr = *(void**)ptr;
									break;
								}
							}
						}
					}
				}
			}

			// Look for global variables
			if (!ptr)
			{
				if (scope == "")
				{
					// If no explicit scope was informed then use the namespace of the current function by default
					scope = func->GetNamespace();
				}
				else if (scope == "::")
				{
					// The global namespace will be empty
					scope = "";
				}

				if (asIScriptModule* mod = func->GetModule())
				{
					for (asUINT n = 0; n < mod->GetGlobalVarCount(); n++)
					{
						const char* varName = 0, * nameSpace = 0;
						mod->GetGlobalVar(n, &varName, &nameSpace, &typeId);

						// Check if both name and namespace match
						if (name == varName && scope == nameSpace)
						{
							ptr = mod->GetAddressOfGlobalVar(n);
							break;
						}
					}
				}
			}

			if (ptr)
			{
				// TODO: If there is a . after the identifier, check for members
				// TODO: If there is a [ after the identifier try to call the 'opIndex(expr) const' method 
				if (str != "")
				{
					auto names = split(expression, '.');
					MatchVariableName(names, ptr, typeId);
				}

				if (ptr)
				{
					// TODO: Allow user to set if members should be expanded
					// Expand members by default to 3 recursive levels only

					uint32_t count = 0;
					int size = 0;
					auto result = ToJsonString(StringSymbalEscape(expression).c_str(), ptr, typeId, 0, count, size, engine, typeMod);

					constexpr auto response = JSON_LINE_FMT(
						{
							"type" : "response",
							"requestSeq" : {},
							"command" : "get_evaluation",
							"success" : true,
							"threadId" : {},
							"frameId" : {},
							"expression" : "{}",
							"result" : {}
						});

					auto str = std::format(response, seq, threadId, stackLevel, StringSymbalEscape(expression), result);
					gDbgSvr.send(str);
				}
				else
				{
					returnError("Invalid expression. Unable to resolve members");
				}
			}
			else
			{
				returnError("Invalid expression. No matching symbol");
			}
		}
		else
		{
			returnError("Invalid expression. Expected identifier");
		}
	}

	void GetProperty(asIScriptContext* ctx, int64_t seq, int threadId, asUINT stackLevel, int64_t addr, asUINT typeId, asUINT start, asUINT count)
	{
		if (stackLevel >= ctx->GetCallstackSize())
		{
			constexpr auto response = JSON_LINE_FMT(
				{
					"type" : "response",
					"requestSeq" : {},
					"command" : "get_property",
					"success" : false,
					"threadId" : {},
					"frameId" : {},
					"addr" : {},
					"typeId" : {},
					"start" : {},
					"count" : {},
					"message" : "Invalid frameId"
				});

			auto str = std::format(response, seq, threadId, stackLevel, addr, typeId, start, count);
			gDbgSvr.send(str);
			return;
		}

		constexpr auto response = JSON_LINE_FMT(
			{
				"type":"response",
				"requestSeq" : {},
				"command" : "get_property",
				"success" : true,
				"threadId" : {},
				"frameId" : {},
				"addr" : {},
				"typeId" : {},
				"start" : {},
				"count" : {},
				"size" : {},
				"properties" : {}
			});

		auto engine = ctx->GetEngine();
		std::string properties = "[";

		int size = 0;
		properties += ToJsonString(nullptr, (void*)addr, typeId, start, count, size, engine, asTM_NONE);

		if (properties.back() == ',')
			properties.back() = ']';
		else
			properties.push_back(']');

		auto str = std::format(response, seq, threadId, stackLevel, addr, typeId, start, count, size, properties);
		gDbgSvr.send(str);
	}

	void GetFrameScope(asIScriptContext* ctx, int64_t seq, int threadId, asUINT stackLevel)
	{
		if (stackLevel >= ctx->GetCallstackSize())
		{
			constexpr auto response = JSON_LINE_FMT(
				{
					"type" : "response",
					"requestSeq" : {},
					"command" : "get_scope",
					"success" : false,
					"threadId" : {},
					"frameId" : {},
					"message" : "Invalid frameId"
				});

			auto str = std::format(response, seq, threadId, stackLevel);
			gDbgSvr.send(str);
			return;
		}

		constexpr auto response = JSON_LINE_FMT(
			{
				"type" : "response",
				"requestSeq" : {},
				"command" : "get_scope",
				"success" : true,
				"threadId" : {},
				"frameId" : {},
				"variables" : {}
			});

		auto engine = ctx->GetEngine();
		std::string variables = "[";
		int numVars = ctx->GetVarCount(stackLevel);
		for (int n = 0; n < numVars; n++)
		{
			if (ctx->IsVarInScope(n, stackLevel))
			{
				const char* varName = nullptr;
				int typeId = 0;
				asETypeModifiers typeMod = asTM_NONE;

				int ret = ctx->GetVar(n, stackLevel, &varName, &typeId, &typeMod);

				if (varName == 0 || strlen(varName) == 0)
					continue;

				auto addr = ctx->GetAddressOfVar(n, stackLevel);
				unsigned int count = 0;
				int size = 0;

				variables += ToJsonString(varName, addr, typeId, 0, count, size, engine, typeMod);
				variables += ',';
			}
		}

		if (variables.back() == ',')
			variables.back() = ']';
		else
			variables.push_back(']');

		auto str = std::format(response, seq, threadId, stackLevel, variables);
		gDbgSvr.send(str);
	}

	void GetStackFrames(asIScriptContext* ctx, int64_t seq, int threadId)
	{
		constexpr auto response = JSON_LINE_FMT(
			{
				"type" : "response",
				"requestSeq" : {},
				"command" : "get_stack",
				"success" : true,
				"threadId" : {},
				"frames" : {}
			});

		std::string frames = "[";
		constexpr auto frame = JSON_FMT(
			{
				"id" : {},
				"name" : "{}",
				"file" : "{}",
				"line" : {}
			});

		auto stackSize = ctx->GetCallstackSize();
		for (asUINT n = 0; n < stackSize; n++)
		{
			auto func = ctx->GetFunction(n);
			auto moduleLine = ctx->GetLineNumber(n, 0, 0);
			auto module = func->GetModule();
			if (module == nullptr || moduleLine <= 0)
				continue;

			auto [file, line] = GetFileLineByModuleLine(func->GetModule(), moduleLine);

			auto funcName = func->GetDeclaration(true, true, true);
			auto frameStr = std::format(frame, n, StringSymbalEscape(funcName), file, line);
			frames += frameStr;

			frames += ',';
		}

		if (frames.back() == ',')
			frames.back() = ']';
		else
			frames.push_back(']');

		auto str = std::format(response, seq, threadId, frames);
		gDbgSvr.send(str);
	}

	void GetThreads(int64_t seq)
	{
		constexpr auto response = JSON_LINE_FMT(
			{
				"type" : "response",
				"requestSeq" : {},
				"command" : "get_threads",
				"success" : true,
				"threads" : {}
			});

		std::string threads = "[";
		constexpr auto thread = JSON_FMT(
			{
				"id" : {},
				"name" : "{}"
			});

		for (size_t i = 0; i < gThreadEngineList.size(); i++)
		{
			auto& [threadId, engine] = gThreadEngineList[i];

			if (i == 0)
			{
				auto threadStr = std::format(thread, threadId, "DatabaseQuery");
				threads += threadStr;
			}
			else
			{
				auto threadStr = std::format(thread, threadId, "TaskDispatch " + std::to_string(i));
				threads += threadStr;
			}
			threads += ',';
		}

		if (threads.back() == ',')
			threads.back() = ']';
		else
			threads.push_back(']');

		auto str = std::format(response, seq, threads);
		gDbgSvr.send(str);
	}

	void CheckConditionBreakpoint(BreakpointInfo& bp)
	{
		if (bp.mConditionType == "expression")
		{
			std::stringstream ss(bp.mConditionExp);

			std::string variableName;
			ss >> variableName;

			std::string opCode;
			ss >> opCode;

			ss >> bp.conditionOpValue;

			if (!variableName.empty() && !opCode.empty() && !bp.conditionOpValue.empty())
			{
				bp.variableName = split(variableName, '.');

				SWITCH(opCode)
				{
					CASE("=") {}
					CASE("==")
					{
						bp.conditionOpCode = 1;
						bp.conditionValid = true;
						BREAK;
					}
					CASE("!=")
					{
						bp.conditionOpCode = 2;
						bp.conditionValid = true;
						BREAK;
					}
					CASE(">=")
					{
						bp.conditionOpCode = 3;
						bp.conditionValid = true;
						BREAK;
					}
					CASE(">")
					{
						bp.conditionOpCode = 4;
						bp.conditionValid = true;
						BREAK;
					}
					CASE("<=")
					{
						bp.conditionOpCode = 5;
						bp.conditionValid = true;
						BREAK;
					}
					CASE("<")
					{
						bp.conditionOpCode = 6;
						bp.conditionValid = true;
						BREAK;
					}
					DEFAULT()
					{
						gDbgSvr.outputDebugText("break_point", "The conditional expression second sub string expects compareSymbal such as \" (= or ==) != >= > <= < \"");
						BREAK;
					}
				}
			}
			else
				gDbgSvr.outputDebugText("break_point", "The conditional expression expects a foramt \"variableName compareSymbal value\" such as \"xxx >= 5\"");
		}
	}

	void HandleCommandFromVSX(std::string& json)
	{
		struct error_check
		{
			error_check(int64_t seq, std::string& command, int threadId, const char* error_msg)
			{
				constexpr auto response = JSON_LINE_FMT(
					{
						"type" : "response",
						"requestSeq" : {},
						"command" : "{}",
						"success" : false,
						"threadId" : {},
						"message" : "{}"
					});

				auto iter = gThreadInfoList.find(threadId);

				if (iter == gThreadInfoList.end())
				{
					auto str = std::format(response, seq, command, threadId, "The thread ID is invalid");
					gDbgSvr.send(str);
					gDbgSvr.outputDebugText("error", str);
					return;
				}

				auto& [threadSignal, threadCommand, threadContext, t1, t2, t3] = iter->second;

				for (int i = 0;;)
				{
					if (threadCommand != VisualStudioCommand::VS_COMMAND_PAUSE && !threadSignal.isThreadWaiting())
					{
						if (++i > 10)
						{
							auto str = std::format(response, seq, command, threadId, error_msg);
							gDbgSvr.send(str);
							gDbgSvr.outputDebugText("error", str);
							return;
						}
						else
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
							continue;
						}
					}

					break;
				}

				this->threadSignal = &threadSignal;
				this->threadCommand = &threadCommand;
				this->threadContext = threadContext;
			}
			error_check(int64_t seq, std::string& command, int threadId, int frameId, const char* error_msg)
			{
				constexpr auto response = JSON_LINE_FMT(
					{
						"type" : "response",
						"requestSeq" : {},
						"command" : "{}",
						"success" : false,
						"threadId" : {},
						"frameId" : {},
						"message" : "{}"
					});

				auto iter = gThreadInfoList.find(threadId);

				if (iter == gThreadInfoList.end())
				{
					auto str = std::format(response, seq, command, threadId, frameId, "The thread ID is invalid");
					gDbgSvr.send(str);
					gDbgSvr.outputDebugText("error", str);
					return;
				}

				auto& [threadSignal, threadCommand, threadContext, t1, t2, t3] = iter->second;

				for (int i = 0;;)
				{
					if (threadCommand != VisualStudioCommand::VS_COMMAND_PAUSE && !threadSignal.isThreadWaiting())
					{
						if (++i > 10)
						{
							auto str = std::format(response, seq, command, threadId, frameId, error_msg);
							gDbgSvr.send(str);
							gDbgSvr.outputDebugText("error", str);
							return;
						}
						else
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
							continue;
						}
					}

					break;
				}

				this->threadSignal = &threadSignal;
				this->threadCommand = &threadCommand;
				this->threadContext = threadContext;
			}

			ThreadSignal* threadSignal = nullptr;
			std::atomic<VisualStudioCommand>* threadCommand = nullptr;
			asIScriptContext* threadContext = nullptr;
		};

		try
		{
			constexpr auto response = JSON_LINE_FMT(
				{
					"type" : "response",
					"requestSeq" : {},
					"command" : "{}",
					"success" : {},
					"message" : "{}"
				});

			auto type = GetJsonValue(json, "type");
			auto seq = std::stoll(GetJsonValue(json, "seq"));
			auto command = GetJsonValue(json, "command");

			SWITCH(command)
			{
				CASE("start")
				{
					if (gThreadEngineList.size())
					{
						auto str = std::format(response, seq, command, true, "");
						gDbgSvr.send(str);
					}
					else
					{
						auto str = std::format(response, seq, command, false, "No script engine available");
						gDbgSvr.send(str);
					}

					BREAK;
				}
				CASE("pause")
				{
					auto str = std::format(response, seq, command, false, "The pause operation is not supported");
					gDbgSvr.send(str);

					BREAK;
				}
				CASE("continue")
				{
					auto threadId = std::stol(GetJsonValue(json, "threadId"));
					error_check error(seq, command, threadId, "The thread is already running");
					auto& [threadSignal, threadCommand, threadContext] = error;

					if (threadSignal)
					{
						auto str = std::format(response, seq, command, true, "");
						gDbgSvr.send(str);

						constexpr auto event = JSON_LINE_FMT(
							{
								"type":"event",
								"event" : "continued",
								"threadId" : {}
							});

						*threadCommand = VisualStudioCommand::VS_COMMAND_CONTINUE;
						threadSignal->setSignal();

						str = std::format(event, threadId);
						gDbgSvr.send(str);
					}

					BREAK;
				}
				CASE("step")
				{
					auto threadId = std::stol(GetJsonValue(json, "threadId"));
					error_check error(seq, command, threadId, "The thread is already running");
					auto& [threadSignal, threadCommand, threadContext] = error;

					if (threadSignal)
					{
						auto stepKind = GetJsonValue(json, "stepKind");

						SWITCH(stepKind)
						{
							CASE("STEP_INTO")
							{
								*threadCommand = VisualStudioCommand::VS_COMMAND_STEP_INTO;
								threadSignal->setSignal();
								BREAK;
							}
							CASE("STEP_OVER")
							{
								*threadCommand = VisualStudioCommand::VS_COMMAND_STEP_OVER;
								threadSignal->setSignal();
								BREAK;
							}
							CASE("STEP_OUT")
							{
								*threadCommand = VisualStudioCommand::VS_COMMAND_STEP_OUT;
								threadSignal->setSignal();
								BREAK;
							}
							DEFAULT()
							{
								auto str = std::format(response, seq, command, false, "The step kind is invalid");
								gDbgSvr.send(str);
								return;
							}
						}

						auto str = std::format(response, seq, command, true, "");
						gDbgSvr.send(str);
					}

					BREAK;
				}
				CASE("stop")
				{
					ClearBreakpoints();
					BREAK;
				}
				CASE("ready")
				{
					gDbgSvr.mReady = true;
					BREAK;
				}
				CASE("set_breakpoint")
				{
					BreakpointInfo bp =
					{
						GetJsonValue(json,"file"),
						std::stol(GetJsonValue(json,"line")),
						GetJsonValue(json,"function"),
						std::stol(GetJsonValue(json,"functionLineOffset")),
						GetJsonValue(json,"conditionType"),
						GetJsonValue(json,"condition"),
						GetJsonValue(json,"enabled") == "true" ? true : false,
					};

					auto file = bp.mFile;
					if (auto pos = file.find(BASE_DIRECTORY); pos != std::string::npos)
						file.erase(0, pos);

					for (auto pos = file.find('\\'); pos != std::string::npos; pos = file.find('\\', pos))
						file.replace(pos, 1, "/");

					CheckConditionBreakpoint(bp);
					gBreakpointList[file][bp.mLine] = std::move(bp);

					BREAK;
				}
				CASE("remove_breakpoint")
				{
					std::string file = GetJsonValue(json, "file");
					int line = std::stol(GetJsonValue(json, "line"));

					if (auto pos = file.find(BASE_DIRECTORY); pos != std::string::npos)
						file.erase(0, pos);

					for (auto pos = file.find('\\'); pos != std::string::npos; pos = file.find('\\', pos))
						file.replace(pos, 1, "/");

					gBreakpointList[file].erase(line);

					BREAK;
				}
				CASE("get_threads")
				{
					GetThreads(seq);
					BREAK;
				}
				CASE("get_stack")
				{
					auto threadId = std::stol(GetJsonValue(json, "threadId"));
					error_check error(seq, command, threadId, "The stack cannot be viewed in the running state");
					auto& [threadSignal, threadCommand, threadContext] = error;

					if (threadSignal)
						GetStackFrames(threadContext, seq, threadId);

					BREAK;
				}
				CASE("get_scope")
				{
					auto threadId = std::stol(GetJsonValue(json, "threadId"));
					auto frameId = std::stol(GetJsonValue(json, "frameId"));
					error_check error(seq, command, threadId, frameId, "The scopes cannot be viewed in the running state");
					auto& [threadSignal, threadCommand, threadContext] = error;

					if (threadSignal)
						GetFrameScope(threadContext, seq, threadId, frameId);

					BREAK;
				}
				CASE("get_property")
				{
					auto threadId = std::stol(GetJsonValue(json, "threadId"));
					auto frameId = std::stol(GetJsonValue(json, "frameId"));
					auto addr = std::stoll(GetJsonValue(json, "addr"));
					auto typeId = std::stol(GetJsonValue(json, "typeId"));
					auto start = std::stol(GetJsonValue(json, "start"));
					auto count = std::stol(GetJsonValue(json, "count"));

					error_check error(seq, command, threadId, frameId, "The property cannot be viewed in the running state");
					auto& [threadSignal, threadCommand, threadContext] = error;

					if (threadSignal)
						GetProperty(threadContext, seq, threadId, frameId, addr, typeId, start, count);

					BREAK;
				}
				CASE("get_evaluation")
				{
					auto threadId = std::stol(GetJsonValue(json, "threadId"));
					auto frameId = std::stol(GetJsonValue(json, "frameId"));
					auto expression = GetJsonValue(json, "expression");
					error_check error(seq, command, threadId, frameId, "The get_evaluation cannot be viewed in the running state");
					auto& [threadSignal, threadCommand, threadContext] = error;

					if (threadSignal)
						GetEvaluation(threadContext, seq, threadId, frameId, expression, threadContext->GetEngine());

					BREAK;
				}
				CASE("set_variable")
				{
					auto threadId = std::stol(GetJsonValue(json, "threadId"));
					auto frameId = std::stol(GetJsonValue(json, "frameId"));
					auto addr = std::stoll(GetJsonValue(json, "addr"));
					auto typeId = std::stol(GetJsonValue(json, "typeId"));
					auto value = GetJsonValue(json, "value");
					error_check error(seq, command, threadId, frameId, "The variable cannot be viewed in the running state");
					auto& [threadSignal, threadCommand, threadContext] = error;

					if (threadSignal)
						SetVariable(threadContext, seq, threadId, frameId, addr, typeId, value);

					BREAK;
				}
				DEFAULT()
				{
					auto str = std::format(response, seq, command, false, "The command is invalid");
					gDbgSvr.send(str);
					BREAK;
				}
			}
		}
		catch (...)
		{
			SYS_LOG("The script debugger encountered an exception");
		}
	}
}

std::string Modify(const std::string& _str, asETypeModifiers typeMod)
{
	if (typeMod == asTM_NONE)
		return _str;

	auto str = _str;

	if (typeMod & asTM_CONST)
		str.insert(0, "const ");

	typeMod = asETypeModifiers(int(typeMod) & 0x03);

	if (typeMod == asTM_INREF)
		str += " & in";
	else if (typeMod == asTM_OUTREF)
		str += " & out";
	else if (typeMod == asTM_INOUTREF)
		str += " &";

	return str;
};

std::string ToJsonString(const char* _name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)
{
	constexpr auto variable = JSON_FMT(
		{
			"type" : "{}",
			"typeId" : {},
			"size" : {},
			"addr" : {},
			"name" : "{}",
			"value" : "{}"
		});

	/*constexpr auto variableArr = JSON_FMT(
		{
			"type" : "{}",
			"typeId" : {},
			"size" : {},
			"addr":{},
			"name" : "{}",
			"value" : "{}",
			"start" : {},
			"count" : {},
			"elements" : [
				variable or variableArr
			]
		});*/

	auto name = _name;
	if (name == nullptr)
		name = "null";

	if (value == nullptr)
	{
		if (asITypeInfo* t = engine->GetTypeInfoById(typeId))
		{
			if (auto nameSpace = t->GetNamespace(); nameSpace && nameSpace[0])
				return std::format(variable, Modify(std::string(nameSpace) + "::" + t->GetName(), typeMod), 0, 0, rand() | (int64_t(rand()) << 32), name, "null");
			else
				return std::format(variable, Modify(t->GetName(), typeMod), 0, 0, rand() | (int64_t(rand()) << 32), name, "null");
		}
		else
			return std::format(variable, "null", 0, 0, rand() | (int64_t(rand()) << 32), name, "null");
	}

	if (_name != nullptr)
	{
		if (typeId == asTYPEID_VOID)
			return std::format(variable, Modify("void", typeMod), typeId, 0, (int64_t)value, name, "void");
		else if (typeId == asTYPEID_BOOL)
			return std::format(variable, Modify("bool", typeMod), typeId, 0, (int64_t)value, name, *(bool*)value ? "true" : "false");
		else if (typeId == asTYPEID_INT8)
			return std::format(variable, Modify("int8", typeMod), typeId, 0, (int64_t)value, name, (int)*(signed char*)value);
		else if (typeId == asTYPEID_INT16)
			return std::format(variable, Modify("int16", typeMod), typeId, 0, (int64_t)value, name, (int)*(signed short*)value);
		else if (typeId == asTYPEID_INT32)
			return std::format(variable, Modify("int32", typeMod), typeId, 0, (int64_t)value, name, *(signed int*)value);
		else if (typeId == asTYPEID_INT64)
			return std::format(variable, Modify("int64", typeMod), typeId, 0, (int64_t)value, name, *(asINT64*)value);
		else if (typeId == asTYPEID_UINT8)
			return std::format(variable, Modify("uint8", typeMod), typeId, 0, (int64_t)value, name, (unsigned int)*(unsigned char*)value);
		else if (typeId == asTYPEID_UINT16)
			return std::format(variable, Modify("uint16", typeMod), typeId, 0, (int64_t)value, name, (unsigned int)*(unsigned short*)value);
		else if (typeId == asTYPEID_UINT32)
			return std::format(variable, Modify("uint32", typeMod), typeId, 0, (int64_t)value, name, *(unsigned int*)value);
		else if (typeId == asTYPEID_UINT64)
			return std::format(variable, Modify("uint64", typeMod), typeId, 0, (int64_t)value, name, *(asQWORD*)value);
		else if (typeId == asTYPEID_FLOAT)
			return std::format(variable, Modify("float", typeMod), typeId, 0, (int64_t)value, name, *(float*)value);
		else if (typeId == asTYPEID_DOUBLE)
			return std::format(variable, Modify("double", typeMod), typeId, 0, (int64_t)value, name, *(double*)value);
		else if ((typeId & asTYPEID_MASK_OBJECT) == 0)
		{
			if (asITypeInfo* t = engine->GetTypeInfoById(typeId))
			{
				for (int n = t->GetEnumValueCount(); n-- > 0; )
				{
					asINT64 enumVal;
					const char* enumName = t->GetEnumValueByIndex(n, &enumVal);
					auto _enumVal = *(asINT32*)value;
					if (enumVal == _enumVal)
					{
						auto str = std::format("{}({})", enumName, enumVal);
						if (auto nameSpace = t->GetNamespace(); nameSpace && nameSpace[0])
							return std::format(variable, Modify(std::string(nameSpace) + "::" + t->GetName(), typeMod), typeId, 0, (int64_t)value, name, str);
						else
							return std::format(variable, Modify(t->GetName(), typeMod), typeId, 0, (int64_t)value, name, str);
					}
				}
			}
			return std::format(variable, Modify("enum", typeMod), typeId, 0, (int64_t)value, name, *(asQWORD*)value);
		}
		else if (typeId & asTYPEID_SCRIPTOBJECT)
		{
			auto scrObj = (asIScriptObject*)value;

			std::string ptr;
			if (typeId & asTYPEID_OBJHANDLE)
			{
				ptr = "@";
				scrObj = *(asIScriptObject**)value;
				if (scrObj == nullptr)
				{
					if (auto t = engine->GetTypeInfoById(typeId))
					{
						if (auto nameSpace = t->GetNamespace(); nameSpace && nameSpace[0])
							return std::format(variable, Modify(std::string(nameSpace) + "::" + t->GetName() + ptr, typeMod), typeId, 0, (int64_t)value, name, "null");
						else
							return std::format(variable, Modify(t->GetName() + ptr, typeMod), typeId, 0, (int64_t)value, name, "null");
					}
					else
						return std::format(variable, Modify("null", typeMod), typeId, 0, (int64_t)value, name, "null");
				}
			}

			auto t = scrObj->GetObjectType();

			if (auto propertyCount = scrObj->GetPropertyCount(); propertyCount > 0)
			{
				std::string members;

				auto getValue = [](void* ptr, int typeId, asIScriptEngine* engine)->std::string
					{
						if (ptr == nullptr)
							return "null";

						switch (typeId)
						{
						case asTYPEID_VOID:
							return "void";
						case asTYPEID_BOOL:
							return *(bool*)ptr ? "true" : "false";
						case asTYPEID_INT8:
							return std::to_string((int64_t)(*(char*)ptr));
						case asTYPEID_INT16:
							return std::to_string((int64_t)(*(short*)ptr));
						case asTYPEID_INT32:
							return std::to_string((int64_t)(*(int*)ptr));
						case asTYPEID_INT64:
							return std::to_string((int64_t)(*(long long*)ptr));
						case asTYPEID_UINT8:
							return std::to_string((uint64_t)(*(unsigned char*)ptr));
						case asTYPEID_UINT16:
							return std::to_string((uint64_t)(*(unsigned short*)ptr));
						case asTYPEID_UINT32:
							return std::to_string((uint64_t)(*(unsigned int*)ptr));
						case asTYPEID_UINT64:
							return std::to_string((uint64_t)(*(unsigned long long*)ptr));
						case asTYPEID_FLOAT:
							return std::to_string((uint64_t)(*(float*)ptr));
						case asTYPEID_DOUBLE:
							return std::to_string((uint64_t)(*(double*)ptr));
						}

						if (asITypeInfo* t = engine->GetTypeInfoById(typeId))
						{
							if ((typeId & asTYPEID_MASK_OBJECT) == 0)
							{
								for (int n = t->GetEnumValueCount(); n-- > 0; )
								{
									asINT64 enumVal;
									const char* enumName = t->GetEnumValueByIndex(n, &enumVal);
									if (enumVal == *(asINT64*)ptr)
										return std::format("{}({})", enumName, enumVal);
								}
							}

							if (t->GetName() == std::string("string"))
								return *(std::string*)ptr;

							if (auto funcDef = t->GetFuncdefSignature())
								return funcDef->GetDeclaration(true, true, true);
						}

						return "{...}";
					};

				for (asUINT n = 0; n < propertyCount; n++)
				{
					auto childName = scrObj->GetPropertyName(n);
					auto childAddr = scrObj->GetAddressOfProperty(n);
					auto childTypeId = scrObj->GetPropertyTypeId(n);

					members += (childName + std::string(" = ") + getValue(childAddr, childTypeId, engine) + ", ");
				}

				if (!members.empty())
				{
					members.pop_back();
					members.pop_back();
				}

				if (auto nameSpace = t->GetNamespace(); nameSpace && nameSpace[0])
					return std::format(variable, Modify(std::string(nameSpace) + "::" + t->GetName() + ptr, typeMod), typeId, -1, (int64_t)value, name, StringSymbalEscape(members));
				else
					return std::format(variable, Modify(t->GetName() + ptr, typeMod), typeId, -1, (int64_t)value, name, StringSymbalEscape(members));
			}
			else
			{
				if (auto nameSpace = t->GetNamespace(); nameSpace && nameSpace[0])
					return std::format(variable, Modify(std::string(nameSpace) + "::" + t->GetName() + ptr, typeMod), typeId, 0, (int64_t)value, name, "empty");
				else
					return std::format(variable, Modify(t->GetName() + ptr, typeMod), typeId, 0, (int64_t)value, name, "empty");
			}
		}
		else
		{
			asITypeInfo* t = engine->GetTypeInfoById(typeId);
			if (t == nullptr)
				return std::format(variable, "error", typeId, 0, (int64_t)value, name, "error");

			if (t->GetName() == std::string("string"))
			{
				std::string& str = *(std::string*)value;

				return std::format(variable, Modify("string", typeMod), typeId, 0, (int64_t)value, name, StringSymbalEscape(str));
			}

			std::string ptr;
			if (typeId & asTYPEID_OBJHANDLE)
			{
				ptr = "@";
				if (*(void**)value == nullptr)
				{
					if (auto nameSpace = t->GetNamespace(); nameSpace && nameSpace[0])
						return std::format(variable, Modify(std::string(nameSpace) + "::" + t->GetName() + ptr, typeMod), typeId, 0, (int64_t)value, name, "null");
					else
						return std::format(variable, Modify(t->GetName() + ptr, typeMod), typeId, 0, (int64_t)value, name, "null");
				}
			}

			auto _t = t;
			if (t->GetFlags() & asOBJ_TEMPLATE)
			{
				auto t_n = t->GetName();
				_t = engine->GetTypeInfoByName(t_n);
			}

			if (gThreadEngineList.size() > 1)
			{
				if (auto iter = gDbgSvr.mToStringCallbacks[GetEngineThreadId(engine)].find(_t); iter != gDbgSvr.mToStringCallbacks[GetEngineThreadId(engine)].end())
					return iter->second(name, value, typeId, start, count, size, engine, typeMod);
			}
			else
			{
				if (auto iter = gDbgSvr.mToStringCallbacks.begin()->second.find(_t); iter != gDbgSvr.mToStringCallbacks.begin()->second.end())
					return iter->second(name, value, typeId, start, count, size, engine, typeMod);
			}

			if (auto funcDef = t->GetFuncdefSignature())
			{
				auto funcDecl = funcDef->GetDeclaration(true, true, true);
				return std::format(variable, Modify(t->GetName() + ptr, typeMod), typeId, 0, (int64_t)value, name, funcDecl);
			}

			if (auto nameSpace = t->GetNamespace(); nameSpace && nameSpace[0])
				return std::format(variable, Modify(std::string(nameSpace) + "::" + t->GetName() + ptr, typeMod), typeId, 0, (int64_t)value, name, "NotImpl");
			else
				return std::format(variable, Modify(t->GetName() + ptr, typeMod), typeId, 0, (int64_t)value, name, "NotImpl");
		}
	}
	else // "get_property"
	{
		if (typeId & asTYPEID_SCRIPTOBJECT)
		{
			auto obj = (asIScriptObject*)value;

			if (typeId & asTYPEID_OBJHANDLE)
				obj = *(asIScriptObject**)value;

			std::string variables;

			if (obj != nullptr)
			{
				size = count = obj->GetPropertyCount();

				for (asUINT n = 0; n < obj->GetPropertyCount(); n++)
				{
					auto childName = obj->GetPropertyName(n);
					auto childAddr = obj->GetAddressOfProperty(n);
					auto childTypeId = obj->GetPropertyTypeId(n);

					int _size = 0;
					unsigned int _count = 0;
					variables += ToJsonString(childName, childAddr, childTypeId, 0, _count, _size, engine, asTM_NONE);
					variables += ",";
				}
			}

			return variables;
		}
		else
		{
			if (typeId & asTYPEID_OBJHANDLE)
			{
				if (*(void**)value == nullptr)
					return "";
			}

			if (asITypeInfo* t = engine->GetTypeInfoById(typeId))
			{
				auto _t = t;

				if (t->GetFlags() & asOBJ_TEMPLATE)
				{
					auto t_n = t->GetName();
					_t = engine->GetTypeInfoByName(t_n);
				}

				if (gThreadEngineList.size() > 1)
				{
					if (auto iter = gDbgSvr.mToStringCallbacks[GetEngineThreadId(engine)].find(_t); iter != gDbgSvr.mToStringCallbacks[GetEngineThreadId(engine)].end())
						return iter->second(nullptr, value, typeId, start, count, size, engine, typeMod);
				}
				else
				{
					if (auto iter = gDbgSvr.mToStringCallbacks.begin()->second.find(_t); iter != gDbgSvr.mToStringCallbacks.begin()->second.end())
						return iter->second(nullptr, value, typeId, start, count, size, engine, typeMod);
				}
			}

			return "";
		}
	}

	return std::format(variable, "NotImpl", typeId, 0, (int64_t)value, name, "NotImpl");
}

void SetToStringCallback(asITypeInfo* t, std::function<std::string(const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)> func)
{
	gDbgSvr.mToStringCallbacks[GetCurrentThreadId()][t] = func;
}

void CreateScriptEngineCallback(asIScriptEngine* pScriptEngine)
{
	int threadId = GetCurrentThreadId();
	gThreadEngineList.emplace_back(threadId, pScriptEngine);
	gThreadInfoList[threadId];
}

void RunDebuggerServer(volatile bool& bTerminated, uint16_t uServerPort)
{
	if (!gDbgSvr.bind(uServerPort))
		throw std::runtime_error("Failed to bind debugger server.");

	if (!gDbgSvr.listen())
		throw std::runtime_error("Failed to listen on debugger server.");

	std::string remainData;

	while (!bTerminated)
	{
		struct sockaddr_in clientAddr = {};
		gDbgSvr.mSocketVS = gDbgSvr.accept(clientAddr);
		if (int(gDbgSvr.mSocketVS) < 0)
		{
			SYS_LOG("Stopped debug server");
			break;
		}

		SYS_LOG("The Visual Studio connected");

		while (true)
		{
			char recvBuffer[4 * 1024 + 1];
			auto recvLength = gDbgSvr.receive(gDbgSvr.mSocketVS, recvBuffer, sizeof(recvBuffer) - 1);
			if (recvLength == -1 || recvLength == 0)
			{
				gDbgSvr.closeSocketVS();
				ClearBreakpoints();
				SYS_LOG("The Visual Studio disconnected");
				break;
			}
			else
				gDbgSvr.mConnectedCount++;

			if (true)
			{
				recvBuffer[recvLength] = '\0';
				std::stringstream ss;
				std::string line;

				ss << remainData;
				ss << recvBuffer;

				while (ss.str().find('\n', ss.tellg()) != std::string::npos && std::getline(ss, line))
				{
#ifdef _DEBUG
					gDbgSvr.outputDebugText("echo", line);
#endif
					HandleCommandFromVSX(line);
				}

				remainData = ss.str().substr(ss.tellg());
			}
		}
	}

	gDbgSvr.closeSocketMy();

	for (auto& [threadId, threadInfo] : gThreadInfoList)
	{
		threadInfo.command = VisualStudioCommand::VS_COMMAND_CONTINUE;
		threadInfo.signal.setSignal();
	}
}

namespace
{
	std::string GetVariableValue(void* ptr, int typeId, const std::vector<std::string>& variableName, int& valueType, asIScriptEngine* engine)
	{
		auto getValue = [](void* ptr, int typeId, int& valueType, asIScriptEngine* engine)->std::string
			{
				switch (typeId)
				{
				case asTYPEID_BOOL:
					valueType = 0;
					return *(bool*)ptr ? "true" : "false";
				case asTYPEID_INT8:
					valueType = 1;
					return std::to_string((int64_t)(*(char*)ptr));
				case asTYPEID_INT16:
					valueType = 1;
					return std::to_string((int64_t)(*(short*)ptr));
				case asTYPEID_INT32:
					valueType = 1;
					return std::to_string((int64_t)(*(int*)ptr));
				case asTYPEID_INT64:
					valueType = 1;
					return std::to_string((int64_t)(*(long long*)ptr));
				case asTYPEID_UINT8:
					valueType = 1;
					return std::to_string((uint64_t)(*(unsigned char*)ptr));
				case asTYPEID_UINT16:
					valueType = 1;
					return std::to_string((uint64_t)(*(unsigned short*)ptr));
				case asTYPEID_UINT32:
					valueType = 1;
					return std::to_string((uint64_t)(*(unsigned int*)ptr));
				case asTYPEID_UINT64:
					valueType = 1;
					return std::to_string((uint64_t)(*(unsigned long long*)ptr));
				case asTYPEID_FLOAT:
					valueType = 2;
					return std::to_string((uint64_t)(*(float*)ptr));
				case asTYPEID_DOUBLE:
					valueType = 2;
					return std::to_string((uint64_t)(*(double*)ptr));
				}

				if (asITypeInfo* t = engine->GetTypeInfoById(typeId))
				{
					if (t->GetName() == std::string("string"))
					{
						valueType = 0;
						return *(std::string*)ptr;
					}
				}

				return "";
			};

		if (MatchVariableName(variableName, ptr, typeId))
			return getValue(ptr, typeId, valueType, engine);

		return "";
	}

	void DebugLineCallback(asIScriptContext* ctx, void* dbg)
	{
		const int lineType = 0x00ff00ff; //Ensure there is no conflict with other types
		const int stackType = lineType + 1; //Ensure there is no conflict with other types
		const int commandType = stackType + 1; //Ensure there is no conflict with other types

		auto threadId = GetCurrentThreadId();
		auto& [threadSignal, threadCommand, threadContext, lastCodeFile, lastCodeLine, lastStackLevel] = gThreadInfoList[threadId];

		if (threadContext != ctx)
		{
			lastCodeLine = (uint64_t)ctx->GetUserData(lineType);
			lastStackLevel = (uint64_t)ctx->GetUserData(stackType);
			if (lastStackLevel != 0)
				threadCommand = VisualStudioCommand((uint64_t)ctx->GetUserData(commandType));
		}

		threadSignal.setPreWaiting(true);
		threadContext = ctx;

		if (gDbgSvr.mSocketVS == SOCKET(-1))
		{
			ctx->ClearExceptionCallback();
			ctx->ClearLineCallback();
			threadSignal.clearPreWaitingSignal();
			return;
		}

		int moduleLine = ctx->GetLineNumber(0, 0, 0);
		asIScriptFunction* function = ctx->GetFunction();
		auto module = function->GetModule();
		if (module == nullptr || moduleLine <= 0)
		{
			threadSignal.clearPreWaitingSignal();
			return;
		}

		auto [codeFile, codeLine] = GetFileLineByModuleLine(module, moduleLine);

		auto waitingResponse = [&]()
			{
				threadCommand = VisualStudioCommand::VS_COMMAND_PAUSE;
				threadSignal.waitForSignal(gDbgSvr.mSocketVS);
				if (gDbgSvr.mSocketVS == SOCKET(-1))
					threadCommand = VisualStudioCommand::VS_COMMAND_CONTINUE;

				lastCodeFile = codeFile;
				lastCodeLine = codeLine;
				lastStackLevel = ctx->GetCallstackSize();

				ctx->SetUserData((void*)lastCodeLine, lineType);
				ctx->SetUserData((void*)lastStackLevel, stackType);
				VisualStudioCommand _threadCommand = threadCommand;
				ctx->SetUserData((void*)_threadCommand, commandType);
			};

		switch (threadCommand)
		{
		case VisualStudioCommand::VS_COMMAND_STEP_INTO:
		{
			if (codeLine != lastCodeLine || lastCodeFile != codeFile)
			{
				constexpr auto event = JSON_LINE_FMT(
					{
						"type":"event",
						"event" : "stopped",
						"reason" : "step",
						"threadId" : {},
						"file" : "{}",
						"line" : {}
					});

				auto str = std::format(event, threadId, codeFile, codeLine);
				gDbgSvr.send(str);

				waitingResponse();
				return;
			}

			break;
		}
		case VisualStudioCommand::VS_COMMAND_STEP_OVER:
		{
			auto currCallstackPos = ctx->GetCallstackSize();

			if (currCallstackPos <= lastStackLevel && (codeLine != lastCodeLine || lastCodeFile != codeFile))
			{
				constexpr auto event = JSON_LINE_FMT(
					{
						"type":"event",
						"event" : "stopped",
						"reason" : "step",
						"threadId" : {},
						"file" : "{}",
						"line" : {}
					});

				auto str = std::format(event, threadId, codeFile, codeLine);
				gDbgSvr.send(str);

				waitingResponse();
				return;
			}

			break;
		}
		case VisualStudioCommand::VS_COMMAND_STEP_OUT:
		{
			auto currCallstackPos = ctx->GetCallstackSize();

			if (currCallstackPos < lastStackLevel)
			{
				constexpr auto event = JSON_LINE_FMT(
					{
						"type":"event",
						"event" : "stopped",
						"reason" : "step",
						"threadId" : {},
						"file" : "{}",
						"line" : {}
					});

				auto str = std::format(event, threadId, codeFile, codeLine);
				gDbgSvr.send(str);

				waitingResponse();
				return;
			}

			break;
		}
		}

		if (auto iter = gBreakpointList.find(codeFile); iter != gBreakpointList.end())
		{
			auto engine = ctx->GetEngine();

			int nextLine = moduleLine;
			auto lineEntryCount = function->GetLineEntryCount();

			for (int i = 0, j = lineEntryCount / 2, k = lineEntryCount, c = 0; c < lineEntryCount; c++)
			{
				int row = 0;
				function->GetLineEntry(j, &row, 0, 0, 0);
				if (row < moduleLine)
				{
					i = j;
					j = j + (k - j) / 2;
				}
				else if (row > moduleLine)
				{
					k = j;
					j = i + (j - i) / 2;
				}
				else
				{
					function->GetLineEntry(j + 1, &row, 0, 0, 0);
					nextLine = row;
					break;
				}
			}
			size_t occupiedLines = nextLine != moduleLine ? (nextLine - moduleLine) : 1;

			if (false)
			{
				std::vector<int> funcLineEntry;
				for (int i = 0; i < lineEntryCount; i++)
				{
					int row = 0;
					function->GetLineEntry(i, &row, 0, 0, 0);
					auto [t1, t2] = GetFileLineByModuleLine(module, row);
					funcLineEntry.push_back(int(t2));
				}
				lineEntryCount = lineEntryCount;
			}

			for (auto& [breakLine, breakInfo] : iter->second)
			{
				if (breakInfo.mEnabled)
				{
					auto isLambda1 = function->GetName()[0] == '$';
					auto isLambda2 = breakInfo.mFunction.find("function(") == 0 || breakInfo.mFunction.find("func(") == 0;

					if (((!isLambda1 && !isLambda2) || (isLambda1 && isLambda2)) &&
						breakLine >= codeLine && breakLine < codeLine + occupiedLines)
					{
						if (breakInfo.conditionValid)
						{
							void* ptr = nullptr;
							int typeId = 0;

							for (asUINT n = function->GetVarCount(); n-- > 0; )
							{
								const char* varName = 0;
								ctx->GetVar(n, 0, &varName, &typeId);
								if (ctx->IsVarInScope(n) && varName != 0 && breakInfo.variableName.front() == varName)
								{
									ptr = ctx->GetAddressOfVar(n);
									break;
								}
							}

							if (ptr != nullptr)
							{
								int valueType = 0;
								auto value = GetVariableValue(ptr, typeId, breakInfo.variableName, valueType, engine);
								if (!value.empty())
								{
									bool conditionSatisfied = false;
									switch (breakInfo.conditionOpCode)
									{
									case 1: // ==
									{
#define COMPARE_VALUE(op) \
									switch (valueType) \
									{\
									case 0:\
										if (value op breakInfo.conditionOpValue)\
											conditionSatisfied = true;\
										break;\
									case 1:\
										if (std::atoi(value.c_str()) op std::atoi(breakInfo.conditionOpValue.c_str()))\
											conditionSatisfied = true;\
										break;\
									case 2:\
										if (std::atof(value.c_str()) op std::atof(breakInfo.conditionOpValue.c_str()))\
											conditionSatisfied = true;\
										break;\
									}

										COMPARE_VALUE(== );
										break;
									}
									case 2: // !=
									{
										COMPARE_VALUE(!= );
										break;
									}
									case 3: // >=
									{
										COMPARE_VALUE(>= );
										break;
									}
									case 4: // >
									{
										COMPARE_VALUE(> );
										break;
									}
									case 5: // <=
									{
										COMPARE_VALUE(<= );
										break;
									}
									case 6: // <
									{
										COMPARE_VALUE(< );
										break;
									}
									}

									if (!conditionSatisfied)
										continue;
								}
							}
						}

						constexpr auto event = JSON_LINE_FMT(
							{
								"type":"event",
								"event" : "stopped",
								"reason" : "breakpoint",
								"threadId" : {},
								"file" : "{}",
								"line" : {}
							});

						auto str = std::format(event, threadId, codeFile, breakLine);
						gDbgSvr.send(str);

						waitingResponse();
						return;
					}
				}
			}
		}

		threadSignal.clearPreWaitingSignal();
	}

	void ExceptionCallback(asIScriptContext* ctx, void* dbg)
	{
		auto threadId = GetCurrentThreadId();
		auto& [threadSignal, threadCommand, threadContext, lastCodeFile, lastCodeLine, lastStackLevel] = gThreadInfoList[threadId];

		threadSignal.setPreWaiting(true);
		threadContext = ctx;

		if (gDbgSvr.mSocketVS == SOCKET(-1))
		{
			ctx->ClearExceptionCallback();
			ctx->ClearLineCallback();
			threadSignal.clearPreWaitingSignal();
			return;
		}

		auto exceptionFunction = ctx->GetExceptionFunction();
		auto  exceptionLineNumber = ctx->GetExceptionLineNumber(0, 0);
		auto exceptionMessage = ctx->GetExceptionString();
		auto module = exceptionFunction->GetModule();
		if (module == nullptr)
		{
			threadSignal.clearPreWaitingSignal();
			return;
		}

		auto [codeFile, codeLine] = GetFileLineByModuleLine(module, exceptionLineNumber);

		constexpr auto event = JSON_LINE_FMT(
			{
				"type":"event",
				"event" : "stopped",
				"reason" : "exception",
				"threadId" : {},
				"file" : "{}",
				"line" : {},
				"exceptionName" : "{}",
				"exceptionMessage" : "{}"
			});

		auto exceptionName = ctx->WillExceptionBeCaught() ? "capturable" : "uncatchable";
		auto str = std::format(event, threadId, codeFile, codeLine, exceptionName, exceptionMessage);
		gDbgSvr.send(str);

		threadCommand = VisualStudioCommand::VS_COMMAND_PAUSE;
		threadSignal.waitForSignal(gDbgSvr.mSocketVS);
		if (gDbgSvr.mSocketVS == SOCKET(-1))
			threadCommand = VisualStudioCommand::VS_COMMAND_CONTINUE;

		lastCodeFile = codeFile;
		lastCodeLine = codeLine;
		lastStackLevel = ctx->GetCallstackSize();
	}
}

void PrepareExecutionCallback(asIScriptContext* ctx)
{
	if (gDbgSvr.mSocketVS != SOCKET(-1))
	{
		auto threadId = GetCurrentThreadId();
		auto& [threadSignal, threadCommand, threadContext, t1, t2, t3] = gThreadInfoList[threadId];

		threadContext = ctx;

		if (threadCommand != VisualStudioCommand::VS_COMMAND_CONTINUE)
		{
			constexpr auto event = JSON_LINE_FMT(
				{
					"type":"event",
					"event" : "continued",
					"threadId" : {}
				});

			threadCommand = VisualStudioCommand::VS_COMMAND_CONTINUE;

			auto str = std::format(event, threadId);
			gDbgSvr.send(str);
		}

		ctx->SetLineCallback(asFUNCTION(DebugLineCallback), nullptr, asCALL_CDECL);
		ctx->SetExceptionCallback(asFUNCTION(ExceptionCallback), nullptr, asCALL_CDECL);
	}
}
