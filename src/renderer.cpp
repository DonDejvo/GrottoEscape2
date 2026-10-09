#include "renderer.h"

#include <iostream>
#include <assert.h>
#include "stb_image.h"
#include "engine.h"
#include "text_analyzer.h"
#include "animation.h"

const float quad[] = {
    -0.5, -0.5, 0.0, 1.0,
    0.5, -0.5, 1.0, 1.0,
    0.5, 0.5, 1.0, 0.0,
    -0.5, 0.5, 0.0, 0.0
};

const int MAX_DRAW_CALLS = 16;

const glm::vec3 zero = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 one = glm::vec3(1.0f);
const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::mat4 identity = glm::mat4(1.0f);
const glm::vec2 origin = glm::vec2(-GAME_WIDTH / SCALE * 0.5f, GAME_HEIGHT / SCALE * 0.5f);
const float menuOffsetX = -32.0f;
const float inventoryCellSize = 8.0f;

Camera::Camera() 
    : viewMatrix(1.0f),
    projMatrix(1.0f),
    color(1.0f)
{
}

RenderLayer::RenderLayer()
    : data(nullptr),
    offset(0),
    drawCallsCount(0),
    drawCalls(MAX_DRAW_CALLS)
{
}

Renderer::Renderer(Window* window)
    : mWindow(window),
    mBackend(window),
    mRenderLayers(RENDER_LAYERS_COUNT),
    mTextureCache(MAX_TEXTURES),
    mLayersData(BUFFERS_COUNT),
    mCameraData(CAMERA_COUNT * CAMERA_BUFFER_SIZE),
    mCameras(CAMERA_COUNT)
{
    mLayersData[BUFFER_STATIC_TILES] = std::vector<float>(TILE_LAYER_COUNT * LAYER_MAX_TILES * SPRITE_STRIDE);
    mLayersData[BUFFER_DYNAMIC_TILES] = std::vector<float>(TILE_LAYER_COUNT * LAYER_MAX_DYNAMIC_TILES * SPRITE_STRIDE);
    mLayersData[BUFFER_ENTITIES] = std::vector<float>(MAX_SPRITES * SPRITE_STRIDE);
    mLayersData[BUFFER_UI] = std::vector<float>(MAX_UI_SPRITES * SPRITE_STRIDE);

    auto setLayer = [this](int layer, LayerBuffer bufferIdx, size_t offset = 0) {
        mRenderLayers[layer].data = mLayersData[bufferIdx].data();
        mRenderLayers[layer].offset = offset;
        };

    setLayer(RENDER_LAYER_BACKGROUND_STATIC, BUFFER_STATIC_TILES);
    setLayer(RENDER_LAYER_MIDGROUND_STATIC, BUFFER_STATIC_TILES, LAYER_MAX_TILES);
    setLayer(RENDER_LAYER_FOREGROUND_STATIC, BUFFER_STATIC_TILES, 2 * LAYER_MAX_TILES);

    setLayer(RENDER_LAYER_BACKGROUND_DYNAMIC, BUFFER_DYNAMIC_TILES);
    setLayer(RENDER_LAYER_MIDGROUND_DYNAMIC, BUFFER_DYNAMIC_TILES, LAYER_MAX_DYNAMIC_TILES);
    setLayer(RENDER_LAYER_FOREGROUND_DYNAMIC, BUFFER_DYNAMIC_TILES, 2 * LAYER_MAX_DYNAMIC_TILES);

    int offset = 0;
    setLayer(RENDER_LAYER_ENTITIES_PLATFORMS, BUFFER_ENTITIES);
    offset += MAX_PLATFORMS;
    setLayer(RENDER_LAYER_ENTITIES_FIXTURES, BUFFER_ENTITIES, offset);
    offset += MAX_FIXTURES;
    setLayer(RENDER_LAYER_ENTITIES_ITEMS, BUFFER_ENTITIES, offset);
    offset += MAX_ITEMS;
    setLayer(RENDER_LAYER_ENTITIES_MOVERS, BUFFER_ENTITIES, offset);
    offset += MAX_MOVERS;
    setLayer(RENDER_LAYER_ENTITIES_PROJECTILES, BUFFER_ENTITIES, offset);
    offset += MAX_PROJECTILES;
    setLayer(RENDER_LAYER_ENTITIES_FX, BUFFER_ENTITIES, offset);

    offset = 0;
    setLayer(RENDER_LAYER_UI, BUFFER_UI);
    offset += MAX_STATIC_UI_SPRITES;
    setLayer(RENDER_LAYER_LEVEL_UI, BUFFER_UI, offset);

    mCameras[CAMERA_UI].projMatrix = glm::orthoRH(
        -0.5f * GAME_WIDTH / SCALE,
        0.5f * GAME_WIDTH / SCALE,
        -0.5f * GAME_HEIGHT / SCALE,
        0.5f * GAME_HEIGHT / SCALE,
        -1.0f,
        1.0f
    );
    mCameras[CAMERA_MAIN].projMatrix = glm::orthoRH(
        -0.5f * GAME_WIDTH / SCALE,
        0.5f * GAME_WIDTH / SCALE,
        -0.5f * (GAME_HEIGHT - LEVEL_HEADER_HEIGHT) / SCALE,
        0.5f * (GAME_HEIGHT - LEVEL_HEADER_HEIGHT) / SCALE,
        -1.0f,
        1.0f
    );
}

