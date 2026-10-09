#pragma once

#include <glm/vec2.hpp>
#include "entity.h"
#include "tilemap.h"
#include <vector>
#include <array>
#include <memory>
#include <string>
#include "ui.h"

const int MAX_PLAYERS = 2;

enum LevelDialogs {
    LEVEL_DIALOG_PASSWORD,
    LEVEL_DIALOG_EXIT,
    LEVEL_DIALOG_HELP,
    LEVEL_DIALOG_SPEECH,
    LEVEL_DIALOGS_COUNT
};

struct LevelInfo {
    std::string password;
    std::string path;
    std::string nextPassword;
};

class Level {
public:
    glm::vec2 cameraPos;
    glm::vec2 exitPos;
    Tilemap tilemap;
    std::array<std::unique_ptr<Player>, MAX_PLAYERS> players;
    std::vector<std::unique_ptr<Mover>> enemies;
    std::vector<std::unique_ptr<Mover>> projectiles;
    std::vector<std::unique_ptr<Item>> items;
    std::vector<std::unique_ptr<Platform>> platforms;
    std::vector<std::unique_ptr<Interactable>> interactables;
    std::vector<std::unique_ptr<Controlable>> controlables;
    std::vector<std::unique_ptr<FXEffect>> effects;
    std::array<Dialog, LEVEL_DIALOGS_COUNT> dialogs;

    Level(const LevelInfo* info);
    void Update();
    void AddEntity(std::unique_ptr<Entity> entity);
    Player* GetNearestPlayer(const glm::vec2& pos, float radius) const;
    Platform* GetPlatformByID(int id) const;
    Controlable* GetStructureByID(int id) const;
    Interactable* GetOverlappingFixture(const Entity& entity) const;
    bool IsAnyDialogOpen() const;
    bool IsInventoryOpen() const;
    bool IsEntityVisible(const Entity& e) const;
    inline Player* GetSelectedPlayer() { return players[mSelectedPlayerIndex].get(); }
    inline const Player* GetSelectedPlayer() const { return players[mSelectedPlayerIndex].get(); }
    inline int GetSelectedPlayerIdx() const { return mSelectedPlayerIndex; }
    inline const LevelInfo* GetInfo() const { return mInfo; }
    inline bool isPlayerAlive(int idx) const { return players[idx] && !players[idx]->isDead(); }

private:
    const LevelInfo* mInfo;
    int mSelectedPlayerIndex;
    std::vector<std::unique_ptr<Entity>> mPendingEntities;

    void updateCamera();
    bool isGameOver();
};

class GameState {
public:
    enum class State {
        Menu,
        Level,
        Loading
    };
    enum class MenuState {
        Main,
        Options,
        Password
    };

    State state;
    MenuState menuState;

    Menu mainMenu;
    Menu optionsMenu;
    PasswordInput passwordInput;

    GameState();
    void LoadLevelList();
    void Init();
    const LevelInfo* GetLevelInfo(const std::string& password) const;
    bool LoadLevel(const std::string& password);
    void ExitLevel();
    void Update();
    bool ConsumeLevelDirty();
    inline Level* GetCurrentLevel() { return mCurrentLevel.get(); }
    inline const Level* GetCurrentLevel() const { return mCurrentLevel.get(); }

private:
    std::unique_ptr<Level> mCurrentLevel;
    std::vector<LevelInfo> mLevels;
    bool mLevelDirty = false;

    void updateMenu();
};