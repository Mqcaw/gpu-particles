#include "App.h"

#include <array>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include "Shaders.h"

const std::array preferredGpuDrivers{
    std::string{"vulkan"},
    std::string{"direct3d12"},
};

static constexpr Uint32 kParticleCount = 100'000;

App::App(int argc, char **argv): m_window(nullptr, &SDL_DestroyWindow), m_gpuDevice(nullptr, &SDL_DestroyGPUDevice) {

}

SDL_AppResult App::Init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    m_window.reset(SDL_CreateWindow("Gpu Particles", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN));
    if (!m_window) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    std::vector<std::string> gpuDrivers;
    auto gpuDriverCount = SDL_GetNumGPUDrivers();
    gpuDrivers.reserve(gpuDriverCount);
    for ( int i = 0; i < gpuDriverCount; i++)
        gpuDrivers.emplace_back(SDL_GetGPUDriver(i));

    SDL_Log("Supported drivers:");
    for (auto const &supportedGpuDriver: gpuDrivers) {
        SDL_Log("    %s", supportedGpuDriver.c_str());
    }

    std::string preferredGpuDriver;
    for (const auto &gpuDriver: preferredGpuDrivers) {
        if (std::ranges::find(gpuDrivers, gpuDriver) != gpuDrivers.end()) {
            preferredGpuDriver = gpuDriver;
            SDL_Log("Using preferred GPU driver: %s", preferredGpuDriver.c_str());
            break;
        }
    }

    m_gpuDevice.reset(SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true,
        preferredGpuDriver.empty() ? nullptr : preferredGpuDriver.c_str()));

    if (!m_gpuDevice) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUDevice* gpu_device = m_gpuDevice.get();

    SDL_Log("GPU device created: %s", SDL_GetGPUDeviceDriver(gpu_device));

    if (!SDL_ClaimWindowForGPUDevice(gpu_device, m_window.get())) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to claim window for GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
    if (SDL_WindowSupportsGPUPresentMode(gpu_device, m_window.get(), SDL_GPU_PRESENTMODE_MAILBOX))
        presentMode = SDL_GPU_PRESENTMODE_MAILBOX;
    
    SDL_SetGPUSwapchainParameters(gpu_device, m_window.get(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR, presentMode);

    if (!SDL_ShowWindow(m_window.get())) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to show window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_ShaderCross_Init()) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to init shadercross: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    m_computePipeline = Shaders::LoadComputePipeline(gpu_device, "shaders/particles.comp.spv");
    SDL_GPUShader* vs = Shaders::LoadGraphicsShader(gpu_device, "shaders/particles.vert.spv", SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    SDL_GPUShader* fs = Shaders::LoadGraphicsShader(gpu_device, "shaders/particles.frag.spv", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);

    SDL_GPUColorTargetDescription colorTarget{
        .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, m_window.get()),
        .blend_state = {
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .color_blend_op        = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .alpha_blend_op        = SDL_GPU_BLENDOP_ADD,
            .enable_blend          = true,
        },
    };
    SDL_GPUGraphicsPipelineCreateInfo pipeInfo{
        .vertex_shader   = vs,
        .fragment_shader = fs,
        .primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info     = { .color_target_descriptions = &colorTarget, .num_color_targets = 1 },
    };
    m_renderPipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeInfo);
    SDL_ReleaseGPUShader(gpu_device, vs);
    SDL_ReleaseGPUShader(gpu_device, fs);

    const Uint32 bufferSize = sizeof(Particle) * kParticleCount;
    SDL_GPUBufferCreateInfo bufInfo{
        .usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ
               | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE
               | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = bufferSize,
    };
    m_particleBuffer = SDL_CreateGPUBuffer(gpu_device, &bufInfo);

    SDL_GPUTransferBufferCreateInfo tInfo{ .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = bufferSize };
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(gpu_device, &tInfo);

    auto* particles = static_cast<Particle*>(SDL_MapGPUTransferBuffer(gpu_device, transfer, false));
    for (Uint32 i = 0; i < kParticleCount; ++i) {
        const float rand = SDL_randf();
        particles[i] = Particle{
            .position = { SDL_randf() * 2.0f - 1.0f, SDL_randf() * 2.0f - 1.0f },
            .velocity = { (SDL_randf() - 0.5f) * 0.5f, (SDL_randf() - 0.5f) * 0.5f },
            .color    = { 0.2f + 0.3f * rand, 0.2f + 0.3f * rand, 0.2f + 0.3f * rand, 1.0f },
        };
    }
    SDL_UnmapGPUTransferBuffer(gpu_device, transfer);

    SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(gpu_device);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(uploadCmd);
    SDL_GPUTransferBufferLocation src{ .transfer_buffer = transfer, .offset = 0 };
    SDL_GPUBufferRegion dst{ .buffer = m_particleBuffer, .offset = 0, .size = bufferSize };
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(uploadCmd);
    SDL_ReleaseGPUTransferBuffer(gpu_device, transfer);

    m_lastTicks = SDL_GetTicksNS();



    return SDL_APP_CONTINUE;
}

