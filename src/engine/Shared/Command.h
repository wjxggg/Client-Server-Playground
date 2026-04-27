#pragma once

#include <span>
#include <string>
#include <vector>

#include "ErrorHandle.h"
#include "Log.h"
#include "Utils/StringUtils.h"

struct ConVar
{
	const char *const name;
	const char *const desc;

	union
	{
		int32_t int32Default;
		uint32_t uint32Default;
		int64_t int64Default;
		uint64_t uint64Default;
		float floatDefault;
		const char *const stringDefault;
	};

	union
	{
		int32_t int32Min;
		uint32_t uint32Min;
		int64_t int64Min;
		uint64_t uint64Min;
		float floatMin;
	};

	union
	{
		int32_t int32Max;
		uint32_t uint32Max;
		int64_t int64Max;
		uint64_t uint64Max;
		float floatMax;
	};
};

class ICommand
{
public:
	virtual void Run(std::span<std::string> argsStr) const = 0;
};

template<typename... Args>
class Command : public ICommand
{
public:
	Command() = delete;
	constexpr Command(const char *const _name, const char *const _desc, void (*_func)(Args ...args), std::initializer_list<ConVar> _conVars) :
		m_name{_name},
		m_desc{_desc},
		m_func{_func},
		m_conVars{_conVars}
	{

	}

	void Run(std::span<std::string> argsStr) const override
	{
		if (argsStr.size() != m_conVars.size())
		{
			BE::Log::Warn("Argument count does not match (args: {})", m_conVars.size());
			return;
		}

		auto sequence = std::index_sequence_for<Args...>{};

		ExtractAndRun(sequence, argsStr);
	}

	const char *const GetName() const { return m_name; }
	const char *const GetDesc() const { return m_desc; }

private:
	const char *const m_name;
	const char *const m_desc;
	void (*m_func)(Args ...args);
	std::vector<ConVar> m_conVars;

	template<size_t... I>
	void ExtractAndRun(std::index_sequence<I...>, std::span<std::string> argsStr) const
	{
		// try-catch to avoid calling m_func when Convert() failed
		try
		{
			// span<string> to Args...
			m_func(Convert<typename std::decay<Args>::type>(argsStr[I], m_conVars[I])...);
		}
		catch (...)
		{

		}
	}

	template<typename T>
	static T Convert(const std::string &argStr, const ConVar &conVar)
	{
		BE::Log::Critical("Undefined ConVar type");
		ErrorHandle::PauseThenAbort();
	}

	template<>
	int32_t Convert<int32_t>(const std::string &argStr, const ConVar &conVar)
	{
		if (argStr == "default") return conVar.int32Default;
		else if (argStr == "min") return conVar.int32Min;
		else if (argStr == "max") return conVar.int32Max;

		int32_t value;
		if (!BE::Utils::FromString(argStr, &value))
		{
			BE::Log::Warn("Argument \"{}: {}\" is invalid (type: int32)", conVar.name, argStr);
			throw "";
		}
		if (value < conVar.int32Min || value > conVar.int32Max)
		{
			BE::Log::Warn("Argument \"{}: {}\" is out of bounds [{}, {}]", conVar.name, value, conVar.int32Min, conVar.int32Max);
			throw "";
		}

		return value;
	}

	template<>
	uint32_t Convert<uint32_t>(const std::string &argStr, const ConVar &conVar)
	{
		if (argStr == "default") return conVar.uint32Default;
		else if (argStr == "min") return conVar.uint32Min;
		else if (argStr == "max") return conVar.uint32Max;

		uint32_t value;
		if (!BE::Utils::FromString(argStr, &value))
		{
			BE::Log::Warn("Argument \"{}: {}\" is invalid (type: uint32)", conVar.name, argStr);
			throw "";
		}
		if (value < conVar.uint32Min || value > conVar.uint32Max)
		{
			BE::Log::Warn("Argument \"{}: {}\" is out of bounds [{}, {}]", conVar.name, value, conVar.uint32Min, conVar.uint32Max);
			throw "";
		}

		return value;
	}

	template<>
	int64_t Convert<int64_t>(const std::string &argStr, const ConVar &conVar)
	{
		if (argStr == "default") return conVar.int64Default;
		else if (argStr == "min") return conVar.int64Min;
		else if (argStr == "max") return conVar.int64Max;

		int64_t value;
		if (!BE::Utils::FromString(argStr, &value))
		{
			BE::Log::Warn("Argument \"{}: {}\" is invalid (type: int64)", conVar.name, argStr);
			throw "";
		}
		if (value < conVar.int64Min || value > conVar.int64Max)
		{
			BE::Log::Warn("Argument \"{}: {}\" is out of bounds [{}, {}]", conVar.name, value, conVar.int64Min, conVar.int64Max);
			throw "";
		}

		return value;
	}

	template<>
	uint64_t Convert<uint64_t>(const std::string &argStr, const ConVar &conVar)
	{
		if (argStr == "default") return conVar.uint64Default;
		else if (argStr == "min") return conVar.uint64Min;
		else if (argStr == "max") return conVar.uint64Max;

		uint64_t value;
		if (!BE::Utils::FromString(argStr, &value))
		{
			BE::Log::Warn("Argument \"{}: {}\" is invalid (type: uint64)", conVar.name, argStr);
			throw "";
		}
		if (value < conVar.uint64Min || value > conVar.uint64Max)
		{
			BE::Log::Warn("Argument \"{}: {}\" is out of bounds [{}, {}]", conVar.name, value, conVar.uint64Min, conVar.uint64Max);
			throw "";
		}

		return value;
	}

	template<>
	float Convert<float>(const std::string &argStr, const ConVar &conVar)
	{
		if (argStr == "default") return conVar.floatDefault;
		else if (argStr == "min") return conVar.floatMin;
		else if (argStr == "max") return conVar.floatMax;

		float value;
		if (!BE::Utils::FromString(argStr, &value))
		{
			BE::Log::Warn("Argument \"{}: {}\" is invalid (type: float)", conVar.name, argStr);
			throw "";
		}
		if (value < conVar.floatMin || value > conVar.floatMax)
		{
			BE::Log::Warn("Argument \"{}: {}\" is out of bounds [{}, {}]", conVar.name, value, conVar.floatMin, conVar.floatMax);
			throw "";
		}

		return value;
	}

	template<>
	std::string Convert<std::string>(const std::string &argStr, const ConVar &conVar)
	{
		if (argStr == "default") return std::string{conVar.stringDefault};
		return argStr;
	}
};