void Renderer::Init() {
    mBackend.Init();
    initTextures();
}

void Renderer::LoadLevel(const Level& level)
{
    const Spritesheet& tileset = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_TILES);
    const Tilemap& tilemap = level.tilemap;
    
    clearLayer(RENDER_LAYER_BACKGROUND_STATIC);
    clearLayer(RENDER_LAYER_MIDGROUND_STATIC);
    clearLayer(RENDER_LAYER_FOREGROUND_STATIC);

    for (int i = 0; i < TILE_LAYER_COUNT; ++i) {
        const TileLayer& tileLayer = tilemap.getTileLayer(static_cast<TileLayerIndex>(i));
        for (int y = 0; y < tileLayer.getHeight(); ++y) {
            for (int x = 0; x < tileLayer.getWidth(); ++x) {
                const Tile& tile = tileLayer.GetTile(y * tileLayer.getWidth() + x);
                if (tile.id && !tile.isDynamic) {
                    addTile(static_cast<RenderLayerIndex>(i), tileset, tileLayer, x, y, tilemap.getTileWidth(), tilemap.getTileHeight());
                }
            }
        }
    }

    const Spritesheet& spritesheetFixtures = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_FIXTURES);
    addSprite(RENDER_LAYER_BACKGROUND_STATIC, level.exitPos, glm::vec2(32, 16), spritesheetFixtures.GetRegion(9, 2, 1), spritesheetFixtures.id);

    mBackend.UploadLayerBuffer(BUFFER_STATIC_TILES, mLayersData[BUFFER_STATIC_TILES].data(), mLayersData[BUFFER_STATIC_TILES].size());
}

void Renderer::Render(const GameState& game) {

    mBackend.BeginPass();

    switch (game.state)
    {
    case GameState::State::Level:
    {
        const Level& level = *game.GetCurrentLevel();
        glm::vec3 eye = glm::vec3(level.cameraPos.x, level.cameraPos.y, 1.0f);
        glm::vec3 center = glm::vec3(level.cameraPos.x, level.cameraPos.y, 0.0f);
        mCameras[CAMERA_MAIN].viewMatrix = glm::lookAt(eye, center, up);
        mCameras[CAMERA_MAIN].color = (level.dialogs[LEVEL_DIALOG_EXIT].IsOpened() || level.IsInventoryOpen()) ? glm::vec3(0.5f) : one;
        updateCamera();
        addTiles(level);
        addEntities(level);
        addLevelUI(level);
        setCamera(CameraBuffer::CAMERA_MAIN);
        drawTileLayer(TILE_LAYER_BACKGROUND);
        drawEntites();
        drawTileLayer(TILE_LAYER_MIDGROUND);
        drawFX();
        drawTileLayer(TILE_LAYER_FOREGROUND);
        drawUI();
        break;
    }
    case GameState::State::Menu:
    {
        updateCamera();
        addMenuUI(game, 0.5f);
        drawUI();
        break;
    }
    default:
        break;
    }

    mBackend.EndPass();
}

void Renderer::EndFrame() {
    mBackend.EndFrame();
}

void Renderer::Destroy()
{
    mBackend.Destroy();
}

