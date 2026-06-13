// 
// rendering_engine.hpp
//
// Created on 30 May 2026
// 

#pragma once

#include <Metal/Metal.hpp>
// #include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.hpp>
// #include <QuartzCore/CAMetalLayer.h>
// #include <QuartzCore/QuartzCore.hpp>

#include "AAPLMathUtilities.h"

#include <simd/simd.h>
#include <filesystem>

#include "vertex_data.hpp"

class RenderingEngine {
    public:
    void init(CA::MetalLayer *mtlLayer, int width, int height);
    // void run();
    void cleanup();
    void draw();

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
    void updateRenderPassDescriptor();

    void encodeRenderCommand(MTL::RenderCommandEncoder* renderEncoder);
    void sendRenderCommand();
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

    // Transformation buffers
    MTL::Buffer* sphereTransformationBuffer;
    MTL::Buffer* lightTransformationBuffer;

    MTL::DepthStencilState* depthStencilState;
    
    MTL::RenderPassDescriptor* renderPassDescriptor;
    
    MTL::Texture* msaaRenderTargetTexture = nullptr;
    MTL::Texture* depthTexture;

    int sampleCount = 4;

    NS::UInteger vertexCount;
    NS::UInteger indexCount;
};