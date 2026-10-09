#pragma once

#include "backend.h"

#include <webgpu/webgpu_cpp.h>
#include <string>
#include <array>
#include <vector>

namespace webgpu {

    struct Texture {
        wgpu::Texture handle;
        wgpu::TextureView view;
        wgpu::BindGroup bindGroup;
    };

    struct ShaderCache {
        wgpu::ShaderModule sprite;
    };

    struct Layouts {
        wgpu::BindGroupLayout uniformBindGroup;
        wgpu::BindGroupLayout textureBindGroup;
        wgpu::PipelineLayout spritePipeline;
    };

    struct Buffers {
        std::vector<wgpu::Buffer> layers;
        wgpu::Buffer index;
        wgpu::Buffer camera;

        Buffers();
    };

    struct BindGroups {
        wgpu::BindGroup uniform;
    };

    class WebGPUBackend : public Backend {
    public:
        WebGPUBackend(Window* window);
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
        wgpu::Instance mInstance;
        wgpu::Adapter mAdapter;
        wgpu::Device mDevice;
        wgpu::Surface mSurface;
        wgpu::TextureFormat mFormat;
        Layouts mLayouts;
        wgpu::RenderPipeline mPipeline;
        Buffers mBuffers;
        wgpu::Sampler mSpriteSampler;
        BindGroups mBindGroups;
        int mNextTexID;
        std::vector<Texture> mTextureCache;
        ShaderCache mShaderModuleCache;
        wgpu::CommandEncoder mEncoder;
        wgpu::RenderPassEncoder mPass;

        void InitLayouts();
        void InitBuffers();
        void InitSamplers();
        void InitShaderModules();
        void ConfigureSurface();
        void InitPipeline();
        wgpu::ShaderModule CreateShaderModule(const std::string& path) const;
        wgpu::BindGroup CreateTextureBindGroup(const Texture& texture, const wgpu::Sampler& sampler) const;
    };
}