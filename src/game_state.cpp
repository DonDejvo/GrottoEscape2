#include "game_state.h"
#include "engine.h"
#include <iostream>
#include <algorithm>
#include "constants.h"
#include <sstream>

float cameraSpeed = 3.0f;
glm::vec2 playerCameraBounds = glm::vec2(16.0f, 8.0f);
Hitbox exitHitbox{32, 16};

Level::Level(const LevelInfo* info)
    : mInfo(info),
    mSelectedPlayerIndex(0),
    cameraPos(0, 0),
    exitPos(0, 0),
    tilemap(Tilemap::CreateFromJson("levels/" + info->path))
{
    const std::vector<ObjectData>& objects = tilemap.getObjects();
    for (const auto& data : objects) {
        std::unique_ptr<Entity> e;
        Interactable::Target target{
            data.target.id,
            data.target.type == "platform" ? EntityType::Platform : EntityType::Controlable
        };
        if (data.name == "ralph") {
            e = std::make_unique<Ralph>();
        }
        else if (data.name == "gregory") {
            e = std::make_unique<Gregory>();
        }
        else if (data.name == "slime") {
            e = std::make_unique<Slime>();
        }
        else if (data.name == "bat") {
            e = std::make_unique<Bat>();
        }
        else if (data.name == "dragon") {
            e = std::make_unique<Dragon>();
        }
        else if (data.name == "platform" && data.platform.index == 0) {
            std::vector<ObjectData> platformIndices;
            for (const auto& platformData : objects) {
                if (platformData.name == "platform" && platformData.id == data.id) {
                    platformIndices.push_back(platformData);
                }
            }
            std::vector<glm::vec2> points(platformIndices.size());
            for (const auto& platformData : platformIndices) {
                points[platformData.platform.index] = glm::vec2(platformData.x, platformData.y);
            }
            e = std::make_unique<Platform>(data.id, data.platform.speed, data.platform.solid, data.active, points);
        }
        else if (data.name == "door") {
            e = std::make_unique<Door>(data.id, data.width, data.height);
        }
        else if (data.name == "bridge") {
            e = std::make_unique<Bridge>(data.id, data.width, data.height, data.direction, data.bridge.tileid);
        }
        else if (data.name == "lock") {
            e = std::make_unique<DoorLock>(target, data.lock.color);
        }
        else if (data.name == "key_red" || data.name == "key_blue" || data.name == "key_yellow") {
            e = std::make_unique<Item>(data.name, ItemType::Key);
        }
        else if (data.name == "button") {
            e = std::make_unique<Button>(target);
        }
        else if (data.name == "lever") {
            e = std::make_unique<Lever>(target, data.lever.up);
        }
        else if (data.name == "help") {
            e = std::make_unique<HelpBlock>(data.help.text);
        }
        else if (data.name == "spawner") {
            ProjectileType projectileType = ProjectileType::Fireball;
            e = std::make_unique<Spawner>(data.id, projectileType, data.spawner.reload, data.direction, data.active);
        }
        else if (data.name == "exit") {
            exitPos.x = data.x;
            exitPos.y = data.y;
        }

        if (!e) continue;

        e->position.x = data.x;
        e->position.y = data.y;

        switch (e->GetType()) {
        case EntityType::Player:
            players[data.name == "ralph" ? 0 : 1] = std::unique_ptr<Player>(static_cast<Player*>(e.release()));
            break;
        case EntityType::Enemy:
            enemies.push_back(std::unique_ptr<Mover>(static_cast<Mover*>(e.release())));
            break;
        case EntityType::Projectile:
            projectiles.push_back(std::unique_ptr<Mover>(static_cast<Mover*>(e.release())));
            break;
        case EntityType::Item:
            items.push_back(std::unique_ptr<Item>(static_cast<Item*>(e.release())));
            break;
        case EntityType::Platform:
            platforms.push_back(std::unique_ptr<Platform>(static_cast<Platform*>(e.release())));
            break;
        case EntityType::Controlable:
            controlables.push_back(std::unique_ptr<Controlable>(static_cast<Controlable*>(e.release())));
            break;
        case EntityType::Interactable:
            interactables.push_back(std::unique_ptr<Interactable>(static_cast<Interactable*>(e.release())));
            break;
        default:
            break;
        }
    }

    if (players[mSelectedPlayerIndex]) {
        cameraPos = players[mSelectedPlayerIndex]->position;
        updateCamera();
    }

    dialogs[LEVEL_DIALOG_PASSWORD] = Dialog(Dialog::Type::System, "PASSWORD\n" + mInfo->password, false);
    dialogs[LEVEL_DIALOG_PASSWORD].SetOpened(true);
    dialogs[LEVEL_DIALOG_EXIT] = Dialog(Dialog::Type::System, "GIVE UP?", true, false, [](GameState& game) {
        game.ExitLevel();
        });
    dialogs[LEVEL_DIALOG_HELP].type = Dialog::Type::Help;
}

