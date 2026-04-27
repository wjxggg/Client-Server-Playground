#include "imgui_logger.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>

constexpr int TAB_SIZE = 4;

static inline bool IsSpace(char c)
{
	return ((1 << (c - 1)) & 0x80001F00 & ~(-int((c - 1) & 0xE0))) != 0;
}

static inline int UTF8CharLength(char c)
{
	if ((c & 0xF8) == 0xF0)
		return 4;// 4-byte sequence (U+10000 to U+10FFFF)
	else if ((c & 0xF0) == 0xE0)
		return 3;// 3-byte sequence (U+0800 to U+FFFF)
	else if ((c & 0xE0) == 0xC0)
		return 2;// 2-byte sequence (U+0080 to U+07FF)
	return 1;// 1-byte (ASCII)
}

static inline bool IsUTFSequence(char c)
{
	return (c & 0xC0) == 0x80;
}

ImGuiLogger::ImGuiLogger() :
	m_pWindow{nullptr},
	m_shouldScrollToCursor{false},
	m_scrolledToBottom{false},
	m_scrollBackAmount{0},
	m_lastFrameScrollY{0.0f},
	m_handleUserInputs{true},
	m_withinLoggingRect{false},
	m_selectionMode{SelectionMode::Normal},
	m_lastClickTime{-1.0},
	m_cursorBlinkerStartTime{-1.0},
	m_maxLineCount{8192},
	m_maxFilteredLineCount{2048},
	m_levelFilter{UINT32_MAX},
	m_sourceFilter{UINT32_MAX}
{
}

ImGuiLogger::~ImGuiLogger()
{

}

std::string ImGuiLogger::GetText(const Coordinates &start, const Coordinates &end) const
{
	std::string result;

	int lstart = start.line;
	int lend = end.line;
	int istart = GetCharacterIndex(start);
	int iend = GetCharacterIndex(end);
	size_t s = 0;

	for (int i = lstart; i < lend; ++i)
		s += (*m_filteredLines[i]).size();

	result.reserve(s + s / 8);

	while (istart < iend || lstart < lend)
	{
		if (lstart >= static_cast<int>(m_filteredLines.size()))
			break;

		const Line &line = *m_filteredLines[lstart];
		if (istart < line.size())
		{
			result += line.text[istart];
			++istart;
		}
		else
		{
			result += '\n';
			istart = 0;
			++lstart;
		}
	}

	return result;
}

ImGuiLogger::Coordinates ImGuiLogger::ScreenPosToCoordinates(const ImVec2 &position) const
{
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 local{position.x - origin.x, position.y - origin.y};

	int lineNo = ImMax(0, static_cast<int>(ImFloor(local.y / m_charAdvance.y)));
	int columnCoord = 0;

	if (lineNo >= 0 && lineNo < static_cast<int>(m_filteredLines.size()))
	{
		const Line &line = *m_filteredLines[lineNo];

		int columnIndex = 0;
		float columnX = 0.0f;

		while (columnIndex < line.size())
		{
			float columnWidth = 0.0f;

			if (line.text[columnIndex] == '\t')
			{
				float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
				float oldX = columnX;
				float newColumnX = (1.0f + ImFloor((1.0f + columnX) / (TAB_SIZE * spaceSize))) * (TAB_SIZE * spaceSize);
				columnWidth = newColumnX - oldX;

				if (columnX + columnWidth * 0.5f > local.x)
					break;

				columnX = newColumnX;
				columnCoord = (columnCoord / TAB_SIZE) * TAB_SIZE + TAB_SIZE;
				++columnIndex;
			}
			else
			{
				char buf[7];
				int d = UTF8CharLength(line.text[columnIndex]);
				int i = 0;

				while (i < 6 && d > 0 && columnIndex < line.size())
				{
					buf[i++] = line.text[columnIndex];
					++columnIndex;
					--d;
				}

				buf[i] = '\0';
				columnWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, &buf[i]).x;

				if (columnX + columnWidth * 0.5f > local.x)
					break;

				columnX += columnWidth;
				++columnCoord;
			}
		}
	}

	return Coordinates(lineNo, columnCoord);
}

