
#include <iostream>

#include "../angelscript/sdk/angelscript/include/angelscript.h"
#define private public
#define protected public
#include "../angelscript/sdk/add_on/autowrapper/aswrappedcall.h"
#include "../angelscript/sdk/add_on/contextmgr/contextmgr.h"
#include "../angelscript/sdk/add_on/datetime/datetime.h"
#include "../angelscript/sdk/add_on/debugger/debugger.h"
#include "../angelscript/sdk/add_on/scriptany/scriptany.h"
#include "../angelscript/sdk/add_on/scriptarray/scriptarray.h"
#include "../angelscript/sdk/add_on/scriptbuilder/scriptbuilder.h"
#include "../angelscript/sdk/add_on/scriptdictionary/scriptdictionary.h"
#include "../angelscript/sdk/add_on/scriptfile/scriptfile.h"
#include "../angelscript/sdk/add_on/scriptfile/scriptfilesystem.h"
#include "../angelscript/sdk/add_on/scriptgrid/scriptgrid.h"
#include "../angelscript/sdk/add_on/scripthandle/scripthandle.h"
#include "../angelscript/sdk/add_on/scripthelper/scripthelper.h"
#include "../angelscript/sdk/add_on/scriptmath/scriptmath.h"
#include "../angelscript/sdk/add_on/scriptmath/scriptmathcomplex.h"
#include "../angelscript/sdk/add_on/scriptsocket/scriptsocket.h"
#include "../angelscript/sdk/add_on/scriptstdstring/scriptstdstring.h"
#include "../angelscript/sdk/add_on/serializer/serializer.h"
#include "../angelscript/sdk/add_on/weakref/weakref.h"

#include "ScriptDebugger.h"

static std::string StringSymbalEscape(std::string str)
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

static std::string getVariableValue(void* ptr, int typeId, asIScriptEngine* engine, int depth = 1)
{
	if (typeId & asTYPEID_OBJHANDLE)
		ptr = *(void**)ptr;

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

	if ((typeId & asTYPEID_MASK_OBJECT) == 0)
	{
		if (asITypeInfo* t = engine->GetTypeInfoById(typeId))
		{
			for (int n = t->GetEnumValueCount(); n-- > 0; )
			{
				asINT64 enumVal;
				const char* enumName = t->GetEnumValueByIndex(n, &enumVal);
				auto _enumVal = *(asINT32*)ptr;
				if (enumVal == _enumVal)
					return std::format("{}({})", enumName, enumVal);
			}
		}
	}

	if (typeId & asTYPEID_SCRIPTOBJECT)
	{
		auto scrObj = (asIScriptObject*)ptr;

		if (auto propertyCount = scrObj->GetPropertyCount(); propertyCount > 0)
		{
			if (depth <= 0)
				return "{...}";

			std::string members = "{";

			for (asUINT n = 0; n < propertyCount; n++)
			{
				auto childName = scrObj->GetPropertyName(n);
				auto childAddr = scrObj->GetAddressOfProperty(n);
				auto childTypeId = scrObj->GetPropertyTypeId(n);

				members += (childName + std::string(" = ") + getVariableValue(childAddr, childTypeId, engine, depth - 1) + ", ");
			}

			if (members[0] != '{')
			{
				members.pop_back();
				members.pop_back();
			}

			members += "}";
			return members;
		}
		else
			return "empty";
	}

	if (asITypeInfo* t = engine->GetTypeInfoById(typeId))
	{
		if (auto funcDef = t->GetFuncdefSignature())
			return funcDef->GetDeclaration(true, true, true);

		if (t->GetName() == std::string("string"))
			return *(std::string*)ptr;

		if (t->GetName() == std::string("array"))
			return std::string("size = ") + std::to_string(((CScriptArray*)ptr)->GetSize());

		if (t->GetName() == std::string("dictionary"))
			return std::string("size = ") + std::to_string(((CScriptDictionary*)ptr)->GetSize());

		if (t->GetName() == std::string("grid"))
			return std::format("width = {}, height = {}", ((CScriptGrid*)ptr)->GetWidth(), ((CScriptGrid*)ptr)->GetHeight());
	}

	return "{...}";
};