void Renderer::initTextures()
{
    mTextureCache[SPRITESHEET_PLAYER] = loadTexture("textures/player.png");
    mTextureCache[SPRITESHEET_ENEMIES] = loadTexture("textures/enemies.png");
    mTextureCache[SPRITESHEET_TILES] = loadTexture("textures/environment-tiles_extruded.png");
    mTextureCache[SPRITESHEET_ITEMS] = loadTexture("textures/items.png");
    mTextureCache[SPRITESHEET_FIXTURES] = loadTexture("textures/interactables.png");
    mTextureCache[SPRITESHEET_FONT] = loadTexture("textures/glyphs.png");
    mTextureCache[SPRITESHEET_DIALOGS] = loadTexture("textures/dialogs.png");
    mTextureCache[SPRITESHEET_UI] = loadTexture("textures/ui.png");
}

void Renderer::updateCamera()
{
    for (int i = 0; i < CAMERA_COUNT; ++i) {
        const Camera& camera = mCameras[i];
        std::copy(
            glm::value_ptr(camera.viewMatrix),
            glm::value_ptr(camera.viewMatrix) + 16,
            mCameraData.begin() + CAMERA_BUFFER_SIZE * i
        );
        std::copy(
            glm::value_ptr(camera.projMatrix),
            glm::value_ptr(camera.projMatrix) + 16,
            mCameraData.begin() + CAMERA_BUFFER_SIZE * i + 16
        );
        std::copy(
            glm::value_ptr(camera.color),
            glm::value_ptr(camera.color) + 3,
            mCameraData.begin() + CAMERA_BUFFER_SIZE * i + 32
        );
    }

    mBackend.UploadCameraBuffer(mCameraData.data());
}

void Renderer::setCamera(CameraBuffer idx)
{
    switch (idx)
    {
    case CAMERA_UI:
        mBackend.SetViewport(0, 0, GAME_WIDTH, GAME_HEIGHT);
        break;
    case CAMERA_MAIN:
        mBackend.SetViewport(0, 0, GAME_WIDTH, GAME_HEIGHT - LEVEL_HEADER_HEIGHT);
        break;
    default:
        break;
    }
    mBackend.SetCamera(idx);
}

void Renderer::addTiles(const Level& level)
{
    const Spritesheet& tileset = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_TILES);
    const Tilemap& tilemap = level.tilemap;

    clearLayer(RENDER_LAYER_BACKGROUND_DYNAMIC);
    clearLayer(RENDER_LAYER_MIDGROUND_DYNAMIC);
    clearLayer(RENDER_LAYER_FOREGROUND_DYNAMIC);

    for (int i = 0; i < TILE_LAYER_COUNT; ++i) {
        const TileLayer& tileLayer = tilemap.getTileLayer(static_cast<TileLayerIndex>(i));
        const auto& dynamicTiles = tileLayer.getDynamicTiles();
        for (int idx : dynamicTiles) {
            const Tile& tile = tileLayer.GetTile(idx);
            if (tile.id == 0) continue;
            addTile(static_cast<RenderLayerIndex>(i + 3), tileset, tileLayer, tile.x, tile.y, tilemap.getTileWidth(), tilemap.getTileHeight());
        }
    }

    mBackend.UploadLayerBuffer(BUFFER_DYNAMIC_TILES, mLayersData[BUFFER_DYNAMIC_TILES].data(), mLayersData[BUFFER_DYNAMIC_TILES].size());
}

void Renderer::addEntities(const Level& level)
{
    clearLayer(RENDER_LAYER_ENTITIES_MOVERS);
    clearLayer(RENDER_LAYER_ENTITIES_ITEMS);
    clearLayer(RENDER_LAYER_ENTITIES_FIXTURES);
    clearLayer(RENDER_LAYER_ENTITIES_PLATFORMS);
    clearLayer(RENDER_LAYER_ENTITIES_FX);
    clearLayer(RENDER_LAYER_ENTITIES_PROJECTILES);

    for (const auto& enemy : level.enemies) {
        addAnimatedEntity(RENDER_LAYER_ENTITIES_MOVERS, *enemy);
    }
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (level.players[i]) {
            addAnimatedEntity(RENDER_LAYER_ENTITIES_MOVERS, *level.players[i]);
        }
    }
    for (const auto& projectile : level.projectiles) {
        addAnimatedEntity(RENDER_LAYER_ENTITIES_PROJECTILES, *projectile);
    }
    for (const auto& item : level.items) {
        addEntity(RENDER_LAYER_ENTITIES_ITEMS, *item, item->spriteRegion);
    }
    for (const auto& platform : level.platforms) {
        addEntity(RENDER_LAYER_ENTITIES_PLATFORMS, *platform, platform->spriteRegion);
    }
    for (const auto& fixture : level.interactables) {
        addEntity(RENDER_LAYER_ENTITIES_FIXTURES, *fixture, fixture->spriteRegion);
    }
    for (const auto& effect : level.effects) {
        addAnimatedEntity(RENDER_LAYER_ENTITIES_FX, *effect);
    }

    mBackend.UploadLayerBuffer(BUFFER_ENTITIES, mLayersData[BUFFER_ENTITIES].data(), mLayersData[BUFFER_ENTITIES].size());
}

