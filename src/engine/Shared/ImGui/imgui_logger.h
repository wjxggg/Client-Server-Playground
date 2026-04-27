#pragma once

#include <string>
#include <vector>
#include <deque>

#include "imgui.h"
#include "imgui_internal.h"

class ImGuiLogger
{
public:
	enum class SelectionMode
	{
		Normal,
		Word,
		Line
	};

	// Represents a character coordinate from the user's point of view,
	// i. e. consider an uniform grid (assuming fixed-width font) on the
	// screen as it is rendered, and each cell has its own coordinate, starting from 0.
	// Tabs are counted as [1..mTabSize] count empty spaces, depending on
	// how many space is necessary to reach the next tab stop.
	// For example, coordinate (1, 5) represents the character 'B' in a line "\tABC", when mTabSize = 4,
	// because it is rendered as "    ABC" on the screen.
	struct Coordinates
	{
		int line, column;
		Coordinates() : line(0), column(0) {}
		Coordinates(int line, int column) : line(line), column(column)
		{
			assert(line >= 0);
			assert(column >= 0);
		}

		bool operator ==(const Coordinates &coord) const
		{
			return
				line == coord.line &&
				column == coord.column;
		}

		bool operator !=(const Coordinates &coord) const
		{
			return
				line != coord.line ||
				column != coord.column;
		}

		bool operator <(const Coordinates &coord) const
		{
			if (line != coord.line)
				return line < coord.line;
			return column < coord.column;
		}

		bool operator >(const Coordinates &coord) const
		{
			if (line != coord.line)
				return line > coord.line;
			return column > coord.column;
		}

		bool operator <=(const Coordinates &coord) const
		{
			if (line != coord.line)
				return line < coord.line;
			return column <= coord.column;
		}

		bool operator >=(const Coordinates &coord) const
		{
			if (line != coord.line)
				return line > coord.line;
			return column >= coord.column;
		}
	};

	typedef uint8_t Char;

	struct ColorTextSegment
	{
		uint32_t end;
		ImU32 color;
	};

	struct Line
	{
		uint32_t level;
		uint32_t source;
		std::string text;
		std::vector<ColorTextSegment> colorTextSegment;
		// for multi line message
		uint32_t offset;
		bool filtered;

		Line(uint32_t _level, uint32_t _source, std::string _text, ImU32 _color, uint32_t _offset) :
			level{_level},
			source{_source},
			text{_text},
			offset{_offset},
			filtered{false}
		{
			colorTextSegment.emplace_back(ColorTextSegment{0, 0xFFFFFFFF});
			colorTextSegment.emplace_back(ColorTextSegment{static_cast<uint32_t>(text.size()), _color});
		}

		inline int size() const
		{
			return static_cast<int>(text.size());
		}

		void AddColor(ImU32 color)
		{
			colorTextSegment.emplace_back(ColorTextSegment{static_cast<uint32_t>(text.size()), color});
		}
	};

	ImGuiLogger();
	~ImGuiLogger();

	void Render(ImVec2 size);
	void Copy() const;

	void RefreshFilteredLines();

	void SetLevelFilter(uint32_t levelFlags) { m_levelFilter = levelFlags; RefreshFilteredLines(); }
	void SetSourceFilter(uint32_t sourceFlags) { m_sourceFilter = sourceFlags; RefreshFilteredLines(); }

	void SetMaxLineCount(uint32_t count) { m_maxLineCount = count; ClampLineCount(); }
	void SetMaxFilteredLineCount(uint32_t count) { m_maxFilteredLineCount = count; RefreshFilteredLines(); }

	ImGuiTextFilter &GetTextFilter() { return m_textFilter; }
	std::string GetSelectedText() const;

	inline void SetHandleUserInputs(bool value){ m_handleUserInputs = value;}
	inline bool IsHandleUserInputsEnabled() const { return m_handleUserInputs; }

	// Add text at new line
	void AddLine(std::string text, uint32_t level, uint32_t source, ImU32 color = 0xFFFFFFFF);
	// Add text at current line
	void AddText(std::string text, ImU32 color = 0xFFFFFFFF);

	void MoveUp(bool select = false);
	void MoveDown(bool select = false);
	void MoveLeft(bool select = false, bool wordMode = false);
	void MoveRight(bool select = false, bool wordMode = false);

	void SetSelectionStart(const Coordinates &position);
	void SetSelectionEnd(const Coordinates &position);
	void SetSelection(const Coordinates &start, const Coordinates &end, SelectionMode mode = SelectionMode::Normal);
	void SelectAll();
	bool HasSelection() const;

private:
	struct LoggerState
	{
		Coordinates m_selectionStart;
		Coordinates m_selectionEnd;
		Coordinates m_cursorPosition;
	};

	bool PassFiltering(int lineIndex);

	void AddFilteredLine(int lineIndex, bool newLine);

	float TextDistanceToLineStart(const Coordinates &from) const;
	void EnsureCursorVisible();
	std::string GetText(const Coordinates &start, const Coordinates &end) const;
	Coordinates ScreenPosToCoordinates(const ImVec2 &position) const;
	Coordinates FindWordStart(const Coordinates &from) const;
	Coordinates FindWordEnd(const Coordinates &from) const;
	int GetCharacterIndex(const Coordinates &coord) const;
	int GetCharacterColumn(int lineIndex, int index) const;
	int GetLineMaxColumn(int lineIndex) const;
	bool IsOnWordBoundary(const Coordinates &at) const;

	void MoveCoordsInternal(Coordinates &coordOut, int lines, bool forward = true);

	void MoveCursor(int lines, bool forward = true);
	void MoveSelection(int lines, bool forward = true);
	void MoveInteractives(int lines, bool forward = true);

	void HandleKeyboardInputs(bool hoveredScrollbar, bool activeScrollbar);
	void HandleMouseInputs(bool hoveredScrollbar, bool activeScrollbar);

	void ClampLineCount();

	ImGuiWindow *m_pWindow;

	bool m_shouldScrollToCursor;

	bool m_scrolledToBottom;

	int m_scrollBackAmount;
	float m_lastFrameScrollY;

	bool m_handleUserInputs;
	bool m_withinLoggingRect;

	SelectionMode m_selectionMode;
	double m_lastClickTime;
	double m_cursorBlinkerStartTime;

	Coordinates m_interactiveStart;
	Coordinates	m_interactiveEnd;

	LoggerState m_state;
	ImVec2 m_charAdvance;

	uint32_t m_maxLineCount;
	uint32_t m_maxFilteredLineCount;
	std::deque<Line> m_lines;
	std::deque<Line *> m_filteredLines;

	ImGuiTextFilter m_textFilter;
	uint32_t m_levelFilter;
	uint32_t m_sourceFilter;
};
