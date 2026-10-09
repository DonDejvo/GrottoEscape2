#pragma once

#include "window.h"
#include "renderer.h"
#include "game_state.h"
#include "input.h"
#include "asset_manager.h"
#include <memory>

class Engine {
public:
    static inline Engine& Get() {
        static Engine instance;
        return instance;
    }

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void Start();
    static inline AssetManager& GetAssetManager() { return Get().mAssetManager; }
    static inline SpriteCache& GetSpriteCache() { return Get().mSpriteCache; }
    static inline Input& GetInput() { return Get().mInput; }
    static inline GameState& GetGame() { return Get().mGame; }
private:
    Engine();

    std::unique_ptr<Window> mWindow;
    std::unique_ptr<Renderer> mRenderer;

    AssetManager mAssetManager;
    SpriteCache mSpriteCache;
    Input mInput;
    GameState mGame;

    double mDeltaTime;
    double mLastTime;
    double mAccTime;

    void init();
    void loop();
    void destroy();
};