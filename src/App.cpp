#include "App.h"

#include <array>
#include <vector>
#include <SDL3/SDL.h>

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

    const std::array preferredGpuDrivers{
        std::string{"vulkan"},
        std::string{"direct3d12"},
    };

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

    SDL_Log("GPU device created: %s", SDL_GetGPUDeviceDriver(m_gpuDevice.get()));

    if (!SDL_ClaimWindowForGPUDevice(m_gpuDevice.get(), m_window.get())) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to claim window for GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
    if (SDL_WindowSupportsGPUPresentMode(m_gpuDevice.get(), m_window.get(), SDL_GPU_PRESENTMODE_MAILBOX))
        presentMode = SDL_GPU_PRESENTMODE_MAILBOX;
    
    SDL_SetGPUSwapchainParameters(m_gpuDevice.get(), m_window.get(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR, presentMode);

    if (!SDL_ShowWindow(m_window.get())) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to show window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

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
    SDL_WaitForGPUIdle(m_gpuDevice.get());
    SDL_ReleaseWindowFromGPUDevice(m_gpuDevice.get(), m_window.get());
}

SDL_AppResult App::OnQuit() {
    return SDL_APP_SUCCESS;
}

SDL_AppResult App::OnRender() {
    auto commandBuffer = SDL_AcquireGPUCommandBuffer(m_gpuDevice.get());

    SDL_GPUTexture *swapchainTexture{};
    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, m_window.get(), &swapchainTexture, nullptr, nullptr)) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to acquire swapchain texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }


    if (swapchainTexture) {
        std::array colorTargetInfos{
            SDL_GPUColorTargetInfo{
                .texture = swapchainTexture,
                .clear_color = (SDL_FColor){1.0f, 0.0f, 0.0f, 1.0f},
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
            }
        };
        const auto renderPass = SDL_BeginGPURenderPass(commandBuffer, colorTargetInfos.data(), colorTargetInfos.size(), nullptr);

        SDL_EndGPURenderPass(renderPass);
    }

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        SDL_LogError(APP_LOG_CATEGORY_GENERIC, "Failed to submit command buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult App::OnUpdate() {
    return SDL_APP_CONTINUE;
}