ImGuiLogger::Coordinates ImGuiLogger::FindWordStart(const Coordinates &from) const
{
	Coordinates at = from;
	if (at.line >= static_cast<int>(m_filteredLines.size()))
		return at;

	const Line &line = *m_filteredLines[at.line];
	int cindex = GetCharacterIndex(at);

	if (cindex >= line.size())
		return at;

	while (cindex > 0 && IsSpace(line.text[cindex]))
		--cindex;

	while (cindex > 0)
	{
		Char c = line.text[cindex];
		if ((c & 0xC0) != 0x80)	// not UTF code sequence 10xxxxxx
		{
			if (c <= 32 && IsSpace(c))
			{
				++cindex;
				break;
			}
		}
		--cindex;
	}
	return Coordinates(at.line, GetCharacterColumn(at.line, cindex));
}

ImGuiLogger::Coordinates ImGuiLogger::FindWordEnd(const Coordinates &from) const
{
	Coordinates at = from;
	if (at.line >= static_cast<int>(m_filteredLines.size()))
		return at;

	const Line &line = *m_filteredLines[at.line];
	int cindex = GetCharacterIndex(at);

	if (cindex >= line.size())
		return at;

	bool prevspace = IsSpace(line.text[cindex]);
	while (cindex < line.size())
	{
		Char c = line.text[cindex];
		int d = UTF8CharLength(c);

		if (prevspace != IsSpace(c))
		{
			if (IsSpace(c))
				while (cindex < line.size() && !IsSpace(line.text[cindex]))
					++cindex;
			break;
		}
		cindex += d;
	}
	return Coordinates(from.line, GetCharacterColumn(from.line, cindex));
}

int ImGuiLogger::GetCharacterIndex(const Coordinates &coord) const
{
	if (coord.line >= static_cast<int>(m_filteredLines.size()))
		return -1;

	const Line &line = *m_filteredLines[coord.line];
	int c = 0;
	int i = 0;

	for (; i < line.size() && c < coord.column;)
	{
		if (line.text[i] == '\t')
			c = (c / TAB_SIZE) * TAB_SIZE + TAB_SIZE;
		else
			++c;
		i += UTF8CharLength(line.text[i]);
	}
	return i;
}

int ImGuiLogger::GetCharacterColumn(int lineIndex, int index) const
{
	if (lineIndex >= static_cast<int>(m_filteredLines.size()))
		return 0;

	const Line &line = *m_filteredLines[lineIndex];
	int col = 0;
	int i = 0;

	while (i < index && i < line.size())
	{
		Char c = line.text[i];
		i += UTF8CharLength(c);

		if (c == '\t')
			col = (col / TAB_SIZE) * TAB_SIZE + TAB_SIZE;
		else
			++col;
	}
	return col;
}

int ImGuiLogger::GetLineMaxColumn(int lineIndex) const
{
	if (lineIndex >= static_cast<int>(m_filteredLines.size()))
		return 0;

	const Line &line = *m_filteredLines[lineIndex];
	int col = 0;

	for (int i = 0; i < line.size(); )
	{
		Char c = line.text[i];
		if (c == '\t')
			col = (col / TAB_SIZE) * TAB_SIZE + TAB_SIZE;
		else
			++col;
		i += UTF8CharLength(c);
	}
	return col;
}

bool ImGuiLogger::IsOnWordBoundary(const Coordinates &at) const
{
	if (at.line >= static_cast<int>(m_filteredLines.size()) || at.column == 0)
		return true;

	const Line &line = *m_filteredLines[at.line];
	int cindex = GetCharacterIndex(at);

	if (cindex >= line.size())
		return true;

	return IsSpace(line.text[cindex]) != IsSpace(line.text[cindex - 1]);
}

void ImGuiLogger::HandleKeyboardInputs(bool hoveredScrollbar, bool activeScrollbar)
{
	ImGuiIO &io = ImGui::GetIO();
	bool shift = io.KeyShift;
	bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (!activeScrollbar && ImGui::IsWindowFocused())
	{
		if (!hoveredScrollbar && ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			MoveUp(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			MoveDown(shift);
		else if (!alt && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
			MoveLeft(shift, ctrl);
		else if (!alt && ImGui::IsKeyPressed(ImGuiKey_RightArrow))
			MoveRight(shift, ctrl);
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_C))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGuiKey_A))
			SelectAll();
	}
}

