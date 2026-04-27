#pragma once

#include <format>
#include <string>
#include <utility>

#include "EngineConfig.h"

BE_BEGIN_NAMESPACE

namespace Log
{

enum Source : uint32_t
{
	CLIENT,
	SERVER,
	MODULE,
	USER,
	VULKAN,
	LOCAL
};

enum Level : uint32_t
{
	TRACE,
	DEBUG,
	INFO,
	WARN,
	ERROR,
	CRITICAL
};

BE_API void Dump(const std::string &file);

BE_API void Trace(const std::string &msg);
BE_API void Trace(Source source, const std::string &msg);
BE_API void Debug(const std::string &msg);
BE_API void Debug(Source source, const std::string &msg);
BE_API void Info(const std::string &msg);
BE_API void Info(Source source, const std::string &msg);
BE_API void Warn(const std::string &msg);
BE_API void Warn(Source source, const std::string &msg);
BE_API void Error(const std::string &msg);
BE_API void Error(Source source, const std::string &msg);
BE_API void Critical(const std::string &msg);
BE_API void Critical(Source source, const std::string &msg);

template <typename... Args>
void Trace(std::format_string<Args...> fmt, Args &&...args)
{
	Trace(std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Trace(Source source, std::format_string<Args...> fmt, Args &&...args)
{
	Trace(source, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Debug(std::format_string<Args...> fmt, Args &&...args)
{
	Debug(std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Debug(Source source, std::format_string<Args...> fmt, Args &&...args)
{
	Debug(source, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Info(std::format_string<Args...> fmt, Args &&...args)
{
	Info(std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Info(Source source, std::format_string<Args...> fmt, Args &&...args)
{
	Info(source, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Warn(std::format_string<Args...> fmt, Args &&...args)
{
	Warn(std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Warn(Source source, std::format_string<Args...> fmt, Args &&...args)
{
	Warn(source, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Error(std::format_string<Args...> fmt, Args &&...args)
{
	Error(std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Error(Source source, std::format_string<Args...> fmt, Args &&...args)
{
	Error(source, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Critical(std::format_string<Args...> fmt, Args &&...args)
{
	Critical(std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void Critical(Source source, std::format_string<Args...> fmt, Args &&...args)
{
	Critical(source, std::format(fmt, std::forward<Args>(args)...));
}

}

BE_END_NAMESPACE
