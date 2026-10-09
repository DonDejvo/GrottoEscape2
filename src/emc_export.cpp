#if defined(__EMSCRIPTEN__)

#include <emscripten.h>
#include "engine.h"
#include "input.h"

extern "C" {

    EMSCRIPTEN_KEEPALIVE
        void InputSetPressed(int button, bool pressed) {
        if (button >= 0 && button < Input::_BUTTON_COUNT) {
            Engine::GetInput().SetButton(static_cast<Input::Button>(button), pressed);
        }
    }

}

#endif