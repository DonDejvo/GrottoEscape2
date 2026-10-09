#pragma once

#include "window.h"
#include <vector>
#include "constants.h"

class Backend {
public:
    Backend(Window* window);
    virtual void Init() = 0;
    virtual void EndFrame() = 0;
    virtual void Destroy() = 0;
    virtual int CreateTexture(unsigned char* pixels, int w, int h) = 0;
    virtual void UploadLayerBuffer(LayerBuffer idx, const float* data, uint32_t size) = 0;
    virtual void UploadCameraBuffer(const float* data) = 0;
    virtual void BeginPass() = 0;
    virtual void SetViewport(float x, float y, float w, float h) = 0;
    virtual void SetLayerBuffer(LayerBuffer idx) = 0;
    virtual void SetCamera(CameraBuffer idx) = 0;
    virtual void SetTexture(int id) = 0;
    virtual void Draw(uint32_t count, uint32_t offset) = 0;
    virtual void EndPass() = 0;
protected:
    Window* mWindow;

    std::vector<uint32_t> genIndexData() const;
};