void Renderer::addLevelUI(const Level& level)
{
    clearLayer(RENDER_LAYER_UI);
    clearLayer(RENDER_LAYER_LEVEL_UI);
    for (int i = 0; i < LEVEL_DIALOGS_COUNT; ++i) {
        const Dialog& dialog = level.dialogs[i];
        if (!dialog.IsOpened()) continue;
        if(dialog.type == Dialog::Type::System) {
            addDialog(RENDER_LAYER_UI, dialog, 0.5f);
        }
        else {
            addDialog(RENDER_LAYER_LEVEL_UI, dialog, 0.5f);
        }
    }
    const Spritesheet& spritesheetUI = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_UI);
    const Spritesheet& spritesheetPlayer = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_PLAYER);
    glm::vec2 tileScale = glm::vec2(16.0f, 16.0f);
    for (int i = 0; i < 10; ++i) {
        addSprite(RENDER_LAYER_UI, origin + glm::vec2((i + 0.5f) * 16.0f, -8.0f), tileScale, spritesheetUI.GetRegion(7), spritesheetUI.id);
    }
    for (int k = 0; k < 10; k += 4) {
        for (int i = 0; i < 3; ++i) {
            if (k == 8) ++k;
            addSprite(RENDER_LAYER_UI, origin + glm::vec2((k + 0.5f) * 16.0f, -(i + 1) * 8.0f), tileScale, spritesheetUI.GetRegion(7), spritesheetUI.id);
            
        }
    }
    for (int i = 1; i < 6; i += 4) {
        addSprite(RENDER_LAYER_UI, origin + glm::vec2((i + 0.5f) * 16.0f, -16.0f), tileScale, spritesheetUI.GetRegion(6), spritesheetUI.id);
        addSprite(RENDER_LAYER_UI, origin + glm::vec2((i + 1.5f) * 16.0f, -16.0f), tileScale, spritesheetUI.GetRegion(5), spritesheetUI.id);
        addSprite(RENDER_LAYER_UI, origin + glm::vec2((i + 1.0f) * 16.0f, -12.0f), glm::vec2(24.0f, 8.0f), spritesheetUI.GetRegion(9), spritesheetUI.id);
        addSprite(RENDER_LAYER_UI, origin + glm::vec2((i + 2.5f) * 16.0f, -16.0f), tileScale, spritesheetUI.GetRegion(10), spritesheetUI.id);
    }
    addSprite(RENDER_LAYER_UI, origin + glm::vec2(8.5f * 16.0f, -16.0f), tileScale, spritesheetUI.GetRegion(5), spritesheetUI.id);
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        const Player* player = level.players[i].get();
        if (!player) continue;
        glm::vec2 offset = getPlayerHeaderOffset(i);
        int spriteID = level.GetSelectedPlayerIdx() == i ? player->baseSpriteID : player->baseSpriteID + 2 * _PLAYER_SPRITESHEET_COLS;
        addSprite(RENDER_LAYER_UI, offset, glm::vec2(16.0f, 8.0f), spritesheetPlayer.GetRegion(spriteID, 1, 0.5f), spritesheetPlayer.id);
        glm::vec2 heartScale = glm::vec2(16.0f, 16.0f);
        for (int j = 0; j < player->hitpoints; ++j) {
            glm::vec2 heartOffset = glm::vec2((j - 1) * 8.0f, -8.0f);
            addSprite(RENDER_LAYER_UI, offset + heartOffset, heartScale, spritesheetUI.GetRegion(1), spritesheetUI.id);
        }
        addInventory(level, i);
    }
    
    mBackend.UploadLayerBuffer(BUFFER_UI, mLayersData[BUFFER_UI].data(), mLayersData[BUFFER_UI].size());
}

