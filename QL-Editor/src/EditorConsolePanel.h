#pragma once
#include "Quentlam/Core/Base.h"

namespace Quentlam
{
	class QUENTLAM_API EditorConsolePanel
	{
	public:
		EditorConsolePanel() = default;

		void OnImGuiRender();
		void Clear();

		bool IsOpen() const { return m_IsOpen; }
		void SetOpen(bool open) { m_IsOpen = open; }

	private:
		void DrawToolbar();
		void DrawOutput();
		void DrawInput();

		bool m_IsOpen = true;
		char m_InputBuffer[512] = { 0 };
		std::vector<std::string> m_FilteredHistory;
		char m_FilterBuffer[128] = { 0 };
		bool m_AutoScroll = true;
		int m_LastOutputSize = 0;
	};
}
