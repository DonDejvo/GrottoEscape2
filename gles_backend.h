#pragma once

#include "backend.h"
#include <vector>
#include <string>

namespace gles {

    struct Buffers {
        std::vector<GLuint> layers;
        GLuint index;
        GLuint camera;

        Buffers();
    };

    struct ShaderCache {
        GLuint sprite;
    };

    class GLESBackend : public Backend {
    public:
        GLESBackend(Window* window);
        void Init() override;
        void EndFrame() override;
        void Destroy() override;
        int CreateTexture(unsigned char* pixels, int w, int h) override;
        void UploadLayerBuffer(LayerBuffer idx, const float* data, uint32_t size) override;
        void UploadCameraBuffer(const float* data) override;
        void BeginPass() override;
        void SetViewport(float x, float y, float w, float h) override;
        void SetLayerBuffer(LayerBuffer idx) override;
        void SetCamera(CameraBuffer idx) override;
        void SetTexture(int id) override;
        void Draw(uint32_t count, uint32_t offset) override;
        void EndPass() override;

    private:
        int mNextTexID;
        std::vector<GLuint> mTextureCache;
        Buffers mBuffers;
        ShaderCache mShaderCache;

        void InitBuffers();
        void InitShaderPrograms();
        GLuint CreateShaderProgram(const std::string& path);
        GLuint CompileShader(const std::string& source, GLenum type);
    };

}