void Renderer::addMenuUI(const GameState& game, float scale)
{
    clearLayer(RENDER_LAYER_UI);
    switch (game.menuState)
    {
    case GameState::MenuState::Main:
        addMenu(game.mainMenu, scale);
        break;
    case GameState::MenuState::Options:
        addMenu(game.optionsMenu, scale);
        break;
    case GameState::MenuState::Password:
        addPassword(game.passwordInput, scale);
        break;
    }
    mBackend.UploadLayerBuffer(BUFFER_UI, mLayersData[BUFFER_UI].data(), mLayersData[BUFFER_UI].size());
}

void Renderer::addMenu(const Menu& menu, float scale)
{
    const glm::vec2 offset = glm::vec2(menuOffsetX, 0);
    glm::vec2 itemOffset = glm::vec2(0);
    for (const auto& item : menu.getItems()) {
        addText(RENDER_LAYER_UI, item.label, menu.position + (offset + itemOffset) * scale, TextAlign::Left, scale, 0);
        if (item.isCheckbox) {
            const std::string& text = item.checkbox.checked ? item.checkbox.checkedText : item.checkbox.uncheckedText;
            addText(RENDER_LAYER_UI, text, menu.position + (offset + itemOffset + glm::vec2(48.0f, 0.0f)) * scale, TextAlign::Left, scale, 0);
        }
        itemOffset.y -= 16.0f;
    }
    const Spritesheet& spritesheet = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_PLAYER);
    glm::vec2 pointerPos = menu.position + (offset + glm::vec2(-12.0f, menu.getActiveIndex() * -16.0f)) * scale;
    glm::vec2 pointerScale = glm::vec2(spritesheet.spriteWidth, spritesheet.spriteHeight) * scale;
    addSprite(RENDER_LAYER_UI, pointerPos, pointerScale, spritesheet.GetRegion(_PLAYER_SPRITESHEET_COLS + 3), spritesheet.id);
}

void Renderer::addPassword(const PasswordInput& input, float scale)
{
    glm::vec2 offset = glm::vec2(menuOffsetX, 0);
    if (input.IsInvalid()) {
        addText(RENDER_LAYER_UI, input.message, input.position + glm::vec2(0.0f, 16.0f) * scale, TextAlign::Center, scale, 0);
    }
    else {
        addText(RENDER_LAYER_UI, "PASSWORD", input.position + offset * scale, TextAlign::Left, scale, 0);
        glm::vec2 valueOffset = glm::vec2(16.0f, -16.0f);
        addText(RENDER_LAYER_UI, input.GetValue(), input.position + (offset + valueOffset) * scale, TextAlign::Left, scale, 0);
        const Spritesheet& spritesheetUI = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_UI);
        glm::vec2 pointerPos = input.position + (offset + valueOffset + glm::vec2(input.getActiveIndex() * 0.5f * spritesheetUI.spriteWidth, 0.0f)) * scale;
        glm::vec2 pointerScale = glm::vec2(spritesheetUI.spriteWidth, spritesheetUI.spriteHeight) * scale;
        addSprite(RENDER_LAYER_UI, pointerPos, pointerScale, spritesheetUI.GetRegion(3), spritesheetUI.id);
    }
}