SDL_AppResult App::Iterate() {
    if (const auto result = OnUpdate(); result != SDL_APP_CONTINUE)
        return result;
    if (const auto result = OnRender(); result != SDL_APP_CONTINUE)
        return result;
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::Event(const SDL_Event *event) {
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return OnQuit();
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (SDL_GetWindowID(m_window.get()) == event->window.windowID) {
                return OnQuit();
            }
        default: return SDL_APP_CONTINUE;
    }
}

void App::Quit(SDL_AppResult result) {
    SDL_GPUDevice* device = m_gpuDevice.get();
    if (device) {
        SDL_WaitForGPUIdle(device);
        SDL_ReleaseGPUGraphicsPipeline(device, m_renderPipeline);
        SDL_ReleaseGPUComputePipeline(device, m_computePipeline);
        SDL_ReleaseGPUBuffer(device, m_particleBuffer);
        SDL_ReleaseWindowFromGPUDevice(device, m_window.get());
    }
    SDL_ShaderCross_Quit();
}

SDL_AppResult App::OnQuit() {
    return SDL_APP_SUCCESS;
}

SDL_AppResult App::OnRender() {
    const Uint64 now = SDL_GetTicksNS();
    const float dt = std::min((now - m_lastTicks) / 1e9f, 1.0f / 30.0f); // clamp so dragging the window doesn't make particles jump
    m_lastTicks = now;
    m_time += dt;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice.get());

    // ---------- Compute pass: simulate
    SimParams sim{ dt, m_time, kParticleCount, 0.0f };
    SDL_PushGPUComputeUniformData(cmd, 0, &sim, sizeof(sim));

    SDL_GPUStorageBufferReadWriteBinding rw{ .buffer = m_particleBuffer, .cycle = false }; // false: we need last frame's data
    SDL_GPUComputePass* cp = SDL_BeginGPUComputePass(cmd, nullptr, 0, &rw, 1);
    SDL_BindGPUComputePipeline(cp, m_computePipeline);
    SDL_DispatchGPUCompute(cp, (kParticleCount + 255) / 256, 1, 1);   // 256 = numthreads.x
    SDL_EndGPUComputePass(cp);

    // ---------- Render pass: draw
    SDL_GPUTexture* swapchainTexture{};
    Uint32 w = 0, h = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, m_window.get(), &swapchainTexture, &w, &h)) {
        SDL_CancelGPUCommandBuffer(cmd);
        return SDL_APP_FAILURE;
    }

    if (swapchainTexture) {
        SDL_GPUColorTargetInfo color{
            .texture     = swapchainTexture,
            .clear_color = { 0.0f, 0.0f, 0.0f, 1.0f },
            .load_op     = SDL_GPU_LOADOP_CLEAR,
            .store_op    = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPURenderPass* rp = SDL_BeginGPURenderPass(cmd, &color, 1, nullptr);

        SDL_BindGPUGraphicsPipeline(rp, m_renderPipeline);
        SDL_BindGPUVertexStorageBuffers(rp, 0, &m_particleBuffer, 1);

        ViewParams view{ { float(h) / float(w), 1.0f }, 0.005f, 0.0f };
        SDL_PushGPUVertexUniformData(cmd, 0, &view, sizeof(view));

        SDL_DrawGPUPrimitives(rp, 6, kParticleCount, 0, 0);   // 6 verts per quad, 1 instance per particle
        SDL_EndGPURenderPass(rp);
    }

    if (!SDL_SubmitGPUCommandBuffer(cmd)) { /* log */ return SDL_APP_FAILURE; }
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::OnUpdate() {
    return SDL_APP_CONTINUE;
}