void RegisterDbgToString(asIScriptEngine* engine)
{
	// You need to manually change the class member variable attribute to "public"

	static auto Modify = [](const std::string& _str, asETypeModifiers typeMod) -> std::string
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

#define	STR(s) #s
#define	JSON_FMT(...) STR({)STR(__VA_ARGS__)STR(});

	constexpr auto variable = JSON_FMT(
		{
			"type" : "{}",
			"typeId" : {},
			"size" : {},
			"addr" : {},
			"name" : "{}",
			"value" : "{}"
		});

	constexpr auto variableArr = JSON_FMT(
		{
			"type" : "{}",
			"typeId" : {},
			"size" : {},
			"addr" : {},
			"name" : "{}",
			"value" : "{}",
			"start" : {},
			"count" : {},
			"elements" : {}
		});

	static auto getElementsSize = [](void* ptr, int typeId, asIScriptEngine* engine)->int
		{
			if (typeId & asTYPEID_OBJHANDLE)
				ptr = *(void**)ptr;

			if (ptr == nullptr)
				return 0;

			switch (typeId)
			{
			case asTYPEID_VOID:
			case asTYPEID_BOOL:
			case asTYPEID_INT8:
			case asTYPEID_INT16:
			case asTYPEID_INT32:
			case asTYPEID_INT64:
			case asTYPEID_UINT8:
			case asTYPEID_UINT16:
			case asTYPEID_UINT32:
			case asTYPEID_UINT64:
			case asTYPEID_FLOAT:
			case asTYPEID_DOUBLE:
				return 0;
			}

			if ((typeId & asTYPEID_MASK_OBJECT) == 0)
				return 0;

			if (typeId & asTYPEID_SCRIPTOBJECT)
			{
				auto scrObj = (asIScriptObject*)ptr;

				return (int)scrObj->GetPropertyCount();
			}

			if (asITypeInfo* t = engine->GetTypeInfoById(typeId))
			{
				if (auto funcDef = t->GetFuncdefSignature())
					return 0;

				if (t->GetName() == std::string("string"))
					return 0;

				if (t->GetName() == std::string("array"))
					return ((CScriptArray*)ptr)->GetSize();

				if (t->GetName() == std::string("dictionary"))
					return ((CScriptDictionary*)ptr)->GetSize();

				if (t->GetName() == std::string("grid"))
					return ((CScriptGrid*)ptr)->GetWidth() * ((CScriptGrid*)ptr)->GetHeight();
			}

			return -1;
		};

	static auto getTypeName = [](int typeId, asIScriptEngine* engine)->std::string
		{
			if (typeId == asTYPEID_VOID)
				return "void";
			else if (typeId == asTYPEID_BOOL)
				return "bool";
			else if (typeId == asTYPEID_INT8)
				return "int8";
			else if (typeId == asTYPEID_INT16)
				return "int16";
			else if (typeId == asTYPEID_INT32)
				return "int32";
			else if (typeId == asTYPEID_INT64)
				return "int64";
			else if (typeId == asTYPEID_UINT8)
				return "uint8";
			else if (typeId == asTYPEID_UINT16)
				return "uint16";
			else if (typeId == asTYPEID_UINT32)
				return "uint32";
			else if (typeId == asTYPEID_UINT64)
				return "uint64";
			else if (typeId == asTYPEID_FLOAT)
				return "float";
			else if (typeId == asTYPEID_DOUBLE)
				return "double";

			std::string ptr;
			if (typeId & asTYPEID_OBJHANDLE)
				ptr = "@";

			if (asITypeInfo* t = engine->GetTypeInfoById(typeId))
				return std::string(t->GetName()) + ptr;

			return "unknown" + ptr;
		};

	if (auto t = engine->GetTypeInfoByName("array"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string ptr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					ptr = "@";
					_value = *(void**)value; // It is safe.
				}

				auto arr = (CScriptArray*)_value;
				auto elemTypeId = arr->GetElementTypeId();
				auto elemTypeName = getTypeName(elemTypeId, engine);

				size = (int)arr->GetSize();
				auto expectedCount = count ? count : uint32_t(-1);
				count = 0;

				auto getElements = [&]()
					{
						std::string elementsStr;
						if (expectedCount > 100 && size > 100)
							expectedCount = 100;

						for (uint32_t n = start, i = 0; n < (uint32_t)size && i < expectedCount; n++, count++)
						{
							auto elemAddr = arr->At(n);
							auto _name = std::format("[{}]", n);

							auto _count = (uint32_t)0;
							auto _size = 0;
							elementsStr += ToJsonString(_name.c_str(), elemAddr, elemTypeId, 0, _count, _size, engine, asTM_NONE);
							elementsStr += ",";
						}

						if (!elementsStr.empty())
							elementsStr.pop_back();

						return elementsStr;
					};

				if (name)
				{
					std::string typeName = std::format("array<{}>", elemTypeName) + ptr;
					std::string elementsStr = "[" + getElements() + "]";
					return std::format(variableArr, Modify(typeName, typeMod), typeId, size, (int64_t)value, name, "size = " + std::to_string(size), 0, count, elementsStr);
				}
				else
					return getElements();
			});
	}

	if (auto t = engine->GetTypeInfoByName("dictionary"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string ptr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					ptr = "@";
					_value = *(void**)value; // It is safe.
				}

				auto dic = (CScriptDictionary*)_value;
				size = (int)dic->GetSize();
				auto expectedCount = count ? count : uint32_t(-1);
				count = 0;

				auto getElements = [&]()
					{
						std::string elementsStr;
						if (expectedCount > 100 && size > 100)
							expectedCount = 100;

						uint32_t i = 0;
						for (auto iter = dic->opForBegin(); !dic->opForEnd(*iter) && i < expectedCount; count++)
						{
							auto& dic_value = dic->opForValue0(*iter);
							auto& dic_key = dic->opForValue1(*iter);
							auto elemTypeId = dic_value.GetTypeId();
							auto elemAddr = const_cast<void*>(dic_value.GetAddressOfValue());

							auto _count = (uint32_t)0;
							auto _size = 0;
							elementsStr += ToJsonString(dic_key.c_str(), elemAddr, elemTypeId, 0, _count, _size, engine, asTM_NONE);
							elementsStr += ",";

							iter = dic->opForNext(*iter);
						}

						if (!elementsStr.empty())
							elementsStr.pop_back();

						return elementsStr;
					};

				if (name)
				{
					std::string typeName = "dictionary" + ptr;
					std::string elementsStr = "[" + getElements() + "]";
					return std::format(variableArr, Modify(typeName, typeMod), typeId, size, (int64_t)value, name, "size = " + std::to_string(size), 0, count, elementsStr);
				}
				else
					return getElements();
			});
	}

	if (auto t = engine->GetTypeInfoByName("dictionaryValue"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				auto dic_value = (CScriptDictValue*)_value;
				if (name)
				{
					auto elemTypeId = dic_value->GetTypeId();
					auto elemAddr = const_cast<void*>(dic_value->GetAddressOfValue());

					auto valueStr = getVariableValue(elemAddr, elemTypeId, engine);
					size = getElementsSize(elemAddr, elemTypeId, engine);

					auto value_type = std::format("dictionaryValue<{}>", getTypeName(elemTypeId, engine));
					return std::format(variable, Modify(value_type + handlePtr, typeMod), typeId, size, (int64_t)value, name, StringSymbalEscape(valueStr));
				}
				else
				{
					auto elemTypeId = dic_value->GetTypeId();
					auto elemAddr = const_cast<void*>(dic_value->GetAddressOfValue());

					auto _count = (uint32_t)0;
					return ToJsonString(nullptr, elemAddr, elemTypeId, 0, _count, size, engine, asTM_NONE);
				}
			});
	}

	if (auto t = engine->GetTypeInfoByName("dictionaryIter"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				auto iter = (CScriptDictionary::CScriptDictIter*)_value;
				auto dic = iter->dict;

				if (!dic->opForEnd(*iter))
				{
					if (name)
					{
						auto& dic_value = dic->opForValue0(*iter);
						auto& dic_key = dic->opForValue1(*iter);
						auto elemTypeId = dic_value.GetTypeId();
						auto elemAddr = const_cast<void*>(dic_value.GetAddressOfValue());

						auto valueStr = getVariableValue(elemAddr, elemTypeId, engine);
						size = getElementsSize(elemAddr, elemTypeId, engine);
						auto key_value = std::format("{} : {}", dic_key.c_str(), valueStr);

						auto iter_type = std::format("dictionaryIter<{}>", getTypeName(elemTypeId, engine));
						return std::format(variable, Modify(iter_type + handlePtr, typeMod), typeId, size, (int64_t)value, name, StringSymbalEscape(key_value));
					}
					else
					{
						auto& dic_value = dic->opForValue0(*iter);
						auto& dic_key = dic->opForValue1(*iter);
						auto elemTypeId = dic_value.GetTypeId();
						auto elemAddr = const_cast<void*>(dic_value.GetAddressOfValue());

						auto _count = (uint32_t)0;
						return ToJsonString(nullptr, elemAddr, elemTypeId, 0, _count, size, engine, asTM_NONE);
					}
				}
				else
					return std::format(variable, Modify("dictionaryIter" + handlePtr, typeMod), typeId, 0, (int64_t)value, name ? name : "null", "end");
			});
	}

	if (auto t = engine->GetTypeInfoByName("grid"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string ptr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					ptr = "@";
					_value = *(void**)value; // It is safe.
				}

				auto grid = (CScriptGrid*)_value;
				auto elemTypeId = grid->GetElementTypeId();
				auto elemTypeName = getTypeName(elemTypeId, engine);

				auto width = grid->GetWidth();
				auto height = grid->GetHeight();
				size = int(width * height);
				auto expectedCount = count ? count : uint32_t(-1);
				count = 0;

				auto x_start = width ? (start % width) : 0;
				auto y_start = width ? (start / width) : 0;

				auto getElements = [&]()
					{
						std::string elementsStr;
						if (expectedCount > 100 && size > 100)
							expectedCount = 100;

						uint32_t i = 0;
						for (uint32_t y = y_start; y < height; y++)
						{
							for (uint32_t x = x_start; x < width; x++)
							{
								if (i < expectedCount)
								{
									count++;

									auto elemAddr = grid->At(x, y);
									auto _name = std::format("[{}][{}]", y, x);

									auto _count = (uint32_t)0;
									auto _size = 0;
									elementsStr += ToJsonString(_name.c_str(), elemAddr, elemTypeId, 0, _count, _size, engine, asTM_NONE);
									elementsStr += ",";
								}
								else
									goto jump;
							}
						}
					jump:
						if (!elementsStr.empty())
							elementsStr.pop_back();

						return elementsStr;
					};

				if (name)
				{
					std::string typeName = std::format("grid<{}>", elemTypeName) + ptr;
					std::string elementsStr = "[" + getElements() + "]";
					auto width_height = std::format("width = {}, height = {}", width, height);
					return std::format(variableArr, Modify(typeName, typeMod), typeId, size, (int64_t)value, name, width_height, 0, count, elementsStr);
				}
				else
					return getElements();
			});
	}

	if (auto t = engine->GetTypeInfoByName("datetime"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				size = 0;
				auto ptr = (CDateTime*)_value;

				std::string valueStr = std::format("{}-{}-{} {}:{}:{}", ptr->getYear(), ptr->getMonth(), ptr->getDay(), ptr->getHour(), ptr->getMinute(), ptr->getSecond());
				return std::format(variable, Modify("datetime" + handlePtr, typeMod), typeId, size, (int64_t)value, name, valueStr);
			});
	}

	if (auto t = engine->GetTypeInfoByName("ref"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				auto ptr = (CScriptHandle*)_value;
				auto childTypeId = ptr->GetTypeId();

				if (auto t = ptr->GetType())
				{
					auto typyName = std::format("ref<{}>", t ? t->GetName() : "void") + handlePtr;

					auto childAddr = ptr->GetRef();

					if (name)
					{
						auto valueStr = getVariableValue(childAddr, childTypeId, engine);
						size = getElementsSize(childAddr, childTypeId, engine);
						return std::format(variable, Modify(typyName, typeMod), typeId, size, (int64_t)value, name, StringSymbalEscape(valueStr));
					}
					else
					{
						auto _count = (uint32_t)0;
						return ToJsonString(nullptr, childAddr, childTypeId, 0, _count, size, engine, asTM_NONE);
					}
				}

				return std::format(variable, Modify("ref" + handlePtr, typeMod), typeId, size = 0, (int64_t)value, name ? name : "null", "null");
			});
	}

	if (auto t = engine->GetTypeInfoByName("weakref"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				auto ptr = (CScriptWeakRef*)_value;

				if (auto t = ptr->GetRefType())
				{
					auto childTypeId = t->GetTypeId();

					auto typyName = std::format("weakref<{}>", t ? t->GetName() : "void") + handlePtr;

					if (auto childAddr = ptr->Get())
					{
						engine->ReleaseScriptObject(childAddr, t);

						if (name)
						{
							auto valueStr = getVariableValue(childAddr, childTypeId, engine);
							size = getElementsSize(childAddr, childTypeId, engine);
							return std::format(variable, Modify(typyName, typeMod), typeId, size, (int64_t)value, name, StringSymbalEscape(valueStr));
						}
						else
						{
							auto _count = (uint32_t)0;
							return ToJsonString(nullptr, childAddr, childTypeId, 0, _count, size, engine, asTM_NONE);
						}
					}
					else
						return std::format(variable, Modify(typyName, typeMod), typeId, 0, (int64_t)value, name ? name : "null", "null");
				}

				return std::format(variable, Modify("weakref" + handlePtr, typeMod), typeId, size = 0, (int64_t)value, name ? name : "null", "null");
			});
	}

	if (auto t = engine->GetTypeInfoByName("const_weakref"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				auto ptr = (CScriptWeakRef*)_value;
				auto t = ptr->GetRefType();
				auto childTypeId = t->GetTypeId();

				auto typyName = std::format("const_weakref<{}>", t ? t->GetName() : "void") + handlePtr;

				if (auto childAddr = ptr->Get())
				{
					engine->ReleaseScriptObject(childAddr, t);

					if (name)
					{
						auto valueStr = getVariableValue(childAddr, childTypeId, engine);
						size = getElementsSize(childAddr, childTypeId, engine);
						return std::format(variable, Modify(typyName, typeMod), typeId, size, (int64_t)value, name, StringSymbalEscape(valueStr));
					}
					else
					{
						auto _count = (uint32_t)0;
						return ToJsonString(nullptr, childAddr, childTypeId, 0, _count, size, engine, asTM_NONE);
					}
				}
				else
					return std::format(variable, Modify(typyName, typeMod), typeId, 0, (int64_t)value, name ? name : "null", "null");
			});
	}

	if (auto t = engine->GetTypeInfoByName("any"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				auto ptr = (CScriptAny*)_value;
				auto childTypeId = ptr->GetTypeId();

				auto typyName = std::format("any<{}>", getTypeName(childTypeId, engine)) + handlePtr;
				void* elemAddr = nullptr;
				ptr->Retrieve(&elemAddr, childTypeId);

				if (name)
				{
					auto valueStr = getVariableValue(elemAddr, childTypeId, engine);
					size = getElementsSize(elemAddr, childTypeId, engine);
					return std::format(variable, Modify(typyName, typeMod), typeId, size, (int64_t)value, name, StringSymbalEscape(valueStr));
				}
				else
				{
					auto _count = (uint32_t)0;
					return ToJsonString(nullptr, elemAddr, childTypeId, 0, _count, size, engine, asTM_NONE);
				}
			});
	}

	if (auto t = engine->GetTypeInfoByName("socket"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				size = 3;
				auto ptr = (CScriptSocket*)_value;

				if (name)
				{
					std::string valueStr = std::format("m_socket = {}, m_isListening = {}, m_refCount = {}", ptr->m_socket, ptr->m_isListening, ptr->m_refCount);
					return std::format(variable, Modify("socket" + handlePtr, typeMod), typeId, size, (int64_t)value, name, valueStr);
				}
				else
				{
					auto str = std::format(variable, "int", 0, 0, (int64_t)&ptr->m_socket, "m_socket", ptr->m_socket);
					str += ",";
					str += std::format(variable, "bool", 0, 0, (int64_t)&ptr->m_isListening, "m_isListening", ptr->m_isListening);
					str += ",";
					str += std::format(variable, "int", 0, 0, (int64_t)&ptr->m_refCount, "m_refCount", ptr->m_refCount);
					return str;
				}
			});
	}

	if (auto t = engine->GetTypeInfoByName("complex"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				size = 2;
				auto ptr = (Complex*)_value;

				if (name)
				{
					std::string valueStr = std::format("r = {}, i = {}", ptr->r, ptr->i);
					return std::format(variable, Modify("complex" + handlePtr, typeMod), typeId, size, (int64_t)value, name, valueStr);
				}
				else
				{
					auto str = ToJsonString("r", &ptr->r, asTYPEID_FLOAT, 0, count, size, engine, asTM_NONE);
					str += ",";
					str += ToJsonString("i", &ptr->i, asTYPEID_FLOAT, 0, count, size, engine, asTM_NONE);
					return str;
				}
			});
	}

	if (auto t = engine->GetTypeInfoByName("file"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				size = 3;
				auto ptr = (CScriptFile*)_value;

				if (name)
				{
					std::string valueStr = std::format("mostSignificantByteFirst = {}, refCount = {}, file = {}", ptr->mostSignificantByteFirst, ptr->refCount, (void*)ptr->file);
					return std::format(variable, Modify("file" + handlePtr, typeMod), typeId, size, (int64_t)value, name, valueStr);
				}
				else
				{
					auto str = std::format(variable, "bool", 0, 0, (int64_t)&ptr->mostSignificantByteFirst, "mostSignificantByteFirst", ptr->mostSignificantByteFirst);
					str += ",";
					str += std::format(variable, "int", 0, 0, (int64_t)&ptr->refCount, "refCount", ptr->refCount);
					str += ",";
					str += std::format(variable, "FILE*", 0, 0, (int64_t)&ptr->file, "m_refCount", (void*)ptr->file);
					return str;
				}
			});
	}

	if (auto t = engine->GetTypeInfoByName("filesystem"))
	{
		SetToStringCallback(t, [](const char* name, void* value, int typeId, uint32_t start, uint32_t& count, int& size, asIScriptEngine* engine, asETypeModifiers typeMod)->std::string
			{
				auto _value = value;
				std::string handlePtr;
				if (typeId & asTYPEID_OBJHANDLE)
				{
					handlePtr = "@";
					_value = *(void**)value; // It is safe.
				}

				size = 2;
				auto ptr = (CScriptFileSystem*)_value;

				if (name)
				{
					std::string valueStr = std::format("refCount = {}, file = {}", ptr->refCount, ptr->currentPath);
					return std::format(variable, Modify("filesystem" + handlePtr, typeMod), typeId, size, (int64_t)value, name, valueStr);
				}
				else
				{
					auto str = std::format(variable, "int", 0, 0, (int64_t)&ptr->refCount, "refCount", ptr->refCount);
					str += ",";
					str += std::format(variable, "std::string", 0, 0, (int64_t)&ptr->currentPath, "currentPath", StringSymbalEscape(ptr->currentPath));
					return str;
				}

			});
	}
}

