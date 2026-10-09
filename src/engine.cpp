#include "engine.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#include <iostream>
#include "constants.h"

Engine::Engine()
    : mWindow(std::make_unique<Window>(GAME_WIDTH, GAME_HEIGHT)),
    mDeltaTime(0),
    mLastTime(0),
    mAccTime(0)
{
    mRenderer = std::make_unique<Renderer>(mWindow.get());
}

void Engine::Start()
{
    init();

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop([]()
        {
            Engine::Get().loop();
        },
        0,
        true);
#else
    while (mWindow->IsRunning()) {
        loop();
    }

    destroy();
#endif
}

void Engine::init()
{
    mAssetManager.Init();

    mSpriteCache.AddSpritesheet(Spritesheet::CreateFromJson(SPRITESHEET_PLAYER, "tilesets/player.tsj"));
    mSpriteCache.AddSpritesheet(Spritesheet::CreateFromJson(SPRITESHEET_ENEMIES, "tilesets/enemies.tsj"));
    mSpriteCache.AddSpritesheet(Spritesheet::CreateFromJson(SPRITESHEET_FONT, "tilesets/glyphs.tsj"));
    mSpriteCache.AddSpritesheet(Spritesheet::CreateFromJson(SPRITESHEET_FIXTURES, "tilesets/interactables.tsj"));
    mSpriteCache.AddSpritesheet(Spritesheet::CreateFromJson(SPRITESHEET_DIALOGS, "tilesets/dialogs.tsj"));
    mSpriteCache.AddSpritesheet(Spritesheet::CreateFromJson(SPRITESHEET_TILES, "tilesets/environment-tiles.tsj", true));
    mSpriteCache.AddSpritesheet(Spritesheet::CreateFromJson(SPRITESHEET_ITEMS, "tilesets/items.tsj"));
    mSpriteCache.AddSpritesheet(Spritesheet::CreateFromJson(SPRITESHEET_UI, "tilesets/ui.tsj"));

    mWindow->Init();
    mRenderer->Init();

    glfwSetKeyCallback(mWindow->getGLFWWindow(),
        [](GLFWwindow*, int key, int, int action, int)
        {
            if (action == GLFW_REPEAT) return;

            const bool pressed = (action == GLFW_PRESS);

            switch (key) {
            case GLFW_KEY_A:
                GetInput().SetButton(Input::DPAD_LEFT, pressed);
                break;
            case GLFW_KEY_D:
                GetInput().SetButton(Input::DPAD_RIGHT, pressed);
                break;
            case GLFW_KEY_W:
                GetInput().SetButton(Input::DPAD_UP, pressed);
                break;
            case GLFW_KEY_S:
                GetInput().SetButton(Input::DPAD_DOWN, pressed);
                break;
            case GLFW_KEY_J:
                GetInput().SetButton(Input::BUTTON_Y, pressed);
                break;
            case GLFW_KEY_K:
            case GLFW_KEY_ENTER:
                GetInput().SetButton(Input::BUTTON_B, pressed);
                break;
            case GLFW_KEY_L:
                GetInput().SetButton(Input::BUTTON_A, pressed);
                break;
            case GLFW_KEY_I:
                GetInput().SetButton(Input::BUTTON_X, pressed);
                break;
            case GLFW_KEY_BACKSPACE:
                GetInput().SetButton(Input::BUTTON_START, pressed);
                break;
            case GLFW_KEY_TAB:
                GetInput().SetButton(Input::BUTTON_SELECT, pressed);
                break;
            case GLFW_KEY_LEFT_CONTROL:
                GetInput().SetButton(Input::BUTTON_LEFT, pressed);
                break;
            case GLFW_KEY_RIGHT_CONTROL:
                GetInput().SetButton(Input::BUTTON_RIGHT, pressed);
                break;
            }
        });

    mGame.LoadLevelList();
    mGame.Init();

    mLastTime = glfwGetTime();
}

void Engine::loop()
{
#ifndef __EMSCRIPTEN__
    glfwPollEvents();
#endif

    double t = glfwGetTime();
    mDeltaTime = std::min(t - mLastTime, MAX_FRAME_TIME);
    mLastTime = t;
    mAccTime += mDeltaTime;
    while (mAccTime >= FIXED_DT) {
        mInput.BeginFrame();
        mGame.Update();
        mInput.EndFrame();
        mAccTime -= FIXED_DT;
    }

    if (mGame.ConsumeLevelDirty()) {
        const Level* level = mGame.GetCurrentLevel();
        if (level) {
            mRenderer->LoadLevel(*level);
        }
    }

    mRenderer->Render(mGame);

#ifndef __EMSCRIPTEN__
    mRenderer->EndFrame();
#endif
}

void Engine::destroy()
{
    mRenderer->Destroy();
    mWindow->Destroy();
}