void ImGuiLogger::HandleMouseInputs(bool hoveredScrollbar, bool activeScrollbar)
{
	ImGuiIO &io = ImGui::GetIO();
	bool shift = io.KeyShift;
	bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
		m_withinLoggingRect = true;
	else if (ImGui::IsMouseReleased(0))
		m_withinLoggingRect = false;

	if (!hoveredScrollbar && !activeScrollbar && m_withinLoggingRect)
	{
		bool click = ImGui::IsMouseClicked(0);

		if (!shift && !alt)
		{
			bool doubleClick = ImGui::IsMouseDoubleClicked(0);

			double t = ImGui::GetTime();
			bool tripleClick = click && !doubleClick && (m_lastClickTime != -1.0 && (t - m_lastClickTime) < io.MouseDoubleClickTime);

			/*
			Left mouse button triple click
			*/

			if (tripleClick)
			{
				if (!ctrl)
				{
					m_state.m_cursorPosition = m_interactiveStart = m_interactiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					m_selectionMode = SelectionMode::Line;
					SetSelection(m_interactiveStart, m_interactiveEnd, m_selectionMode);
				}

				m_lastClickTime = -1.0;
			}

			/*
			Left mouse button double click
			*/

			else if (doubleClick)
			{
				if (!ctrl)
				{
					m_state.m_cursorPosition = m_interactiveStart = m_interactiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());

					// Advance cursor to the end of the selection
					m_interactiveStart = FindWordStart(m_state.m_cursorPosition);
					m_state.m_cursorPosition = m_interactiveEnd = FindWordEnd(m_state.m_cursorPosition);

					if (m_selectionMode == SelectionMode::Line)
						m_selectionMode = SelectionMode::Normal;
					else
						m_selectionMode = SelectionMode::Word;
					SetSelection(m_interactiveStart, m_interactiveEnd, m_selectionMode);
				}

				m_lastClickTime = ImGui::GetTime();
			}

			/*
			Left mouse button click
			*/
			else if (click)
			{
				m_state.m_cursorPosition = m_interactiveStart = m_interactiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
				if (ctrl)
					m_selectionMode = SelectionMode::Word;
				else
					m_selectionMode = SelectionMode::Normal;
				SetSelection(m_interactiveStart, m_interactiveEnd, m_selectionMode);

				m_lastClickTime = ImGui::GetTime();
			}
			// Mouse left button dragging (=> update selection)
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
			{
				m_selectionMode = SelectionMode::Normal;
				io.WantCaptureMouse = true;

				m_state.m_cursorPosition = m_interactiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());

				SetSelection(m_interactiveStart, m_interactiveEnd, m_selectionMode);
				EnsureCursorVisible();
			}
		}
		else if (shift)
		{
			if (click) // Shift select range
			{
				const Coordinates newCursorPos = ScreenPosToCoordinates(ImGui::GetMousePos());

				// Set selection from old cursor pos to new cursor pos
				m_selectionMode = SelectionMode::Normal;
				SetSelection(m_state.m_cursorPosition, newCursorPos, m_selectionMode);

				m_interactiveStart = m_state.m_selectionStart;
				m_interactiveEnd = m_state.m_selectionEnd;
			}
		}
	}
}

