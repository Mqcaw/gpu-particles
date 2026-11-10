#pragma once
#include <SDL3_shadercross/SDL_shadercross.h>

namespace Shaders {
    Uint8* LoadSPIRV(const char* relPath, size_t* outSize);
    SDL_GPUShader* LoadGraphicsShader(SDL_GPUDevice* device, const char* relPath, SDL_ShaderCross_ShaderStage stage);
    SDL_GPUComputePipeline* LoadComputePipeline(SDL_GPUDevice* device, const char* relPath);
};