void MessageCallback(const asSMessageInfo* msg, void* param)
{
	const char* type = "ERR ";
	if (msg->type == asMSGTYPE_WARNING)
		type = "WARN";
	else if (msg->type == asMSGTYPE_INFORMATION)
		type = "INFO";

	auto str = std::format("{} {}({},{}) : {}", type, msg->section, msg->row, msg->col, msg->message);

	std::cout << str << std::endl;
}

void as_output(const std::string& str)
{
	std::cout << str << std::endl;
}

int main()
{
	static volatile bool bTerminated = false;
	auto thread = std::thread([] { RunDebuggerServer(bTerminated, 9000); });

	std::cout << "Waiting to be connected by the Visual Studio" << std::endl;
	while (!gDbgSvr.mReady)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	std::cout << "Exit waiting mode, the program officially begins" << std::endl;

	auto engine = asCreateScriptEngine();
	CreateScriptEngineCallback(engine);
	engine->SetEngineProperty(asEP_ALLOW_UNICODE_IDENTIFIERS, true);
	engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, true);
	engine->SetEngineProperty(asEP_NO_DEBUG_OUTPUT, true);
	engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
	engine->SetEngineProperty(asEP_ALLOW_MULTILINE_STRINGS, true);
	engine->SetEngineProperty(asEP_BOOL_CONVERSION_MODE, true);

	RegisterStdString(engine);
	RegisterScriptArray(engine, false);
	RegisterStdStringUtils(engine);
	RegisterScriptDictionary(engine);
	RegisterScriptDateTime(engine);
	RegisterScriptFile(engine);
	RegisterScriptFileSystem(engine);
	RegisterScriptMath(engine);
	RegisterScriptMathComplex(engine);
	RegisterExceptionRoutines(engine);
	RegisterScriptHandle(engine);
	RegisterScriptGrid(engine);
	RegisterScriptWeakRef(engine);
	RegisterScriptAny(engine);
	RegisterScriptSocket(engine);

	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, 0);
	engine->RegisterGlobalFunction("void as_output(const string & in)", asFUNCTION(as_output), asCALL_CDECL);
	RegisterDbgToString(engine);

	auto ctx = engine->CreateContext();
	PrepareExecutionCallback(ctx);

	std::string module_file = "AngelScriptProject/Script/demo.cc";
	std::ifstream ifs(module_file);
	if (!ifs.is_open())
	{
		std::cout << "File opening failed" << std::endl;
		return -1;
	}
	std::ostringstream oss;
	oss << ifs.rdbuf();
	std::string codes = oss.str();

	for (auto pos = codes.find("functyp"); pos != std::string::npos; pos = codes.find("functyp", pos))
	{
		codes[pos] = ' ';

		while (pos > 0 && codes[pos] != '\n')
			pos--;

		if (codes[pos] == '\n')
			pos++;

		codes[pos++] = '/';
		codes[pos++] = '/';
	}

	for (auto pos = codes.find("assume"); pos != std::string::npos; pos = codes.find("assume", pos))
	{
		codes[pos] = ' ';

		while (pos > 0 && codes[pos] != '\n')
			pos--;

		if (codes[pos] == '\n')
			pos++;

		codes[pos++] = '/';
		codes[pos++] = '/';
	}

	for (auto pos = codes.find("$"); pos != std::string::npos; pos = codes.find("$", pos))
	{
		codes[pos++] = '@';
	}

	auto module = engine->GetModule(module_file.c_str(), asGM_ALWAYS_CREATE);
	module->AddScriptSection(module_file.c_str(), codes.c_str(), codes.size());
	if (module->Build() < 0)
	{
		std::cout << "Build failed" << std::endl;
		return -2;
	}
	module->SetUserData(&module_file);

	auto func = module->GetFunctionByName("main");

	ctx->Prepare(func);
	ctx->Execute();

	std::cout << "Demo finished" << std::endl;
	bTerminated = true;
	thread.join();
	ctx->Release();
	engine->Release();
	return 0;
}

std::pair<std::string, size_t> GetFileLineByModuleLine(asIScriptModule* module, size_t inLine)
{
	auto str = (std::string*)module->GetUserData();

	auto pos = str->find("Script/");
	if (pos != std::string::npos)
		return std::pair(str->substr(pos), inLine);

	return std::pair(*str, inLine);
}