void ImGuiLogger::Render(ImVec2 size)
{
	// Compute m_charAdvance regarding to scaled font size (Ctrl + mouse wheel)
	const ImVec2 fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr);
	m_charAdvance = ImVec2(fontSize.x, ImGui::GetTextLineHeightWithSpacing());

	// Scroll back if needed
	// Must before BeginChild()
	if (!m_scrolledToBottom && m_scrollBackAmount > 0 && m_pWindow != nullptr)
	{
		ImGui::SetScrollY(m_pWindow, m_lastFrameScrollY - m_scrollBackAmount * m_charAdvance.y);
	}
	m_scrollBackAmount = 0;

	ImGui::BeginChild(
		"##ImGuiLogger",
		size,
		ImGuiChildFlags_None,
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_HorizontalScrollbar |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoNavInputs
	);
	m_pWindow = ImGui::GetCurrentWindow();
	m_lastFrameScrollY = ImGui::GetScrollY();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	const ImGuiStyle &style = ImGui::GetStyle();

	const ImGuiID activeID = ImGui::GetActiveID();
	const ImGuiID hoveredID = ImGui::GetHoveredID();

	const bool hoveredScrollbar = hoveredID && (hoveredID == ImGui::GetWindowScrollbarID(m_pWindow, ImGuiAxis_X) || hoveredID == ImGui::GetWindowScrollbarID(m_pWindow, ImGuiAxis_Y));
	const bool activeScrollbar = activeID && (activeID == ImGui::GetWindowScrollbarID(m_pWindow, ImGuiAxis_X) || activeID == ImGui::GetWindowScrollbarID(m_pWindow, ImGuiAxis_Y));

	if (m_handleUserInputs)
	{
		HandleKeyboardInputs(hoveredScrollbar, activeScrollbar);
		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, false);

		HandleMouseInputs(hoveredScrollbar, activeScrollbar);
	}

	const ImVec2 contentSize = ImGui::GetWindowContentRegionMax();
	ImDrawList *const drawList = ImGui::GetWindowDrawList();

	const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

	float longest = 0.0f;
	float scrollY = ImGui::GetScrollY();

	int lineNo = static_cast<int>(ImFloor(scrollY / m_charAdvance.y));
	const int lineMax = ImMin(static_cast<int>(m_filteredLines.size()), lineNo + static_cast<int>(ImCeil((scrollY + contentSize.y) / m_charAdvance.y)));

	while (lineNo < lineMax)
	{
		const Line &line = *m_filteredLines[lineNo];

		const ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * m_charAdvance.y);

		longest = ImMax(TextDistanceToLineStart(Coordinates(lineNo, GetLineMaxColumn(lineNo))), longest);

		Coordinates lineStartCoord(lineNo, 0);
		Coordinates lineEndCoord(lineNo, GetLineMaxColumn(lineNo));

		// Draw selection for the current line
		float sstart = -1.0f;
		float ssend = -1.0f;

		assert(m_state.m_selectionStart <= m_state.m_selectionEnd);
		if (m_state.m_selectionStart <= lineEndCoord)
			sstart = m_state.m_selectionStart > lineStartCoord ? TextDistanceToLineStart(m_state.m_selectionStart) : 0.0f;
		if (m_state.m_selectionEnd > lineStartCoord)
			ssend = TextDistanceToLineStart(m_state.m_selectionEnd < lineEndCoord ? m_state.m_selectionEnd : lineEndCoord);

		if (m_state.m_selectionEnd.line > lineNo)
			ssend += m_charAdvance.x;

		if (sstart != -1 && ssend != -1 && sstart < ssend)
		{
			const ImVec2 vstart(lineStartScreenPos.x + sstart, lineStartScreenPos.y);
			const ImVec2 vend(lineStartScreenPos.x + ssend, lineStartScreenPos.y + m_charAdvance.y);

			drawList->AddRectFilled(vstart, vend, ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
		}

		// Render the cursor
		if (m_state.m_cursorPosition.line == lineNo && ImGui::IsWindowFocused())
		{
			// Initialize the cursor start render time, this is done here
			// as Dear ImGui typically isn't initialized during the
			// construction of this class
			if (m_cursorBlinkerStartTime == -1.0)
				m_cursorBlinkerStartTime = ImGui::GetTime();

			const double currTime = ImGui::GetTime();
			const double elapsed = currTime - m_cursorBlinkerStartTime;

			if (elapsed > 0.4)
			{
				const float width = 1.0f;
				const float cx = TextDistanceToLineStart(m_state.m_cursorPosition);

				const ImVec2 cstart(lineStartScreenPos.x + cx, lineStartScreenPos.y);
				const ImVec2 cend(lineStartScreenPos.x + cx + width, lineStartScreenPos.y + m_charAdvance.y);

				drawList->AddRectFilled(cstart, cend, ImGui::ColorConvertFloat4ToU32(ImVec4(0.87f, 0.87f, 0.87f, 1.0f)));

				if (elapsed > 0.8)
					m_cursorBlinkerStartTime = currTime;
			}
		}

		// Render segments with their colors
		for (int i = 1; i < line.colorTextSegment.size(); ++i)
		{
			// Handle different char length
			const int lastSegment = line.colorTextSegment[i - 1].end;
			const int curSegment = line.colorTextSegment[i].end;

			const float cx = TextDistanceToLineStart(Coordinates{lineNo, GetCharacterColumn(lineNo, lastSegment)});
			const ImVec2 offset{lineStartScreenPos.x + cx, lineStartScreenPos.y};
			drawList->AddText(offset, line.colorTextSegment[i].color, &line.text[lastSegment], &line.text[curSegment]);
		}

		++lineNo;
	}

	// This dummy is here to let Dear ImGui know where the last character of
	// the line had ended, so that it could properly process the horizontal
	// scrollbar
	const float additional = m_pWindow->ScrollbarY ? style.ScrollbarSize : 0.0f;
	ImGui::Dummy(ImVec2(longest + additional, m_filteredLines.size() * m_charAdvance.y));

	m_scrolledToBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

	if (m_scrolledToBottom && !m_shouldScrollToCursor)
	{
		ImGui::SetScrollHereY(1.0f);
	}

	m_shouldScrollToCursor = false;

	if (m_handleUserInputs)
		ImGui::PopItemFlag();

	ImGui::PopStyleVar();

	ImGui::EndChild();
}

