#pragma once

#include <algorithm>
#include <cmath>

namespace Quentlam::EditorToolbarLayout
{
	inline constexpr float kMinButtonSize = 34.0f;
	inline constexpr float kMaxButtonSize = 52.0f;
	inline constexpr float kBaseButtonSize = 42.0f;
	inline constexpr float kHorizontalPadding = 12.0f;
	inline constexpr float kVerticalPadding = 8.0f;
	inline constexpr float kToolbarTopMargin = 12.0f;
	inline constexpr float kButtonSpacingY = 6.0f;

	struct Metrics
	{
		float DpiScale = 1.0f;
		float ButtonSize = kBaseButtonSize;
		float WindowWidth = kBaseButtonSize + (kHorizontalPadding * 2.0f);
		float WindowHeight = kBaseButtonSize + (kVerticalPadding * 2.0f);
		float WindowX = 0.0f;
		float WindowY = kToolbarTopMargin;
		float ButtonX = kHorizontalPadding;
		float ButtonY = kVerticalPadding;
		float Row2Y = kVerticalPadding;
		float CornerRounding = 8.0f;
		float IconInset = 8.0f;
		bool IsClipped = false;
		bool IsClamped = false;
	};

	inline Metrics Calculate(int numButtons, float viewportWidth, float viewportTop, float uiScale = 1.0f, float dpiScale = 1.0f)
	{
		Metrics metrics;
		const float scale = std::max(uiScale, 1.0f) * std::max(dpiScale, 1.0f);
		const float availableWidth = std::max(viewportWidth, 0.0f);

		metrics.DpiScale = std::max(dpiScale, 1.0f);
		metrics.ButtonSize = std::clamp(std::round(kBaseButtonSize * scale), kMinButtonSize * scale, kMaxButtonSize * scale);
		const float paddingX = std::round(kHorizontalPadding * scale);
		const float paddingY = std::round(kVerticalPadding * scale);
		const float buttonSpacing = std::round(8.0f * scale);
		const float spacingY = std::round(kButtonSpacingY * scale);
		metrics.CornerRounding = std::round(10.0f * scale);
		metrics.IconInset = std::round(8.0f * scale);

		if (numButtons == 3)
		{
			// Row 1: 1 button (Play/Stop), centered
			float row1Width = metrics.ButtonSize + paddingX * 2.0f;
			// Row 2: 2 buttons (Pause, Step), centered side by side
			float row2Width = (metrics.ButtonSize * 2.0f) + buttonSpacing + paddingX * 2.0f;
			metrics.WindowWidth = std::max(row1Width, row2Width);

			// Row 1: centered, starts at top
			float row1CenteredX = std::round((availableWidth - metrics.WindowWidth) * 0.5f);
			metrics.ButtonX = row1CenteredX + paddingX;
			metrics.ButtonY = paddingY;

			// Row 2: centered below row 1
			float row2StartX = row1CenteredX + paddingX;
			metrics.Row2Y = paddingY + metrics.ButtonSize + spacingY;

			float centeredX = std::round((availableWidth - metrics.WindowWidth) * 0.5f);
			if (centeredX < 0.0f)
			{
				metrics.IsClamped = true;
				centeredX = 0.0f;
			}
			metrics.WindowX = centeredX;
			metrics.WindowY = viewportTop + std::round(kToolbarTopMargin * scale);
			metrics.WindowHeight = (metrics.ButtonSize * 2.0f) + spacingY + paddingY * 2.0f;
			metrics.IsClipped = metrics.WindowWidth > availableWidth;
			return metrics;
		}

		// Default single-row layout
		metrics.WindowWidth = (metrics.ButtonSize * numButtons) + (buttonSpacing * std::max(0, numButtons - 1)) + (paddingX * 2.0f);
		metrics.WindowHeight = metrics.ButtonSize + (paddingY * 2.0f);
		metrics.ButtonX = paddingX;
		metrics.ButtonY = paddingY;
		metrics.WindowY = viewportTop + std::round(kToolbarTopMargin * scale);
		metrics.Row2Y = paddingY;

		float centeredX = std::round((availableWidth - metrics.WindowWidth) * 0.5f);
		if (centeredX < 0.0f)
		{
			metrics.IsClamped = true;
			centeredX = 0.0f;
		}

		if (centeredX + metrics.WindowWidth > availableWidth)
		{
			metrics.IsClamped = true;
			centeredX = std::max(0.0f, availableWidth - metrics.WindowWidth);
		}

		metrics.WindowX = centeredX;
		metrics.IsClipped = metrics.WindowWidth > availableWidth;

		return metrics;
	}
}