void Level::Update()
{
    const Input& input = Engine::GetInput();

    if (IsAnyDialogOpen()) {
        GameState& game = Engine::GetGame();
        for (int i = 0; i < LEVEL_DIALOGS_COUNT; ++i) {
            dialogs[i].Update(game);
        }
        return;
    }

    if (IsInventoryOpen()) {
        players[mSelectedPlayerIndex]->inventory.Update(*this);
        return;
    }

    if (isGameOver()) {
        GameState& game = Engine::GetGame();
        if (!game.LoadLevel(mInfo->nextPassword)) {
            game.ExitLevel();
        }
        return;
    }

    if (input.IsClicked(Input::BUTTON_START)) {
        dialogs[LEVEL_DIALOG_EXIT].SetOpened(true);
    }
    else if (input.IsClicked(Input::BUTTON_RIGHT) || input.IsClicked(Input::BUTTON_LEFT)) {
        int direction = input.IsClicked(Input::BUTTON_RIGHT) ? 1 : -1;
        for (int i = 1; i < MAX_PLAYERS; ++i) {
            int playerIdx = (mSelectedPlayerIndex + MAX_PLAYERS + i * direction) % MAX_PLAYERS;
            if (isPlayerAlive(playerIdx)) {
                players[mSelectedPlayerIndex]->ResetInput();
                mSelectedPlayerIndex = playerIdx;
                break;
            }
        }
    }

    Player* selectedPlayer = players[mSelectedPlayerIndex].get();
    if (selectedPlayer && IsEntityVisible(*selectedPlayer)) {
        selectedPlayer->HandleInput();
    }

    Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_TILES).Update();

    tilemap.Update(*this);

    for (auto& e : platforms) {
        e->Update(*this);
    }
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (!players[i]) continue;
        players[i]->Update(*this);
    }
    for (auto& e : enemies) {
        e->Update(*this);
    }
    for (auto& e : projectiles) {
        e->Update(*this);
    }
    for (auto& e : items) {
        e->Update(*this);
    }
    for (auto& e : interactables) {
        e->Update(*this);
    }
    for (auto& e : controlables) {
        e->Update(*this);
    }
    for (auto& e : effects) {
        e->Update(*this);
    }

    for (auto& e : mPendingEntities) {
        switch (e->GetType()) {
        case EntityType::Projectile:
            projectiles.push_back(std::unique_ptr<Mover>(static_cast<Mover*>(e.release())));
            break;
        case EntityType::Item:
            items.push_back(std::unique_ptr<Item>(static_cast<Item*>(e.release())));
            break;
        case EntityType::FX:
            effects.push_back(std::unique_ptr<FXEffect>(static_cast<FXEffect*>(e.release())));
            break;
        default:
            break;
        }
    }
    mPendingEntities.clear();

    updateCamera();

    std::erase_if(enemies, [](auto& e) { return !e->alive; });
    std::erase_if(projectiles, [](auto& e) { return !e->alive; });
    std::erase_if(items, [](auto& e) { return !e->alive; });
    std::erase_if(interactables, [](auto& e) { return !e->alive; });
    std::erase_if(effects, [](auto& e) { return !e->alive; });
}

void Level::AddEntity(std::unique_ptr<Entity> entity)
{
    mPendingEntities.push_back(std::move(entity));
}

Player* Level::GetNearestPlayer(const glm::vec2& pos, float radius) const
{
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        auto& player = players[i];
        if (player && glm::length(player->position - pos) < radius) {
            return player.get();
        }
    }
    return nullptr;
}