void ImGuiLogger::Copy() const
{
	if (HasSelection())
	{
		ImGui::SetClipboardText(GetSelectedText().c_str());
	}
}

void ImGuiLogger::SetSelectionStart(const Coordinates &position)
{
	m_state.m_selectionStart = position;
	if (m_state.m_selectionStart > m_state.m_selectionEnd)
		ImSwap(m_state.m_selectionStart, m_state.m_selectionEnd);
}

void ImGuiLogger::SetSelectionEnd(const Coordinates &position)
{
	m_state.m_selectionEnd = position;
	if (m_state.m_selectionStart > m_state.m_selectionEnd)
		ImSwap(m_state.m_selectionStart, m_state.m_selectionEnd);
}

void ImGuiLogger::SetSelection(const Coordinates &start, const Coordinates &end, SelectionMode mode)
{
	m_state.m_selectionStart = start;
	m_state.m_selectionEnd = end;

	if (m_state.m_selectionStart > m_state.m_selectionEnd)
		ImSwap(m_state.m_selectionStart, m_state.m_selectionEnd);

	switch (mode)
	{
		case ImGuiLogger::SelectionMode::Normal:
			break;
		case ImGuiLogger::SelectionMode::Word:
		{
			m_state.m_selectionStart = FindWordStart(m_state.m_selectionStart);
			if (!IsOnWordBoundary(m_state.m_selectionEnd))
				m_state.m_selectionEnd = FindWordEnd(FindWordStart(m_state.m_selectionEnd));
			break;
		}
		case ImGuiLogger::SelectionMode::Line:
		{
			const int lineNo = m_state.m_selectionEnd.line;
			m_state.m_selectionStart = Coordinates(m_state.m_selectionStart.line, 0);
			m_state.m_selectionEnd = m_filteredLines.size() > lineNo + 1 ? Coordinates(lineNo + 1, 0) : Coordinates(lineNo, GetLineMaxColumn(lineNo));
			m_state.m_cursorPosition = m_state.m_selectionEnd;
			break;
		}
		default:
			break;
	}
}

void ImGuiLogger::MoveUp(bool select)
{
	const Coordinates oldPos = m_state.m_cursorPosition;
	m_state.m_cursorPosition.line = ImMax(0, m_state.m_cursorPosition.line - 1);
	if (oldPos != m_state.m_cursorPosition)
	{
		if (select)
		{
			if (oldPos == m_interactiveStart)
				m_interactiveStart = m_state.m_cursorPosition;
			else if (oldPos == m_interactiveEnd)
				m_interactiveEnd = m_state.m_cursorPosition;
			else
			{
				m_interactiveStart = m_state.m_cursorPosition;
				m_interactiveEnd = oldPos;
			}
		}
		else
			m_interactiveStart = m_interactiveEnd = m_state.m_cursorPosition;

		SetSelection(m_interactiveStart, m_interactiveEnd, SelectionMode::Normal);
		EnsureCursorVisible();
	}
}

void ImGuiLogger::MoveDown(bool select)
{
	assert(m_state.m_cursorPosition.column >= 0);
	Coordinates oldPos = m_state.m_cursorPosition;

	m_state.m_cursorPosition.line = ImMax(0, ImMin(static_cast<int>(m_filteredLines.size() - 1), m_state.m_cursorPosition.line + 1));

	if (m_state.m_cursorPosition != oldPos)
	{
		if (select)
		{
			if (oldPos == m_interactiveEnd)
				m_interactiveEnd = m_state.m_cursorPosition;
			else if (oldPos == m_interactiveStart)
				m_interactiveStart = m_state.m_cursorPosition;
			else
			{
				m_interactiveStart = oldPos;
				m_interactiveEnd = m_state.m_cursorPosition;
			}
		}
		else
			m_interactiveStart = m_interactiveEnd = m_state.m_cursorPosition;

		SetSelection(m_interactiveStart, m_interactiveEnd, SelectionMode::Normal);
		EnsureCursorVisible();
	}
}

