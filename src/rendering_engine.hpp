// 
// rendering_engine.hpp
//
// Created on 30 May 2026
// 

#pragma once

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include "utils/AAPLMathUtilities.h"

#include <simd/simd.h>
#include <filesystem>

#include "vertex_data.hpp"
#include "texture/texture.hpp"
#include "mesh/mesh.hpp"
// #include "rendering_engine_api.h"

typedef uint16_t RenderableHandle;

class RenderingEngine {
    public:
    void init(CA::MetalLayer *mtlLayer, int width, int height);
    void cleanup();
    void draw(CA::MetalDrawable *drawable);

    // These should be moved to another class
    const RenderableHandle loadModel(const char* path);
    void unloadModel(const RenderableHandle handle);

    private:
    void initDevice();
    void setupLayer(CA::MetalLayer *mtlLayer, int width, int height);

    void createSphere(int numOfLatitudeLines = 34, int numOfLongitudeLines = 34);
    void createLight();

    void createBuffers();
    void createDefaultLibrary();
    void createCommandQueue();

    void createRenderPipeline();
    void createLightSourceRenderPipeline();

    void createDepthAndMsaaTextures();
    void createRenderPassDescriptor();
    void updateRenderPassDescriptor(CA::MetalDrawable *drawable);

    void encodeRenderCommand(MTL::RenderCommandEncoder* renderEncoder);
    void sendRenderCommand(CA::MetalDrawable *drawable);
    // void draw();

    MTL::Device *metalDevice;
    CA::MetalLayer *metalLayer;
    CA::MetalDrawable *metalDrawable;

    MTL::Library* metalDefaultLibrary;
    MTL::CommandQueue* metalCommandQueue;
    MTL::CommandBuffer* metalCommandBuffer;
    MTL::RenderPipelineState* metalRenderPSO;
    MTL::RenderPipelineState* metalLightSourceRenderPSO;

    // Vertex and index buffers
    MTL::Buffer* sphereVertexBuffer;
    MTL::Buffer* sphereIndexBuffer;
    MTL::Buffer* lightVertexBuffer;
    MeshAsset* meshAsset;

    // Transformation buffers
    MTL::Buffer* sphereTransformationBuffer;
    MTL::Buffer* lightTransformationBuffer;
    MTL::Buffer* modelTransformationBuffer;

    MTL::DepthStencilState* depthStencilState;
    
    MTL::RenderPassDescriptor* renderPassDescriptor;
    
    MTL::Texture* msaaRenderTargetTexture = nullptr;
    MTL::Texture* depthTexture;

    int sampleCount = 4;

    NS::UInteger vertexCount;
    NS::UInteger indexCount;

    RenderableHandle currentRenderableHandle;
    
    MTL::Texture* earthTexture; // Temporary
    MTL::Texture* cubeTexture; // Temporary
};