Platform* Level::GetPlatformByID(int id) const
{
    auto it = std::find_if(platforms.begin(), platforms.end(), [id](const auto& p) { return p->id == id; });
    if (it != platforms.end()) {
        return it->get();
    }
    return nullptr;
}

Controlable* Level::GetStructureByID(int id) const
{
    auto it = std::find_if(controlables.begin(), controlables.end(), [id](const auto& s) { return s->id == id; });
    if (it != controlables.end()) {
        return it->get();
    }
    return nullptr;
}

Interactable* Level::GetOverlappingFixture(const Entity& entity) const
{
    for (auto& fixture : interactables) {
        if (entity.Collide(*fixture)) {
            return fixture.get();
        }
    }
    return nullptr;
}

bool Level::IsAnyDialogOpen() const
{
    for (int i = 0; i < LEVEL_DIALOGS_COUNT; ++i) {
        if (dialogs[i].IsOpened()) return true;
    }
    return false;
}

bool Level::IsInventoryOpen() const
{
    if (players[mSelectedPlayerIndex]) {
        Inventory& inventory = players[mSelectedPlayerIndex]->inventory;
        return inventory.getMode() != Inventory::Mode::Idle;
    }
    return false;
}

bool Level::IsEntityVisible(const Entity& e) const
{
    float diffX = std::abs(cameraPos.x - e.position.x);
    float diffY = std::abs(cameraPos.y - e.position.y);
    return diffX <= GAME_WIDTH / SCALE * 0.5f + 8.0f &&
        (diffY <= GAME_HEIGHT - LEVEL_HEADER_HEIGHT) / SCALE * 0.5f + 8.0f;
}

void Level::updateCamera()
{
    Player* selectedPlayer = GetSelectedPlayer();
    if (!selectedPlayer) return;
    float diffX = std::abs(cameraPos.x - selectedPlayer->position.x);
    float diffY = std::abs(cameraPos.y - selectedPlayer->position.y);
    if (diffX > playerCameraBounds.x * 0.5f) {
        int direction = cameraPos.x > selectedPlayer->position.x ? -1 : 1;
        cameraPos.x += direction * std::min(diffX - playerCameraBounds.x * 0.5f, cameraSpeed);
    }
    if (diffY > playerCameraBounds.y * 0.5f) {
        int direction = cameraPos.y > selectedPlayer->position.y ? -1 : 1;
        cameraPos.y += direction * std::min(diffY - playerCameraBounds.y * 0.5f, cameraSpeed);
    }
    if (tilemap.getWidth() * tilemap.getTileWidth() >= GAME_WIDTH / SCALE) {
        float x;
        if (cameraPos.x < (x = GAME_WIDTH / SCALE * 0.5f - 0.5f * tilemap.getTileWidth())) {
            cameraPos.x = x;
        }
        else if (cameraPos.x > (x = (tilemap.getWidth() - 0.5f) * tilemap.getTileWidth() - GAME_WIDTH / SCALE * 0.5f)) {
            cameraPos.x = x;
        }
    }
    if (tilemap.getHeight() * tilemap.getTileHeight() >= (GAME_HEIGHT - LEVEL_HEADER_HEIGHT) / SCALE) {
        float y;
        if (cameraPos.y < (y = (GAME_HEIGHT - LEVEL_HEADER_HEIGHT) / SCALE * 0.5f - tilemap.getTileHeight() * 0.5f)) {
            cameraPos.y = y;
        }
        else if (cameraPos.y > (y = (tilemap.getHeight() - 0.5f) * tilemap.getTileHeight() - (GAME_HEIGHT - LEVEL_HEADER_HEIGHT) / SCALE * 0.5f)) {
            cameraPos.y = y;
        }
    }
}

bool Level::isGameOver()
{
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        const Player* p = players[i].get();
        if (p && (p->getRight() < exitPos.x - exitHitbox.w * 0.5f || 
            p->getLeft() > exitPos.x + exitHitbox.w * 0.5f || 
            p->getBottom() > exitPos.y + exitHitbox.h * 0.5f || 
            p->getTop() < exitPos.y - exitHitbox.h * 0.5f)) {
            return false;
        }
    }
    return true;
}

