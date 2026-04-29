#include "Console.h"

#include <array>
#include <format>
#include <string>

#include "ImGui/imgui_logger.h"

#include "Commands.h"
#include "Log.h"
#include "Utils/StringUtils.h"
#include "Utils.h"

BE_USING_NAMESPACE

//-----------------------------------------------------------------------------
// Const
//-----------------------------------------------------------------------------

static consteval ImU32 RGBAToU32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    ImU32 color = static_cast<ImU32>(
        (r << IM_COL32_R_SHIFT) |
        (g << IM_COL32_G_SHIFT) |
        (b << IM_COL32_B_SHIFT) |
        (a << IM_COL32_A_SHIFT));
    return color;
}

constexpr std::array<const char *const, 3> MESSAGE_SOURCE_STRING{"client", "server", "user"};
constexpr std::array<ImU32, 3> MESSAGE_SOURCE_COLOR{
    RGBAToU32(253, 253, 150, 255),	// client (pastel yellow)
    RGBAToU32(32, 224, 208, 255),	// server (bright turquoise)
    RGBAToU32(108, 232, 108, 255)	// user (pastel green)
};

constexpr std::array<const char *const, 6> MESSAGE_LEVEL_STRING{"trace", "debug", "info", "warn", "error", "critical"};
constexpr std::array<ImU32, 6> MESSAGE_LEVEL_COLOR{
    RGBAToU32(180, 180, 180, 255), // trace (white)
    RGBAToU32(40, 200, 200, 255),  // debug (cyan)
    RGBAToU32(40, 200, 40, 255),   // info (green)
    RGBAToU32(200, 200, 40, 255),  // warning (yellow)
    RGBAToU32(200, 40, 40, 255),   // error (red)
    RGBAToU32(255, 0, 0, 255)      // critical (very red)
};

constexpr size_t CONSOLE_COMMAND_SIZE = 512;

//-----------------------------------------------------------------------------
// Variable
//-----------------------------------------------------------------------------

static bool m_isOpen{true};
static uint32_t m_messageSourceFilter{Util_ToFlagBit(Log::CLIENT) | Util_ToFlagBit(Log::SERVER) | Util_ToFlagBit(Log::USER)};
static uint32_t m_messageLevelFilter{Util_ToFlagBit(Log::LEVEL_TRACE) | Util_ToFlagBit(Log::LEVEL_DEBUG) | Util_ToFlagBit(Log::LEVEL_INFO) | Util_ToFlagBit(Log::LEVEL_WARN) | Util_ToFlagBit(Log::LEVEL_ERROR) | Util_ToFlagBit(Log::LEVEL_CRITICAL)};

static ImGuiLogger m_textLogger;

static char m_inputTextBuf[CONSOLE_COMMAND_SIZE];

//-----------------------------------------------------------------------------
// Private Decl
//-----------------------------------------------------------------------------

static uint32_t GetSourceFromLoggerName(const char *const loggerName);
static void DisplayOptions();
static void DisplayCommandLine();

//-----------------------------------------------------------------------------
// Public
//-----------------------------------------------------------------------------

void Console::RenderFrame()
{
    if (!m_isOpen) return;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, RGBAToU32(5, 5, 5, 255));

    ImGui::Begin(
        "Console",
        &m_isOpen,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar
    );

    ImGui::SetWindowSize(ImVec2(720, 480), ImGuiCond_Once);
    ImGui::SetWindowPos(ImVec2(50, 50), ImGuiCond_Once);

    DisplayOptions();

    ImVec2 loggerRegion = ImGui::GetContentRegionAvail();
    loggerRegion.y -= ImGui::CalcTextSize("#").y;
    m_textLogger.Render(loggerRegion);

    DisplayCommandLine();

    ImGui::End();

    ImGui::PopStyleColor();
}

void Console::ToggleConsole()
{
    m_isOpen = !m_isOpen;
}

//-----------------------------------------------------------------------------
// Protected
//-----------------------------------------------------------------------------

void ConsoleSink::sink_it_(const spdlog::details::log_msg &msg)
{
    m_textLogger.AddLine(std::format("[{:%T}]", floor<std::chrono::seconds>(msg.time)), msg.level, GetSourceFromLoggerName(msg.logger_name.data()), RGBAToU32(180, 180, 180, 255));
    m_textLogger.AddText(('[' + std::string{msg.logger_name} + ']'), MESSAGE_SOURCE_COLOR[GetSourceFromLoggerName(msg.logger_name.data())]);
    m_textLogger.AddText(": ", RGBAToU32(180, 180, 180, 255));
    m_textLogger.AddText(std::string{msg.payload}, MESSAGE_LEVEL_COLOR[msg.level]);
}

void ConsoleSink::flush_()
{

}

//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------

uint32_t GetSourceFromLoggerName(const char *const loggerName)
{
    switch (loggerName[0])
    {
        case 'c':
            return Log::CLIENT;
        case 's':
            return Log::SERVER;
        case 'u':
            return Log::USER;
    }

	std::unreachable();
}

void DisplayOptions()
{
    const ImVec2 fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr);
    const float windowWidth = ImGui::GetWindowWidth();

    if (m_textLogger.GetTextFilter().Draw("##Console_TextFilter", std::max(fontSize.x, windowWidth - 240)))
    {
        m_textLogger.RefreshFilteredLines();
    }

    ImGui::SameLine();

    // Message source filter
    bool sourceFilterChanged = false;
    if (ImGui::BeginCombo("##Console_SourceFilter", "Source", ImGuiComboFlags_WidthFitPreview))
    {
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(MESSAGE_SOURCE_STRING.size()));
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                if (ImGui::Selectable(MESSAGE_SOURCE_STRING[i], static_cast<bool>(m_messageSourceFilter & Util_ToFlagBit(i)), ImGuiSelectableFlags_DontClosePopups))
                {
                    m_messageSourceFilter ^= Util_ToFlagBit(i);
                    sourceFilterChanged = true;
                }
            }
        }
        ImGui::EndCombo();
    }

    if (sourceFilterChanged)
    {
        m_textLogger.SetSourceFilter(m_messageSourceFilter);
    }

    ImGui::SameLine();

    // Message level filter
    bool levelFilterChanged = false;
    if (ImGui::BeginCombo("##Console_LevelFilter", "Level", ImGuiComboFlags_WidthFitPreview))
    {
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(MESSAGE_LEVEL_STRING.size()));
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                if (ImGui::Selectable(MESSAGE_LEVEL_STRING[i], static_cast<bool>(m_messageLevelFilter & Util_ToFlagBit(i)), ImGuiSelectableFlags_DontClosePopups))
                {
                    m_messageLevelFilter ^= Util_ToFlagBit(i);
                    levelFilterChanged = true;
                }
            }
        }
        ImGui::EndCombo();
    }

    if (levelFilterChanged)
    {
        m_textLogger.SetLevelFilter(m_messageLevelFilter);
    }
}

void DisplayCommandLine()
{
    constexpr int inputTextFieldFlags{
        ImGuiInputTextFlags_EnterReturnsTrue
    };

    if (ImGui::InputText("##Console_CommandLine", m_inputTextBuf, CONSOLE_COMMAND_SIZE, inputTextFieldFlags))
    {
        Log::Info(Log::USER, m_inputTextBuf);

        // cmd, arg1, arg2, arg3...
        std::vector<std::string> argsStr;
        Utils::SplitString(m_inputTextBuf, &argsStr);

        m_inputTextBuf[0] = '\0';

        if (argsStr.empty()) return;

        Commands::Run(argsStr);
    }
}
