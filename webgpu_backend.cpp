#include "webgpu_backend.h"

#include <iostream>
#include <assert.h>
#include <dawn/webgpu_cpp_print.h>
#include <webgpu/webgpu_glfw.h>
#include "engine.h"

namespace webgpu {

    Buffers::Buffers()
        : layers(BUFFERS_COUNT)
    {
    }

    WebGPUBackend::WebGPUBackend(Window* window)
        : Backend(window),
        mNextTexID(0),
        mTextureCache(MAX_TEXTURES),
        mFormat(wgpu::TextureFormat::Undefined)
    {
    }

    void WebGPUBackend::Init() {
        static const auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
        wgpu::InstanceDescriptor instanceDesc{
            .requiredFeatureCount = 1,
            .requiredFeatures = &kTimedWaitAny
        };
        mInstance = wgpu::CreateInstance(&instanceDesc);

        wgpu::Future f1 = mInstance.RequestAdapter(
            nullptr,
            wgpu::CallbackMode::WaitAnyOnly,
            [this](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView message) {
                if (status != wgpu::RequestAdapterStatus::Success) {
                    std::cout << "RequestAdapter: " << message << "\n";
                    exit(0);
                }
                mAdapter = std::move(a);
            });
        mInstance.WaitAny(f1, UINT64_MAX);

        wgpu::DeviceDescriptor desc{};
        desc.SetUncapturedErrorCallback(
            [](const wgpu::Device&, wgpu::ErrorType errorType, wgpu::StringView message) {
                std::cout << "Error: " << errorType << " - message: " << message << "\n";
            });

        desc.SetDeviceLostCallback(
            wgpu::CallbackMode::WaitAnyOnly,
            [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
                std::cout << "Device lost: reason " << reason << " (" << message << ")\n";
            });

        wgpu::Future f2 = mAdapter.RequestDevice(
            &desc, wgpu::CallbackMode::WaitAnyOnly,
            [this](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView message) {
                if (status != wgpu::RequestDeviceStatus::Success) {
                    std::cout << "RequestDevice: " << message << "\n";
                    exit(0);
                }
                mDevice = std::move(d);
            });
        mInstance.WaitAny(f2, UINT64_MAX);

        ConfigureSurface();

        InitLayouts();

        InitBuffers();
        InitSamplers();
        InitShaderModules();

        InitPipeline();
    }

    void WebGPUBackend::EndFrame()
    {
        mSurface.Present();
        mInstance.ProcessEvents();
    }

    void WebGPUBackend::Destroy()
    {
        mSurface.Unconfigure();
    }