void Renderer::addDialog(RenderLayerIndex layerIdx, const Dialog& dialog, float scale)
{
    const Spritesheet& dialogsSpritesheet = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_DIALOGS);
    float maxWidth = 200;
    TextAnalyzer textAnalyzer(dialog.text, floorf(maxWidth / dialogsSpritesheet.spriteWidth));
    glm::vec2 tileScale = glm::vec2(dialogsSpritesheet.spriteWidth, dialogsSpritesheet.spriteHeight);
    int cols = textAnalyzer.GetMaxRowLength();
    int rows = dialog.isConfirm ? textAnalyzer.GetRowCount() + 2 : textAnalyzer.GetRowCount();
    int baseTileID = 1;
    switch (dialog.type) {
    case Dialog::Type::Help:
        baseTileID = 5;
        break;
    default:
        break;
    }
    int sheetCols = dialogsSpritesheet.columns;

    glm::vec2 offset = glm::vec2(0.0f, (rows - 1) * 0.5f * tileScale.y);

    for (int i = -1; i <= rows; ++i) {
        bool isTop = (i == -1);
        bool isBottom = (i == rows);
        for (int j = -1; j <= cols; ++j) {
            bool isLeft = (j == -1);
            bool isRight = (j == cols);

            int tileID;
            if (isTop && isLeft)          tileID = baseTileID;                          // top left
            else if (isTop && isRight)    tileID = baseTileID + 3;                      // top right
            else if (isBottom && isLeft)  tileID = baseTileID + 2 * sheetCols;          // bottom left
            else if (isBottom && isRight) tileID = baseTileID + 2 * sheetCols + 3;      // bottom right
            else if (isTop)                tileID = baseTileID + 1;                     // top
            else if (isBottom)             tileID = baseTileID + 2 * sheetCols + 1;     // bottom
            else if (isLeft)               tileID = baseTileID + sheetCols;             // left
            else if (isRight)              tileID = baseTileID + sheetCols + 3;         // right
            else                           tileID = baseTileID + sheetCols + 1;         // fill

            glm::vec2 tileOffset = glm::vec2((j - 0.5f * cols) * tileScale.x, -i * tileScale.y);
            addSprite(layerIdx, dialog.position + (offset + tileOffset) * scale, tileScale * scale, dialogsSpritesheet.GetRegion(tileID), SPRITESHEET_DIALOGS);
        }
    }

    if (dialog.type == Dialog::Type::System) {
        addText(layerIdx, textAnalyzer, dialog.position + offset * scale, TextAlign::Center, scale);
    }
    else {
        addText(layerIdx, textAnalyzer, dialog.position + (offset - glm::vec2(cols * 0.5f * tileScale.x, 0)) * scale, TextAlign::Left, scale);
    }

    if (dialog.isConfirm) {
        if (!dialog.GetValue() || dialog.GetAnimationFrame() == 1) {
            addText(layerIdx, "YES", dialog.position + (offset + glm::vec2(-(cols - 1) * 0.5f * tileScale.x, -(rows - 1) * tileScale.y)) * scale, TextAlign::Left, scale, 0);
        }
        if (dialog.GetValue() || dialog.GetAnimationFrame() == 1) {
            addText(layerIdx, "NO", dialog.position + (offset + glm::vec2((cols - 1) * 0.5f * tileScale.x, -(rows - 1) * tileScale.y)) * scale, TextAlign::Right, scale, 0);
        }
    }
}

void Renderer::addInventory(const Level& level, int playerIdx)
{
    const Inventory& inventory = level.players[playerIdx]->inventory;
    const Spritesheet& spritesheetUI = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_UI);
    glm::vec2 position = getInventoryPosition(playerIdx);
    glm::vec2 itemScale = glm::vec2(8.0f);
    for (int i = 0; i < INVETORY_SIZE; ++i) {
        const Item* item = inventory.getItem(i);
        bool isTransfering = i == inventory.getSelectedIdx() && 
            inventory.getMode() == Inventory::Mode::Transfer;
        if (!item || (isTransfering && (inventory.getTransferIdx() != -1 || inventory.GetAnimationFrame() == 0))) continue;
        glm::vec2 itemOffset = glm::vec2(i % INVENTORY_COLS * inventoryCellSize, -i / INVENTORY_COLS * inventoryCellSize);
        addSprite(RENDER_LAYER_UI, position + itemOffset, itemScale, item->spriteRegion, SPRITESHEET_ITEMS);
    }
    if (inventory.getMode() == Inventory::Mode::Transfer && inventory.GetAnimationFrame() == 1 && inventory.getTransferIdx() != -1 && level.players[inventory.getTransferIdx()]) {
        glm::vec2 transferInventoryPosition = getInventoryPosition(inventory.getTransferIdx());
        const Inventory& transferInventory = level.players[inventory.getTransferIdx()]->inventory;
        int i = transferInventory.GetEmptySlotIdx();
        glm::vec2 itemOffset = glm::vec2(i % INVENTORY_COLS * inventoryCellSize, -i / INVENTORY_COLS * inventoryCellSize);
        addSprite(RENDER_LAYER_UI, transferInventoryPosition + itemOffset, itemScale, inventory.getItem(inventory.getSelectedIdx())->spriteRegion, SPRITESHEET_ITEMS);
    }
    if (inventory.getMode() != Inventory::Mode::Select || inventory.GetAnimationFrame() == 1) {
        int selectedCol = inventory.getSelectedIdx() % INVENTORY_COLS;
        int selectedRow = inventory.getSelectedIdx() / INVENTORY_COLS;
        glm::vec2 pointerPos = position + glm::vec2(selectedCol * inventoryCellSize, -selectedRow * inventoryCellSize);
        glm::vec2 pointerScale = glm::vec2(spritesheetUI.spriteWidth, spritesheetUI.spriteHeight);
        addSprite(RENDER_LAYER_UI, pointerPos, pointerScale, spritesheetUI.GetRegion(2), spritesheetUI.id);
    }
}