GameState::GameState()
    : mCurrentLevel(nullptr),
    state(GameState::State::Menu),
    menuState(GameState::MenuState::Main)
{
}

void GameState::LoadLevelList()
{
    std::istringstream is(Engine::GetAssetManager().LoadToString("levels.txt"));

    std::string line;
    while (std::getline(is, line))
    {
        if (line.empty())
            continue;

        std::istringstream lineStream(line);
        std::string password, path, nextPassword;

        lineStream >> password >> path >> nextPassword;

        mLevels.emplace_back(password, path, nextPassword);
    }
}

void GameState::Init()
{
    mainMenu.position = glm::vec2(0, 0);

    MenuItem newGameBtn{
        .label = "NEW GAME",
        .onClick = [](GameState& game, bool) {
            if (game.mLevels.size()) {
                std::string msg;
                game.LoadLevel(game.mLevels[0].password);
            }
        }
    };

    MenuItem optionsBtn{
        .label = "OPTIONS",
        .onClick = [](GameState& game, bool) {
            game.menuState = GameState::MenuState::Options;
            game.optionsMenu.ResetIndex();
        }
    };

    MenuItem passwordBtn{
        .label = "PASSWORD",
        .onClick = [](GameState& game, bool) {
            game.menuState = GameState::MenuState::Password;
        }
    };

    mainMenu.AddItem(std::move(newGameBtn));
    mainMenu.AddItem(std::move(optionsBtn));
    mainMenu.AddItem(std::move(passwordBtn));

    optionsMenu.position = glm::vec2(0, 0);

    MenuItem soundCheckbox{
        .label = "SOUND",
        .isCheckbox = true,
        .checkbox = {.checked = true, .checkedText = "ON", .uncheckedText = "OFF"}
    };

    MenuItem musicCheckbox{
        .label = "MUSIC",
        .isCheckbox = true,
        .checkbox = {.checked = true, .checkedText = "ON", .uncheckedText = "OFF"}
    };

    MenuItem optionsExitBtn{
        .label = "EXIT",
        .onClick = [](GameState& game, bool checked) {
            game.menuState = GameState::MenuState::Main;
            game.mainMenu.ResetIndex();
        }
    };

    optionsMenu.AddItem(std::move(musicCheckbox));
    optionsMenu.AddItem(std::move(soundCheckbox));
    optionsMenu.AddItem(std::move(optionsExitBtn));

    passwordInput.position = glm::vec2(0, -16.0f);
    if (mLevels.size()) {
        passwordInput.SetPassword(mLevels[0].password);
    }
}

const LevelInfo* GameState::GetLevelInfo(const std::string& password) const
{
    auto it = std::find_if(mLevels.begin(), mLevels.end(),
        [&password](const LevelInfo& info) { return info.password == password; });
    return it != mLevels.end() ? &(*it) : nullptr;
}

bool GameState::LoadLevel(const std::string& password)
{
    passwordInput.message = "";

    const LevelInfo* info = GetLevelInfo(password);

    if (!info) {
        passwordInput.message = "INVALID PASSWORD";
        return false;
    }
    try {
        state = State::Loading;
        mCurrentLevel = std::make_unique<Level>(info);
        state = State::Level;
        mLevelDirty = true;
        passwordInput.SetPassword(password);

        return true;
    }
    catch (const AssetNotFoundException& e) {
        passwordInput.message = "INVALID TILEMAP PATH";
    }

    return false;
}

void GameState::ExitLevel()
{
    mainMenu.ResetIndex();
    state = GameState::State::Menu;
    menuState = GameState::MenuState::Main;
}

void GameState::Update()
{
    switch (state)
    {
    case GameState::State::Menu:
        updateMenu();
        break;
    case GameState::State::Level:
        mCurrentLevel->Update();
        break;
    case GameState::State::Loading:
        break;
    }
}

bool GameState::ConsumeLevelDirty()
{
    bool wasDirty = mLevelDirty;
    mLevelDirty = false;
    return wasDirty;
}

void GameState::updateMenu()
{
    switch (menuState)
    {
    case GameState::MenuState::Main:
        mainMenu.Update(*this);
        break;
    case GameState::MenuState::Options:
        optionsMenu.Update(*this);
        break;
    case GameState::MenuState::Password:
        passwordInput.Update(*this);
        break;
    }
}
