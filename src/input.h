#pragma once

#include <array>

class Input {
public:
	enum Button {
		DPAD_LEFT = 0,
		DPAD_UP,
		DPAD_RIGHT,
		DPAD_DOWN,
		BUTTON_A,
		BUTTON_B,
		BUTTON_X,
		BUTTON_Y,
		BUTTON_LEFT,
		BUTTON_RIGHT,
		BUTTON_START,
		BUTTON_SELECT,

		_BUTTON_COUNT
	};

private:
	std::array<bool, _BUTTON_COUNT> mPressed{};
	std::array<bool, _BUTTON_COUNT> mClicked{};
	std::array<bool, _BUTTON_COUNT> mCurrentClicked{};

public:
	inline bool IsPressed(Button button) const { return mPressed[button]; }
	inline bool IsClicked(Button button) const { return mCurrentClicked[button]; }
	void SetButton(Button button, bool pressed);
	void BeginFrame();
	void EndFrame();
};