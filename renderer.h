#pragma once

#include "window.h"
#include "game_state.h"
#include <string>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/ext.hpp>
#if defined(BACKEND_GLES)
#include "gles_backend.h"
#else
#include "webgpu_backend.h"
#endif
#include "constants.h"
#include "text_analyzer.h"

enum RenderLayerIndex {
    RENDER_LAYER_BACKGROUND_STATIC,
    RENDER_LAYER_MIDGROUND_STATIC,
    RENDER_LAYER_FOREGROUND_STATIC,
    RENDER_LAYER_BACKGROUND_DYNAMIC,
    RENDER_LAYER_MIDGROUND_DYNAMIC,
    RENDER_LAYER_FOREGROUND_DYNAMIC,
    RENDER_LAYER_ENTITIES_PROJECTILES,
    RENDER_LAYER_ENTITIES_ITEMS,
    RENDER_LAYER_ENTITIES_FIXTURES,
    RENDER_LAYER_ENTITIES_PLATFORMS,
    RENDER_LAYER_ENTITIES_MOVERS,
    RENDER_LAYER_ENTITIES_FX,
    RENDER_LAYER_LEVEL_UI,
    RENDER_LAYER_UI,
    RENDER_LAYERS_COUNT
};

enum class TextAlign {
    Left,
    Center,
    Right
};

struct Texture {
    int id = 0;
    int width = 0;
    int height = 0;
};

struct Camera {
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec3 color;

    Camera();
};

struct DrawCall {
    int count = 0;
    int offset = 0;
    int textureID = 0;
};

struct RenderLayer {
    float* data;
    int offset;
    int drawCallsCount;
    std::vector<DrawCall> drawCalls;

    RenderLayer();
};

class Renderer {
public:
    Renderer(Window* window);
    void Init();
    void LoadLevel(const Level& level);
    void Render(const GameState& game);
    void EndFrame();
    void Destroy();

private:
    Window* mWindow;
#if defined(BACKEND_GLES)
    gles::GLESBackend mBackend;
#else
    webgpu::WebGPUBackend mBackend;
#endif
    std::vector<RenderLayer> mRenderLayers;
    std::vector<std::vector<float>> mLayersData;
    std::vector<float> mCameraData;
    std::vector<Camera> mCameras;
    std::vector<Texture> mTextureCache;

    void initTextures();

    void updateCamera();
    void setCamera(CameraBuffer idx);
    void addTiles(const Level& level);
    void addEntities(const Level& level);
    void addLevelUI(const Level& level);
    void addMenuUI(const GameState& game, float scale);
    void addMenu(const Menu& menu, float scale);
    void addPassword(const PasswordInput& input, float scale);
    void addDialog(RenderLayerIndex layerIdx, const Dialog& dialog, float scale);
    void addInventory(const Level& level, int playerIdx);
    glm::vec2 getPlayerHeaderOffset(int playerIdx) const;
    glm::vec2 getInventoryPosition(int playerIdx) const;
    void addHitpoints(const Player& player, const glm::vec2& position);
    void drawTileLayer(TileLayerIndex tileLayerIdx);
    void drawEntites();
    void drawFX();
    void drawUI();
    void clearLayer(RenderLayerIndex layerIdx);
    void drawLayer(RenderLayerIndex layerIdx);
    void addText(RenderLayerIndex layerIdx, const TextAnalyzer& analyzer, const glm::vec2& position, TextAlign textAlign, float scale);
    void addText(RenderLayerIndex layerIdx, const std::string& text, const glm::vec2& position, TextAlign textAlign, float scale, float maxWidth);
    void addTile(RenderLayerIndex layerIdx, const Spritesheet& tileset, const TileLayer& tileLayer, int x, int y, int tileWidth, int tileHeight);
    void addEntity(RenderLayerIndex layerIdx, const Entity& entity, const SpriteRegion& region);
    void addAnimatedEntity(RenderLayerIndex layerIdx, const Entity& entity);
    void addSprite(RenderLayerIndex layerIdx, const glm::vec2& position, const glm::vec2& scale, const SpriteRegion& region, int textureID);
    glm::vec2 getSpriteRegionUv(const SpriteRegion& region, const Texture& texture, glm::vec2 uv) const;
    Texture loadTexture(const std::string& path);
};