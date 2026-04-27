#pragma once

#include <mutex>

#include <spdlog/sinks/base_sink.h>

namespace Console
{

void RenderFrame();
void ToggleConsole();

};

class ConsoleSink : public spdlog::sinks::base_sink<std::mutex>
{
protected:
	void sink_it_(const spdlog::details::log_msg &msg) override;
	void flush_() override;
};