void ImGuiLogger::MoveLeft(bool select, bool wordMode)
{
	const Coordinates oldPos = m_state.m_cursorPosition;
	m_state.m_cursorPosition = m_state.m_cursorPosition;

	int lineIndex = m_state.m_cursorPosition.line;
	int cindex = GetCharacterIndex(m_state.m_cursorPosition);

	if (cindex == 0)
	{
		if (lineIndex > 0)
		{
			--lineIndex;
			if (m_filteredLines.size() > lineIndex)
				cindex = (*m_filteredLines[lineIndex]).size();
			else
				cindex = 0;
		}
	}
	else
	{
		--cindex;
		if (cindex > 0)
		{
			if (m_filteredLines.size() > lineIndex)
			{
				const Line &line = *m_filteredLines[lineIndex];

				while (cindex > 0 && IsUTFSequence(line.text[cindex]))
					--cindex;

				// Skip the newline character.
				if (cindex > 0 && line.text[cindex] == '\n')
					--cindex;
			}
		}
	}

	m_state.m_cursorPosition = Coordinates(lineIndex, GetCharacterColumn(lineIndex, cindex));
	if (wordMode)
	{
		m_state.m_cursorPosition = FindWordStart(m_state.m_cursorPosition);
	}

	if (select)
	{
		if (oldPos == m_interactiveStart)
			m_interactiveStart = m_state.m_cursorPosition;
		else if (oldPos == m_interactiveEnd)
			m_interactiveEnd = m_state.m_cursorPosition;
		else
		{
			m_interactiveStart = m_state.m_cursorPosition;
			m_interactiveEnd = oldPos;
		}
	}
	else
	{
		if (HasSelection() && !wordMode)
			m_state.m_cursorPosition = m_interactiveStart;

		m_interactiveStart = m_interactiveEnd = m_state.m_cursorPosition;
	}

	SetSelection(m_interactiveStart, m_interactiveEnd, select && wordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void ImGuiLogger::MoveRight(bool select, bool wordMode)
{
	const Coordinates oldPos = m_state.m_cursorPosition;

	if (m_filteredLines.empty() || oldPos.line >= static_cast<int>(m_filteredLines.size()))
		return;

	int cindex = GetCharacterIndex(m_state.m_cursorPosition);

	int lindex = m_state.m_cursorPosition.line;
	const Line &line = *m_filteredLines[lindex];

	bool isNewLine = false;
	const bool isLastChar = (cindex >= line.size() - 1);

	// If the cursor is at the last character before the newline character,
	// we want to skip the newline character and move to the next line.
	if (isLastChar && !line.text.empty())
		isNewLine = line.text.back() == '\n';

	if (cindex >= line.size() || isNewLine)
	{
		if (m_state.m_cursorPosition.line < static_cast<int>(m_filteredLines.size()) - 1)
		{
			m_state.m_cursorPosition.line = ImMax(0, ImMin(static_cast<int>(m_filteredLines.size()) - 1, m_state.m_cursorPosition.line + 1));
			m_state.m_cursorPosition.column = 0;
		}
		else
			return;
	}
	else
	{
		cindex += UTF8CharLength(line.text[cindex]);
		m_state.m_cursorPosition = Coordinates(lindex, GetCharacterColumn(lindex, cindex));
		if (wordMode)
			m_state.m_cursorPosition = FindWordEnd(m_state.m_cursorPosition);
	}

	if (select)
	{
		if (oldPos == m_interactiveEnd)
			m_interactiveEnd = m_state.m_cursorPosition;
		else if (oldPos == m_interactiveStart)
			m_interactiveStart = m_state.m_cursorPosition;
		else
		{
			m_interactiveStart = oldPos;
			m_interactiveEnd = m_state.m_cursorPosition;
		}
	}
	else
	{
		if (HasSelection() && !wordMode)
			m_state.m_cursorPosition = m_interactiveEnd;

		m_interactiveStart = m_interactiveEnd = m_state.m_cursorPosition;
	}
	SetSelection(m_interactiveStart, m_interactiveEnd, select && wordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void ImGuiLogger::SelectAll()
{
	SetSelection(Coordinates(0, 0), Coordinates(static_cast<int>(m_filteredLines.size()), 0));
}

bool ImGuiLogger::HasSelection() const
{
	return m_state.m_selectionEnd > m_state.m_selectionStart;
}

void ImGuiLogger::MoveCoordsInternal(Coordinates &coordOut, int lines, bool forward)
{
	if (lines < 1)
		return;

	if (forward)
	{
		coordOut.line += lines;

		if (coordOut.line >= static_cast<int>(m_filteredLines.size()))
		{
			coordOut.line = static_cast<int>(m_filteredLines.size()) - 1;
			coordOut.column = GetLineMaxColumn(coordOut.line);
		}
	}
	else
	{
		coordOut.line -= lines;

		if (coordOut.line < 0)
		{
			coordOut.line = 0;
			coordOut.column = 0;
		}
	}
}

// If you click on line 100, and the first line gets removed from the vector,
// the line you clicked will now be 99. This function corrects the selection
// again after the adjustments to the vector
void ImGuiLogger::MoveSelection(int lines, bool forward)
{
	// This always has to be called as interactives are the start/end pos of a
	// mouse click, which gets cached off.
	MoveInteractives(lines, forward);

	if (HasSelection())
	{
		Coordinates newCoords[2];

		newCoords[0] = m_state.m_selectionStart;
		newCoords[1] = m_state.m_selectionEnd;

		for (size_t i = 0; i < IM_ARRAYSIZE(newCoords); i++)
		{
			MoveCoordsInternal(newCoords[i], lines, forward);
		}

		SetSelectionStart(newCoords[0]);
		SetSelectionEnd(newCoords[1]);
	}
}

void ImGuiLogger::MoveInteractives(int lines, bool forward)
{
	Coordinates newCoords[2];

	newCoords[0] = m_interactiveStart;
	newCoords[1] = m_interactiveEnd;

	for (size_t i = 0; i < IM_ARRAYSIZE(newCoords); i++)
	{
		MoveCoordsInternal(newCoords[i], lines, forward);
	}

	m_interactiveStart = newCoords[0];
	m_interactiveEnd = newCoords[1];
}

void ImGuiLogger::MoveCursor(int lines, bool forward)
{
	MoveCoordsInternal(m_state.m_cursorPosition, lines, forward);
}

std::string ImGuiLogger::GetSelectedText() const
{
	return GetText(m_state.m_selectionStart, m_state.m_selectionEnd);
}

static std::vector<std::string> SplitNewLines(std::string text)
{
	if (text.empty()) return std::vector<std::string>{""};

	std::vector<std::string> splitText;
	size_t splitStart = 0;
	size_t cindex = 0;
	while ((cindex = text.find('\n', cindex)) != std::string::npos)
	{
		splitText.emplace_back(text.substr(splitStart, cindex - splitStart)); // not include '\n'
		splitStart = cindex + 1;
		++cindex;
	}

	if (splitStart < text.size()) splitText.emplace_back(text.substr(splitStart));

	return splitText;
}

void ImGuiLogger::AddLine(std::string text, uint32_t level, uint32_t source, ImU32 color)
{
	std::vector<std::string> splitText = SplitNewLines(text);

	for (uint32_t i = 0; i < splitText.size(); ++i)
	{
		m_lines.emplace_back(Line{level, source, splitText[i], color, i});
		if (PassFiltering(static_cast<int>(m_lines.size() - 1)))
		{
			AddFilteredLine(static_cast<int>(m_lines.size() - 1), true);
		}
	}
	ClampLineCount();
}

void ImGuiLogger::AddText(std::string text, ImU32 color)
{
	std::vector<std::string> splitText = SplitNewLines(text);
	uint32_t rootLine = static_cast<uint32_t>(m_lines.size() - 1);

	m_lines[rootLine].text.append(splitText[0]);
	m_lines[rootLine].AddColor(color);

	for (uint32_t i = 1; i < splitText.size(); ++i)
	{
		m_lines.emplace_back(Line{m_lines[rootLine].level, m_lines[rootLine].source, splitText[i], color, i});
		if (PassFiltering(static_cast<int>(m_lines.size() - 1)))
		{
			AddFilteredLine(static_cast<int>(m_lines.size() - 1), true);
		}
	}
	ClampLineCount();
}

bool ImGuiLogger::PassFiltering(int lineIndex)
{
	// Need to make sure check rootLine first

	Line &line = m_lines[lineIndex];

	// If line is childLine, check if the rootLine pass filtering
	int rootLine = lineIndex - line.offset;
	if (line.offset != 0) return rootLine >= 0 && m_lines[rootLine].filtered;

	bool passTextFilter = false;
	if (m_textFilter.IsActive())
	{
		// If line is rootLine, check all the child lines.
		// So we can save all the lines in the same message
		// if any child line pass filtering.
		int i = lineIndex;
		while (i < m_lines.size() &&  i - m_lines[i].offset == lineIndex)
		{
			// Check child line
			if (m_textFilter.PassFilter(m_lines[i].text.c_str(), &m_lines[i].text[m_lines[i].text.size()]))
			{
				passTextFilter = true;
				break;
			}

			++i;
		}
	}
	else
	{
		passTextFilter = true;
	}

	line.filtered = ((1 << line.level) & m_levelFilter) && ((1 << line.source) & m_sourceFilter) && passTextFilter;

	return line.filtered;
}

void ImGuiLogger::AddFilteredLine(int lineIndex, bool newLine)
{
	if (newLine)
	{
		m_filteredLines.push_back(&m_lines[lineIndex]);
	}
	else
	{
		m_filteredLines.push_front(&m_lines[lineIndex]);
	}

	if (m_filteredLines.size() > m_maxFilteredLineCount)
	{
		m_filteredLines.pop_front();
		MoveSelection(1, false);
		MoveCursor(1, false);

		++m_scrollBackAmount;
	}
}

void ImGuiLogger::RefreshFilteredLines()
{
	m_filteredLines.clear();

	// Add lines form new to old
	int lineIndex = static_cast<int>(m_lines.size() - 1);
	while (lineIndex >= 0 && m_filteredLines.size() < m_maxFilteredLineCount)
	{
		if (m_lines[lineIndex].offset != 0)
		{
			int rootLine = lineIndex - m_lines[lineIndex].offset;

			bool passFiltering = rootLine >= 0 && PassFiltering(rootLine);
			if (passFiltering)
			{
				for (int i = lineIndex; i >= lineIndex - static_cast<int>(m_lines[lineIndex].offset); --i)
				{
					AddFilteredLine(i, false);
				}
			}

			lineIndex -= m_lines[lineIndex].offset + 1;
		}
		else
		{
			if (PassFiltering(lineIndex))
			{
				AddFilteredLine(lineIndex, false);
			}
			--lineIndex;
		}
	}
}

float ImGuiLogger::TextDistanceToLineStart(const Coordinates& from) const
{
	const Line &line = *m_filteredLines[from.line];

	float distance = 0.0f;
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;
	int colIndex = GetCharacterIndex(from);

	for (size_t it = 0; it < line.size() && it < colIndex; )
	{
		if (line.text[it] == '\t')
		{
			distance = (1.0f + ImFloor((1.0f + distance) / (TAB_SIZE * spaceSize))) * (TAB_SIZE * spaceSize);
			++it;
		}
		else
		{
			size_t d = UTF8CharLength(line.text[it]);
			size_t i = 0;
			char tempCString[7];

			for (; i < 6 && d-- > 0 && it < line.size(); ++i, ++it)
				tempCString[i] = line.text[it];

			tempCString[i] = '\0';
			distance += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, &tempCString[i], nullptr).x;
		}
	}

	return distance;
}

void ImGuiLogger::EnsureCursorVisible()
{
	m_shouldScrollToCursor = true;
	const Coordinates pos = m_state.m_cursorPosition;

	const float scrollX = ImGui::GetScrollX();
	const float scrollY = ImGui::GetScrollY();

	const float width = ImGui::GetWindowWidth();
	const float height = ImGui::GetWindowHeight();

	const float top = ImCeil(scrollY / m_charAdvance.y);
	const float bottom = ImCeil((scrollY + height) / m_charAdvance.y);

	const float left = ImCeil(scrollX / m_charAdvance.x);
	const float right = ImCeil((scrollX + width) / m_charAdvance.x);

	const ImGuiWindow *const window = ImGui::GetCurrentWindow();

	// For right offset, +.1f as otherwise it would render right below the
	// first pixel making up the right perimeter.
	const float rightOffsetAmount = window->ScrollbarY ? 3.1f : 1.1f;
	const float bottomOffsetAmount = window->ScrollbarX ? 3.f : 2.f;

	if (pos.column < left)
		ImGui::SetScrollX(ImMax(0.0f, (pos.column) * m_charAdvance.x));
	if (pos.column > right - rightOffsetAmount)
		ImGui::SetScrollX(ImMax(0.0f, (pos.column + rightOffsetAmount - 1.f) * m_charAdvance.x - width));
	if (pos.line < top)
		ImGui::SetScrollY(ImMax(0.0f, (pos.line) * m_charAdvance.y));
	if (pos.line > bottom - bottomOffsetAmount)
		ImGui::SetScrollY(ImMax(0.0f, (pos.line + bottomOffsetAmount - 1.f) * m_charAdvance.y - height));
}

void ImGuiLogger::ClampLineCount()
{
	int scrollBackAmout = 0;
	while (m_lines.size() > m_maxLineCount)
	{
		if (&m_lines.front() == m_filteredLines.front())
		{
			m_filteredLines.pop_front();
			++scrollBackAmout;
			++m_scrollBackAmount;
		}
		m_lines.pop_front();
	}

	MoveSelection(scrollBackAmout, false);
	MoveCursor(scrollBackAmout, false);
}
