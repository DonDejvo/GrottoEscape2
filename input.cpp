#include "input.h"

void Input::SetButton(Button button, bool pressed)
{
	if (pressed && !mPressed[button]) {
		mClicked[button] = true;
	}
	mPressed[button] = pressed;
}

void Input::BeginFrame()
{
	mCurrentClicked = mClicked;
}

void Input::EndFrame()
{
	for (int i = 0; i < _BUTTON_COUNT; ++i) {
		if (mCurrentClicked[i]) {
			mClicked[i] = false;
		}
	}
}
