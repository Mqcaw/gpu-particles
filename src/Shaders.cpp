#include "Shaders.h"

#include <SDL3_shadercross/SDL_shadercross.h>
#include <string>

namespace Shaders {
    Uint8* LoadSPIRV(const char* relPath, size_t* outSize) {
        const std::string path = std::string{SDL_GetBasePath()} + relPath;
        auto* code = static_cast<Uint8*>(SDL_LoadFile(path.c_str(), outSize));
        if (!code) SDL_Log("Failed to load %s: %s", path.c_str(), SDL_GetError());
        return code;
    }

    SDL_GPUShader* LoadGraphicsShader(SDL_GPUDevice* device, const char* relPath, SDL_ShaderCross_ShaderStage stage) {
        size_t size = 0;
        Uint8* spirv = LoadSPIRV(relPath, &size);
        if (!spirv) return nullptr;

        // Reads the resource counts (storage buffers, uniform buffers, ...) from the bytecode
        SDL_ShaderCross_GraphicsShaderMetadata* meta = SDL_ShaderCross_ReflectGraphicsSPIRV(spirv, size, 0);

        const SDL_ShaderCross_SPIRV_Info info{
            .bytecode = spirv, .bytecode_size = size,
            .entrypoint = "main", .shader_stage = stage, .props = 0,
        };
        // Converts SPIR-V to whatever the device needs (SPIR-V on Vulkan, DXIL on D3D12, ...)
        SDL_GPUShader* shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device, &info, &meta->resource_info, 0);
        if (!shader) SDL_Log("Failed to create shader %s: %s", relPath, SDL_GetError());

        SDL_free(meta);
        SDL_free(spirv);
        return shader;
    }

    SDL_GPUComputePipeline* LoadComputePipeline(SDL_GPUDevice* device, const char* relPath) {
        size_t size = 0;
        Uint8* spirv = LoadSPIRV(relPath, &size);
        if (!spirv) return nullptr;

        SDL_ShaderCross_ComputePipelineMetadata* meta = SDL_ShaderCross_ReflectComputeSPIRV(spirv, size, 0);

        const SDL_ShaderCross_SPIRV_Info info{
            .bytecode = spirv, .bytecode_size = size,
            .entrypoint = "main", .shader_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE, .props = 0,
        };
        SDL_GPUComputePipeline* pipeline = SDL_ShaderCross_CompileComputePipelineFromSPIRV(device, &info, meta, 0);
        if (!pipeline) SDL_Log("Failed to create compute pipeline %s: %s", relPath, SDL_GetError());

        SDL_free(meta);
        SDL_free(spirv);
        return pipeline;
    }
}