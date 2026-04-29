#include "Log.h"
#include "Log.h"

#include <array>

#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Console.h"

#if defined(BE_HOST_CLIENT)
	#define LOCAL_HOST Log::CLIENT
#elif defined(BE_HOST_SERVER)
	#define LOCAL_HOST Log::SERVER
#endif

BE_USING_NAMESPACE

static std::array<spdlog::logger, 3> m_loggers{
	spdlog::logger{"client"},
	spdlog::logger{"server"},
	spdlog::logger{"user"}
};

void Log::Init()
{
	auto stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

	for (int i = 0; i < m_loggers.size(); ++i)
	{
		m_loggers[i].set_level(spdlog::level::trace);
		m_loggers[i].sinks().emplace_back(stdoutSink);
	}

	auto consoleSink = std::make_shared<ConsoleSink>();
	for (int i = 0; i < m_loggers.size(); ++i)
	{
		m_loggers[i].sinks().emplace_back(consoleSink);
	}
}

void Log::Dump(const std::string &file)
{
	// TODO
}

void Log::Trace(const std::string &msg)
{
	m_loggers[LOCAL_HOST].trace(msg);
}

void Log::Trace(Log::Source source, const std::string &msg)
{
	if (source >= Log::LOCAL)
	{
		source = LOCAL_HOST;
	}

	m_loggers[source].trace(msg);
}

void Log::Debug(const std::string &msg)
{
	m_loggers[LOCAL_HOST].debug(msg);
}

void Log::Debug(Log::Source source, const std::string &msg)
{
	if (source >= Log::LOCAL)
	{
		source = LOCAL_HOST;
	}

	m_loggers[source].debug(msg);
}

void Log::Info(const std::string &msg)
{
	m_loggers[LOCAL_HOST].info(msg);
}

void Log::Info(Log::Source source, const std::string &msg)
{
	if (source >= Log::LOCAL)
	{
		source = LOCAL_HOST;
	}

	m_loggers[source].info(msg);
}

void Log::Warn(const std::string &msg)
{
	m_loggers[LOCAL_HOST].warn(msg);
}

void Log::Warn(Log::Source source, const std::string &msg)
{
	if (source >= Log::LOCAL)
	{
		source = LOCAL_HOST;
	}

	m_loggers[source].warn(msg);
}

void Log::Error(const std::string &msg)
{
	m_loggers[LOCAL_HOST].error(msg);
}

void Log::Error(Log::Source source, const std::string &msg)
{
	if (source >= Log::LOCAL)
	{
		source = LOCAL_HOST;
	}

	m_loggers[source].error(msg);
}

void Log::Critical(const std::string &msg)
{
	m_loggers[LOCAL_HOST].critical(msg);
}

void Log::Critical(Log::Source source, const std::string &msg)
{
	if (source >= Log::LOCAL)
	{
		source = LOCAL_HOST;
	}

	m_loggers[source].critical(msg);
}