glm::vec2 Renderer::getPlayerHeaderOffset(int playerIdx) const
{
    return origin + glm::vec2(32.0f + playerIdx * 64.0f, -12.0f);
}

glm::vec2 Renderer::getInventoryPosition(int playerIdx) const
{
    return getPlayerHeaderOffset(playerIdx) + glm::vec2(20.0f, 0.0f);
}

void Renderer::addHitpoints(const Player& player, const glm::vec2& position)
{
}

void Renderer::drawTileLayer(TileLayerIndex tileLayerIdx)
{
    mBackend.SetLayerBuffer(LayerBuffer::BUFFER_STATIC_TILES);
    drawLayer(static_cast<RenderLayerIndex>(tileLayerIdx));
    mBackend.SetLayerBuffer(LayerBuffer::BUFFER_DYNAMIC_TILES);
    drawLayer(static_cast<RenderLayerIndex>(tileLayerIdx + 3));
}

void Renderer::drawEntites()
{
    mBackend.SetLayerBuffer(LayerBuffer::BUFFER_ENTITIES);
    drawLayer(RENDER_LAYER_ENTITIES_PROJECTILES);
    drawLayer(RENDER_LAYER_ENTITIES_ITEMS);
    drawLayer(RENDER_LAYER_ENTITIES_FIXTURES);
    drawLayer(RENDER_LAYER_ENTITIES_PLATFORMS);
    drawLayer(RENDER_LAYER_ENTITIES_MOVERS);
}

void Renderer::drawFX()
{
    mBackend.SetLayerBuffer(LayerBuffer::BUFFER_ENTITIES);
    drawLayer(RENDER_LAYER_ENTITIES_FX);
}

void Renderer::drawUI()
{
    mBackend.SetLayerBuffer(BUFFER_UI);
    setCamera(CameraBuffer::CAMERA_MAIN);
    drawLayer(RENDER_LAYER_LEVEL_UI);
    setCamera(CameraBuffer::CAMERA_UI);
    drawLayer(RENDER_LAYER_UI);
}

void Renderer::clearLayer(RenderLayerIndex layerIdx)
{
    mRenderLayers[layerIdx].drawCallsCount = 0;
}

void Renderer::drawLayer(RenderLayerIndex layerIdx)
{
    const RenderLayer& layer = mRenderLayers[layerIdx];
    for (int i = 0; i < layer.drawCallsCount; ++i) {
        const DrawCall& drawCall = layer.drawCalls[i];
        mBackend.SetTexture(mTextureCache[drawCall.textureID].id);
        mBackend.Draw(drawCall.count, layer.offset + drawCall.offset);
    }
}

void Renderer::addText(RenderLayerIndex layerIdx, const TextAnalyzer& textAnalyzer, const glm::vec2& position, TextAlign textAlign, float scale)
{
    const Spritesheet& fontSpritesheet = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_FONT);
    glm::vec2 charScale = glm::vec2(fontSpritesheet.spriteWidth, fontSpritesheet.spriteHeight) * scale;

    int rowIdx = 0;
    int maxRowLength = textAnalyzer.GetMaxRowLength();
    for (const std::string& row : textAnalyzer.GetRows()) {
        int colIdx = 0;
        int rowLength = row.length();
        float rowOffset = 0.0f;
        switch (textAlign) {
        case TextAlign::Left:
            rowOffset = 0.0f;
            break;
        case TextAlign::Center:
            rowOffset = -rowLength * 0.5f * charScale.x;
            break;
        case TextAlign::Right:
            rowOffset = -rowLength * charScale.x;
            break;
        }
        for (char ch : row) {
            if (ch != ' ') {
                int tileID = ch + 1;
                SpriteRegion region = fontSpritesheet.GetRegion(tileID);
                glm::vec2 charPos = glm::vec2(position.x + rowOffset + colIdx * charScale.x, position.y - rowIdx * charScale.y);
                addSprite(layerIdx, charPos, charScale, region, SPRITESHEET_FONT);
            }
            ++colIdx;
        }
        ++rowIdx;
    }
}