    void WebGPUBackend::InitPipeline()
    {
        wgpu::VertexAttribute vertexAttribs[2]{
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 0,
                .shaderLocation = 0
            },
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 2 * sizeof(float),
                .shaderLocation = 1
            }
        };

        wgpu::VertexBufferLayout vertexBufferLayout{
            .arrayStride = SPRITE_VERTEX_STRIDE * sizeof(float),
            .attributeCount = 2,
            .attributes = vertexAttribs
        };

        wgpu::BlendState blendState{
            .color = {
                .operation = wgpu::BlendOperation::Add,
                .srcFactor = wgpu::BlendFactor::SrcAlpha,
                .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha
            }
        };

        wgpu::ColorTargetState colorTargetState{ .format = mFormat, .blend = &blendState };

        wgpu::FragmentState fragmentState{
            .module = mShaderModuleCache.sprite,
            .targetCount = 1,
            .targets = &colorTargetState
        };

        wgpu::RenderPipelineDescriptor pipelineDesc{
            .layout = mLayouts.spritePipeline,
            .vertex = {
                .module = mShaderModuleCache.sprite,
                .bufferCount = 1,
                .buffers = &vertexBufferLayout
            },
            .fragment = &fragmentState
        };

        mPipeline = mDevice.CreateRenderPipeline(&pipelineDesc);

        wgpu::BindGroupEntry uniformBindings[1]{
            {
                .binding = 0,
                .buffer = mBuffers.camera,
                .size = CAMERA_BUFFER_SIZE * sizeof(float)
            }
        };

        wgpu::BindGroupDescriptor uniformBindGroupDesc{
            .layout = mLayouts.uniformBindGroup,
            .entryCount = 1,
            .entries = uniformBindings
        };
        mBindGroups.uniform = mDevice.CreateBindGroup(&uniformBindGroupDesc);
    }

    void WebGPUBackend::InitLayouts()
    {
        wgpu::BindGroupLayoutEntry uniformBindGroupLayoutEntries[1]{
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
                .buffer = {.type = wgpu::BufferBindingType::Uniform,.hasDynamicOffset = true }
            }
        };

        wgpu::BindGroupLayoutDescriptor uniformBindGroupLayoutDesc{
            .entryCount = 1,
            .entries = uniformBindGroupLayoutEntries,

        };
        mLayouts.uniformBindGroup = mDevice.CreateBindGroupLayout(&uniformBindGroupLayoutDesc);

        wgpu::BindGroupLayoutEntry textureBindGroupLayoutEntries[2]{
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Fragment,
                .texture = {.sampleType = wgpu::TextureSampleType::Float, .viewDimension = wgpu::TextureViewDimension::e2D}
            },
            {
                .binding = 1,
                .visibility = wgpu::ShaderStage::Fragment,
                .sampler = {.type = wgpu::SamplerBindingType::Filtering}
            }
        };

        wgpu::BindGroupLayoutDescriptor textureBindGroupLayoutDesc{
            .entryCount = 2,
            .entries = textureBindGroupLayoutEntries,

        };
        mLayouts.textureBindGroup = mDevice.CreateBindGroupLayout(&textureBindGroupLayoutDesc);

        wgpu::BindGroupLayout bindGroupLayouts[2]{ mLayouts.uniformBindGroup, mLayouts.textureBindGroup };

        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{
            .bindGroupLayoutCount = 2,
            .bindGroupLayouts = bindGroupLayouts
        };

        mLayouts.spritePipeline = mDevice.CreatePipelineLayout(&pipelineLayoutDesc);
    }

    void WebGPUBackend::InitBuffers()
    {
        wgpu::BufferDescriptor indexBufferDesc{
            .usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst,
            .size = TILE_LAYER_COUNT * LAYER_MAX_TILES * 6 * sizeof(uint32_t)
        };
        mBuffers.index = mDevice.CreateBuffer(&indexBufferDesc);

        std::vector<uint32_t> indexData = genIndexData();
        mDevice.GetQueue().WriteBuffer(mBuffers.index, 0, indexData.data(), indexBufferDesc.size);

        wgpu::BufferDescriptor objectLayerBufferDesc{
            .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
            .size = MAX_SPRITES * SPRITE_STRIDE * sizeof(float)
        };
        mBuffers.layers[LayerBuffer::BUFFER_ENTITIES] = mDevice.CreateBuffer(&objectLayerBufferDesc);

        wgpu::BufferDescriptor tileLayerBufferDesc{
            .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
            .size = TILE_LAYER_COUNT * LAYER_MAX_TILES * SPRITE_STRIDE * sizeof(float)
        };
        mBuffers.layers[LayerBuffer::BUFFER_STATIC_TILES] = mDevice.CreateBuffer(&tileLayerBufferDesc);

        wgpu::BufferDescriptor dynamicTileLayerBufferDesc{
            .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
            .size = TILE_LAYER_COUNT * LAYER_MAX_TILES * SPRITE_STRIDE * sizeof(float)
        };
        mBuffers.layers[LayerBuffer::BUFFER_DYNAMIC_TILES] = mDevice.CreateBuffer(&dynamicTileLayerBufferDesc);

        wgpu::BufferDescriptor uiLayerBufferDesc{
            .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
            .size = MAX_UI_SPRITES * SPRITE_STRIDE * sizeof(float)
        };
        mBuffers.layers[LayerBuffer::BUFFER_UI] = mDevice.CreateBuffer(&uiLayerBufferDesc);

        wgpu::BufferDescriptor cameraBufferDesc{
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = CAMERA_BUFFER_SIZE * CAMERA_COUNT * sizeof(float)
        };
        mBuffers.camera = mDevice.CreateBuffer(&cameraBufferDesc);
    }

    void WebGPUBackend::ConfigureSurface() {
        mSurface = wgpu::glfw::CreateSurfaceForWindow(mInstance, mWindow->getGLFWWindow());

        wgpu::SurfaceCapabilities capabilities;
        mSurface.GetCapabilities(mAdapter, &capabilities);
        mFormat = capabilities.formats[0];

        wgpu::SurfaceConfiguration config = { .device = mDevice,.format = mFormat,.width = mWindow->getWidth(),.height = mWindow->getHeight() };
        mSurface.Configure(&config);
    }

    void WebGPUBackend::InitSamplers()
    {
        wgpu::SamplerDescriptor samplerDesc{
            .addressModeU = wgpu::AddressMode::ClampToEdge,
            .addressModeV = wgpu::AddressMode::ClampToEdge,
            .magFilter = wgpu::FilterMode::Nearest,
            .minFilter = wgpu::FilterMode::Nearest
        };
        mSpriteSampler = mDevice.CreateSampler(&samplerDesc);
    }

    void WebGPUBackend::InitShaderModules()
    {
        mShaderModuleCache.sprite = CreateShaderModule("shaders/sprite.wgsl");
    }

    int WebGPUBackend::CreateTexture(unsigned char* pixels, int w, int h) {
        assert(mNextTexID < MAX_TEXTURES);

        wgpu::TextureDescriptor textureDesc{
            .usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = { static_cast<unsigned int>(w), static_cast<unsigned int>(h), 1 },
            .format = wgpu::TextureFormat::RGBA8Unorm,
            .mipLevelCount = 1,
            .sampleCount = 1
        };

        Texture texture;
        texture.handle = mDevice.CreateTexture(&textureDesc);
        texture.view = texture.handle.CreateView();
        texture.bindGroup = CreateTextureBindGroup(texture, mSpriteSampler);

        wgpu::TexelCopyTextureInfo destination{
            .texture = texture.handle,
            .mipLevel = 0
        };

        wgpu::TexelCopyBufferLayout layout{
            .bytesPerRow = 4 * textureDesc.size.width,
            .rowsPerImage = textureDesc.size.height
        };

        mDevice.GetQueue().WriteTexture(&destination, pixels, static_cast<size_t>(w) * h * 4, &layout, &textureDesc.size);

        int texID = mNextTexID++;
        mTextureCache[texID] = std::move(texture);

        return texID;
    }

    void WebGPUBackend::UploadLayerBuffer(LayerBuffer idx, const float* data, uint32_t size)
    {
        mDevice.GetQueue().WriteBuffer(mBuffers.layers[idx], 0, data, size * sizeof(float));
    }

    void WebGPUBackend::UploadCameraBuffer(const float* data)
    {
        mDevice.GetQueue().WriteBuffer(mBuffers.camera, 0, data, CAMERA_BUFFER_SIZE * CAMERA_COUNT * sizeof(float));
    }

    void WebGPUBackend::BeginPass()
    {
        wgpu::SurfaceTexture surfaceTexture;
        mSurface.GetCurrentTexture(&surfaceTexture);

        wgpu::RenderPassColorAttachment attachment{
            .view = surfaceTexture.texture.CreateView(),
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = {0.0, 0.0, 0.0, 1.0}
        };

        wgpu::RenderPassDescriptor renderpass{
            .colorAttachmentCount = 1,
            .colorAttachments = &attachment
        };

        mEncoder = mDevice.CreateCommandEncoder();
        mPass = mEncoder.BeginRenderPass(&renderpass);

        mPass.SetPipeline(mPipeline);
        mPass.SetIndexBuffer(mBuffers.index, wgpu::IndexFormat::Uint32);
    }

    void WebGPUBackend::SetViewport(float x, float y, float w, float h)
    {
        mPass.SetViewport(x, mWindow->getHeight() - y - h, w, h, 0.0f, 1.0f);
    }

    void WebGPUBackend::SetLayerBuffer(LayerBuffer idx)
    {
        mPass.SetVertexBuffer(0, mBuffers.layers[idx]);
    }

    void WebGPUBackend::SetCamera(CameraBuffer idx)
    {
        uint32_t offset = idx * CAMERA_BUFFER_SIZE * sizeof(float);
        mPass.SetBindGroup(0, mBindGroups.uniform, 1, &offset);
    }

    void WebGPUBackend::SetTexture(int id)
    {
        assert(id < mNextTexID);
        mPass.SetBindGroup(1, mTextureCache[id].bindGroup);
    }

    void WebGPUBackend::Draw(uint32_t count, uint32_t offset)
    {
        mPass.DrawIndexed(count * 6, 1, offset * 6);
    }

    void WebGPUBackend::EndPass()
    {
        mPass.End();

        wgpu::CommandBuffer commands = mEncoder.Finish();
        mDevice.GetQueue().Submit(1, &commands);
    }

    wgpu::ShaderModule WebGPUBackend::CreateShaderModule(const std::string& path) const
    {
        std::vector<char> buffer = Engine::GetAssetManager().Load(path);

        wgpu::ShaderModuleDescriptor shaderDesc;
        wgpu::ShaderSourceWGSL shaderSource;
        shaderSource.code = wgpu::StringView(buffer.data(), buffer.size());
        shaderDesc.nextInChain = &shaderSource;
        wgpu::ShaderModule shaderModule = mDevice.CreateShaderModule(&shaderDesc);

        return shaderModule;
    }

    wgpu::BindGroup WebGPUBackend::CreateTextureBindGroup(const Texture& texture, const wgpu::Sampler& sampler) const
    {
        wgpu::BindGroupEntry textureBindings[2]{
            {
                .binding = 0,
                .textureView = texture.view
            },
            {
                .binding = 1,
                .sampler = sampler
            }
        };

        wgpu::BindGroupDescriptor textureBindGroupDesc{
            .layout = mLayouts.textureBindGroup,
            .entryCount = 2,
            .entries = textureBindings
        };
        return mDevice.CreateBindGroup(&textureBindGroupDesc);
    }
}