void Renderer::addText(RenderLayerIndex layerIdx, const std::string& text, const glm::vec2& position, TextAlign textAlign, float scale, float maxWidth)
{
    const Spritesheet& fontSpritesheet = Engine::GetSpriteCache().GetSpritesheet(SPRITESHEET_FONT);
    glm::vec2 charScale = glm::vec2(fontSpritesheet.spriteWidth, fontSpritesheet.spriteHeight) * scale;

    int rowMaxLength = floorf(maxWidth / charScale.x);
    TextAnalyzer textAnalyzer(text, rowMaxLength);

    addText(layerIdx, textAnalyzer, position, textAlign, scale);
}

void Renderer::addTile(RenderLayerIndex layerIdx, const Spritesheet& tileset, const TileLayer& tileLayer, int x, int y, int tileWidth, int tileHeight)
{
    const Tile& tile = tileLayer.GetTile(y * tileLayer.getWidth() + x);
    glm::vec2 position(x * tileWidth, y * tileHeight);
    glm::vec2 scale(tileWidth, tileHeight);
    SpriteRegion region = tileset.GetRegion(tile.id);
    addSprite(layerIdx, position, scale, region, SPRITESHEET_TILES);
}

void Renderer::addEntity(RenderLayerIndex layerIdx, const Entity& entity, const SpriteRegion& region)
{
    const Spritesheet& spritesheet = entity.getSpritesheet();
    glm::vec2 scale(region.width * (entity.orientation == Orientation::LEFT ? -1 : 1), region.height);
    addSprite(layerIdx, entity.position, scale, region, spritesheet.id);
}

void Renderer::addAnimatedEntity(RenderLayerIndex layerIdx, const Entity& entity)
{
    SpriteRegion region = entity.getSpritesheet().GetRegion(entity.getAnimator().GetCurrentFrameID());
    addEntity(layerIdx, entity, region);
}

void Renderer::addSprite(RenderLayerIndex layerIdx, const glm::vec2& position, const glm::vec2& scale, const SpriteRegion& region, int textureID)
{
    RenderLayer& layer = mRenderLayers[layerIdx];
    if (layer.drawCallsCount == 0) {
        layer.drawCalls[0] = { 0, 0, textureID };
        ++layer.drawCallsCount;
    }
    else if(layer.drawCalls[layer.drawCallsCount - 1].textureID != textureID) {
        const DrawCall& prevDrawCall = layer.drawCalls[layer.drawCallsCount - 1];
        layer.drawCalls[layer.drawCallsCount] = { 0, prevDrawCall.offset + prevDrawCall.count, textureID };
        ++layer.drawCallsCount;
    }
    DrawCall& drawCall = layer.drawCalls[layer.drawCallsCount - 1];

    for (int i = 0; i < 4; ++i) {
        int offset = i * SPRITE_VERTEX_STRIDE + (layer.offset + drawCall.offset + drawCall.count) * SPRITE_STRIDE;
        layer.data[offset] = position.x + quad[i * SPRITE_VERTEX_STRIDE] * scale.x;
        layer.data[offset + 1] = position.y + quad[i * SPRITE_VERTEX_STRIDE + 1] * scale.y;
        glm::vec2 uv = getSpriteRegionUv(region, mTextureCache[textureID], glm::vec2(quad[i * SPRITE_VERTEX_STRIDE + 2], quad[i * SPRITE_VERTEX_STRIDE + 3]));
        layer.data[offset + 2] = uv.x;
        layer.data[offset + 3] = uv.y;
    }

    ++drawCall.count;
}

glm::vec2 Renderer::getSpriteRegionUv(const SpriteRegion& region, const Texture& texutre, glm::vec2 uv) const
{
    return glm::vec2(
        (static_cast<float>(region.x) + region.width * uv.x) / texutre.width,
        (static_cast<float>(region.y) + region.height * uv.y) / texutre.height
    );
}

Texture Renderer::loadTexture(const std::string& path)
{
    std::vector<char> buffer = Engine::GetAssetManager().Load(path);
    int width, height, channels;

    unsigned char* pixels = stbi_load_from_memory(reinterpret_cast<unsigned char*>(buffer.data()), buffer.size(), &width, &height, &channels, 4);

    int internalTexID = mBackend.CreateTexture(pixels, width, height);
    Texture tex{
        internalTexID,
        width,
        height
    };

    stbi_image_free(pixels);

    return tex;
}
