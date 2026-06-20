// 
// rendering_engine.cpp
//
// Created on 30 May 2026
// 

#include "rendering_engine.hpp"
#include <iostream>

void RenderingEngine::init(CA::MetalLayer *mtlLayer, int width, int height) 
{
    initDevice();
    setupLayer(mtlLayer, width, height);

    // Temporary
    Texture texture = Texture(metalDevice, MTL::PixelFormatBGRA8Unorm);
    earthTexture = texture.loadTexture("assets/climate_map.png");


    createSphere();
    createLight();

    createBuffers();
    createDefaultLibrary();
    createCommandQueue();
    createRenderPipeline();
    createLightSourceRenderPipeline();
    createDepthAndMsaaTextures();
    createRenderPassDescriptor();
}

void RenderingEngine::cleanup() {
    if (earthTexture) earthTexture->release(); // Temporary

    sphereVertexBuffer->release();
    sphereIndexBuffer->release();
    lightVertexBuffer->release();
    sphereTransformationBuffer->release();
    lightTransformationBuffer->release();
    msaaRenderTargetTexture->release();
    depthTexture->release();
    renderPassDescriptor->release();
    metalDefaultLibrary->release();
    metalCommandQueue->release();
    metalRenderPSO->release();
    metalLightSourceRenderPSO->release();
    depthStencilState->release();
    metalDevice->release(); 
}

void RenderingEngine::initDevice()
{
    metalDevice = MTL::CreateSystemDefaultDevice();
}

void RenderingEngine::setupLayer(CA::MetalLayer *mtlLayer, int width, int height)
{
    metalLayer = mtlLayer;
    metalLayer->setDevice(metalDevice);
    metalLayer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
    metalLayer->setDrawableSize(CGSizeMake(width, height));
}

void RenderingEngine::createBuffers()
{
    sphereTransformationBuffer = metalDevice->newBuffer(sizeof(TransformationData), MTL::ResourceStorageModeShared);
    lightTransformationBuffer = metalDevice->newBuffer(sizeof(TransformationData), MTL::ResourceStorageModeShared);
}

void RenderingEngine::createDefaultLibrary()
{
    metalDefaultLibrary = metalDevice->newDefaultLibrary();
    if(!metalDefaultLibrary)
    {
        std::cerr << "Failed to load default library";
        exit(-1);
    }
}

void RenderingEngine::createCommandQueue()
{
    metalCommandQueue = metalDevice->newCommandQueue();
}

void RenderingEngine::createRenderPipeline()
{
    MTL::Function *vertexShader = metalDefaultLibrary->newFunction(
        NS::String::string("sphereVertexShader", NS::UTF8StringEncoding)
    );
    assert(vertexShader);
    MTL::Function *fragmentShader = metalDefaultLibrary->newFunction(
        NS::String::string("sphereFragmentShader", NS::UTF8StringEncoding)
    );
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor *renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setLabel(NS::String::string("Sphere Rendering Pipeline", NS::UTF8StringEncoding));
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);
    
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat)metalLayer->pixelFormat();
    
    // Color attachment
    MTL::RenderPipelineColorAttachmentDescriptor* colorAttachment = renderPipelineDescriptor->colorAttachments()->object(0);
    colorAttachment->setPixelFormat(pixelFormat);
    colorAttachment->setBlendingEnabled(true);
    colorAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
    colorAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
    colorAttachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
    colorAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
    colorAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    colorAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);

    renderPipelineDescriptor->setSampleCount(sampleCount);
    renderPipelineDescriptor->setLabel(NS::String::string("Sphere Render Pipeline", NS::UTF8StringEncoding));
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    renderPipelineDescriptor->setTessellationOutputWindingOrder(MTL::WindingClockwise);

    MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    NS::UInteger currentOffset = 0;

    // Position attribute
    vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);
    vertexDescriptor->attributes()->object(0)->setOffset(0);
    vertexDescriptor->attributes()->object(0)->setBufferIndex(offsetof(VertexData, position));
    currentOffset += sizeof(simd::float3);

    // Normal attribute
    vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat3);
    vertexDescriptor->attributes()->object(1)->setOffset(offsetof(VertexData, normal));
    vertexDescriptor->attributes()->object(1)->setBufferIndex(0);
    currentOffset += sizeof(simd::float3);

    // UV (Texture Coordinate) attribute
    vertexDescriptor->attributes()->object(2)->setFormat(MTL::VertexFormatFloat2);
    vertexDescriptor->attributes()->object(2)->setOffset(offsetof(VertexData, textureCoordinate));
    vertexDescriptor->attributes()->object(2)->setBufferIndex(0);
    currentOffset += sizeof(simd::float2);

    // Setting the stride
    vertexDescriptor->layouts()->object(0)->setStride(sizeof(VertexData));

    renderPipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    vertexDescriptor->release();

    NS::Error* error;
    metalRenderPSO = metalDevice->newRenderPipelineState(renderPipelineDescriptor, &error);
    
    if (metalRenderPSO == nil) {
        std::cerr << "Error creating render pipeline state: " << error << std::endl;
        std::exit(0);
    }
    
    MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    depthStencilState = metalDevice->newDepthStencilState(depthStencilDescriptor);

    renderPipelineDescriptor->release();
    vertexShader->release();
    fragmentShader->release();
    depthStencilDescriptor->release();
}

void RenderingEngine::createLightSourceRenderPipeline()
{
    MTL::Function* vertexShader = metalDefaultLibrary->newFunction(NS::String::string("lightVertexShader", NS::UTF8StringEncoding));
    assert(vertexShader);
    MTL::Function* fragmentShader = metalDefaultLibrary->newFunction(NS::String::string("lightFragmentShader", NS::UTF8StringEncoding));
    assert(fragmentShader);
    
    MTL::RenderPipelineDescriptor *renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);
    
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat)metalLayer->pixelFormat();
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    renderPipelineDescriptor->setSampleCount(4);
    renderPipelineDescriptor->setLabel(NS::String::string("Lightsource Rendering Pipeline", NS::UTF8StringEncoding));
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    renderPipelineDescriptor->setTessellationOutputWindingOrder(MTL::WindingClockwise);
    
    NS::Error* error;
    metalLightSourceRenderPSO = metalDevice->newRenderPipelineState(renderPipelineDescriptor, &error);
    
    renderPipelineDescriptor->release();
}

void RenderingEngine::createDepthAndMsaaTextures()
{
    MTL::TextureDescriptor* msaaTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    msaaTextureDescriptor->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    msaaTextureDescriptor->setWidth(metalLayer->drawableSize().width);
    msaaTextureDescriptor->setHeight(metalLayer->drawableSize().height);
    msaaTextureDescriptor->setSampleCount(sampleCount);
    msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);

    msaaRenderTargetTexture = metalDevice->newTexture(msaaTextureDescriptor);

    MTL::TextureDescriptor* depthTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    depthTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    depthTextureDescriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthTextureDescriptor->setWidth(metalLayer->drawableSize().width);
    depthTextureDescriptor->setHeight(metalLayer->drawableSize().height);
    depthTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    depthTextureDescriptor->setSampleCount(sampleCount);

    depthTexture = metalDevice->newTexture(depthTextureDescriptor);

    msaaTextureDescriptor->release();
    depthTextureDescriptor->release();
}

void RenderingEngine::createRenderPassDescriptor()
{
    renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* colorAttachment = renderPassDescriptor->colorAttachments()->object(0);
    MTL::RenderPassDepthAttachmentDescriptor* depthAttachment = renderPassDescriptor->depthAttachment();

    colorAttachment->setTexture(msaaRenderTargetTexture);
    // colorAttachment->setResolveTexture(metalDrawable->texture());
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    // Setting the background color
    colorAttachment->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);

    depthAttachment->setTexture(depthTexture);
    depthAttachment->setLoadAction(MTL::LoadActionClear);
    depthAttachment->setStoreAction(MTL::StoreActionDontCare);
    depthAttachment->setClearDepth(1.0);
}

void RenderingEngine::updateRenderPassDescriptor(CA::MetalDrawable *drawable)
{
    renderPassDescriptor->colorAttachments()->object(0)->setTexture(msaaRenderTargetTexture);
    renderPassDescriptor->colorAttachments()->object(0)->setResolveTexture(drawable->texture());
    renderPassDescriptor->depthAttachment()->setTexture(depthTexture);
}

void RenderingEngine::draw(CA::MetalDrawable *drawable)
{
    sendRenderCommand(drawable);
}

void RenderingEngine::sendRenderCommand(CA::MetalDrawable *drawable)
{
    metalDrawable = drawable;
    metalCommandBuffer = metalCommandQueue->commandBuffer();

    updateRenderPassDescriptor(drawable);

    MTL::RenderCommandEncoder *renderCommandEncoder = metalCommandBuffer->renderCommandEncoder(renderPassDescriptor);
    encodeRenderCommand(renderCommandEncoder);
    renderCommandEncoder->endEncoding();

    metalCommandBuffer->presentDrawable(metalDrawable);
    metalCommandBuffer->commit();
    metalCommandBuffer->waitUntilCompleted();
}

void RenderingEngine::encodeRenderCommand(MTL::RenderCommandEncoder *renderCommandEncoder)
{
    // Moves the sphere one unit down the negative Z axis
    matrix_float4x4 translationMatrix = matrix4x4_translation(0.0f, 0.0f, -1.0);
    matrix_float4x4 scaleMatrix = matrix4x4_scale(0.5, 0.5, 0.5);
    
    matrix_float4x4 modelMatrix = matrix_multiply(translationMatrix, scaleMatrix);
    matrix_float4x4 normalMatrix = matrix_inverse_transpose(modelMatrix);
    
    simd::float3 R = simd::float3 { 1, 0, 0 }; // Unit-Right
    simd::float3 U = simd::float3 { 0, 1, 0 }; // Unit-Up
    simd::float3 F = simd::float3 { 0, 0, -1}; // Unit-Forward
    simd::float3 P = simd::float3 { 0, 0, 0 }; // Camera Position in world space

    matrix_float4x4 viewMatrix = matrix_make_rows(
        R[0], R[1], R[2], simd_dot(-R, P),
        U[0], U[1], U[2], simd_dot(-U, P),
        -F[0], -F[1], -F[2], simd_dot(F, P),
        0, 0, 0, 1
    );

    CGSize size = metalLayer->drawableSize();
    float aspectRatio = (size.width / size.height);
    float fov = 90 * (M_PI / 180.0f);
    float nearZ = 0.1f;
    float farZ = 100.0f;
    
    matrix_float4x4 perspectiveMatrix = matrix_perspective_right_hand(fov, aspectRatio, nearZ, farZ);
    TransformationData transformationData = { modelMatrix, viewMatrix, perspectiveMatrix, normalMatrix };
    memcpy(sphereTransformationBuffer->contents(), &transformationData, sizeof(transformationData));
    
    simd_float4 sphereColor = simd_make_float4(0.5, 0.9, 0.7, 1.0);
    simd_float4 lightColor = simd_make_float4(1.0, 1.0, 1.0, 1.0);
    simd_float4 lightPosition = simd_make_float4(-2.5, 1.5, 1.0, 1);
    simd_float4 cameraPosition = simd_make_float4(simd_make_float3(P[0], P[1], P[2]), 1.0);
    
    renderCommandEncoder->setFragmentTexture(earthTexture, 0); // Temporary
    // renderCommandEncoder->setFragmentBytes(&sphereColor, sizeof(sphereColor), 0);
    renderCommandEncoder->setFragmentBytes(&lightColor, sizeof(lightColor), 1);
    renderCommandEncoder->setFragmentBytes(&lightPosition, sizeof(lightPosition), 2);
    renderCommandEncoder->setFragmentBytes(&cameraPosition, sizeof(cameraPosition), 3);
    
    
    renderCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    renderCommandEncoder->setCullMode(MTL::CullModeBack);
    renderCommandEncoder->setRenderPipelineState(metalRenderPSO);
    renderCommandEncoder->setTriangleFillMode(MTL::TriangleFillModeFill);
    renderCommandEncoder->setDepthStencilState(depthStencilState);
    renderCommandEncoder->setVertexBuffer(sphereVertexBuffer, 0, 0);
    renderCommandEncoder->setVertexBuffer(sphereTransformationBuffer, 0, 1);
    MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;
    
    renderCommandEncoder->drawIndexedPrimitives(
        typeTriangle,
        indexCount,
        MTL::IndexTypeUInt16,
        sphereIndexBuffer,
        (NS::UInteger)0
    );
    
    // Drawing the light source
    scaleMatrix = matrix4x4_scale(0.25f, 0.25f, 0.25f);
    translationMatrix = matrix4x4_translation(
        simd_make_float3(lightPosition[0], lightPosition[1], lightPosition[2])
    );
    
    modelMatrix = simd_mul(translationMatrix, scaleMatrix);
    normalMatrix = matrix_inverse_transpose(modelMatrix);
    renderCommandEncoder->setRenderPipelineState(metalLightSourceRenderPSO);
    
    transformationData = { modelMatrix, viewMatrix, perspectiveMatrix, normalMatrix };
    memcpy(lightTransformationBuffer->contents(), &transformationData, sizeof(transformationData));
    
    renderCommandEncoder->setVertexBuffer(lightVertexBuffer, 0, 0);
    renderCommandEncoder->setVertexBuffer(lightTransformationBuffer, 0, 1);
    
    renderCommandEncoder->setFragmentBytes(&lightColor, sizeof(lightColor), 0);
    renderCommandEncoder->drawPrimitives(typeTriangle, (NS::UInteger)0, (NS::UInteger)36);
}

/**
 * Creates sphere
 */
void RenderingEngine::createSphere(int numOfLatitudeLines, int numOfLongitudeLines) {
    std::vector<VertexData> vertices;
    std::vector<uint16_t> indices;
    const float PI = 3.14159265359f;
    
    for (int lat = 0; lat < numOfLatitudeLines; ++lat) {
        for (int lon = 0; lon < numOfLongitudeLines; ++lon) {
            // Defining the corners of the square that will form the bounds of the sphere
            std::array<simd::float3, 4> squareVertices;
            std::array<simd::float3, 4> normals;
            std::array<simd::float2, 4> uv;
            
            for (int i = 0; i < 4; ++i) {
                float theta = (lat + (i / 2)) * PI / numOfLatitudeLines;
                float phi = (lon + (i % 2)) * 2 * PI / numOfLongitudeLines;
                float sinTheta = sinf(theta);
                float cosTheta = cosf(theta);
                float sinPhi = sinf(phi);
                float cosPhi = cosf(phi);

                squareVertices[i] = {cosPhi * sinTheta, cosTheta, sinPhi * sinTheta};
                
                // Normal of the vertex, same as its position on a unit sphere
                normals[i] = simd::normalize(squareVertices[i]);

                uv[i] = {
                    1.0f - (float)(lon + (i % 2)) / numOfLongitudeLines,
                    (float)(lat + (i / 2)) / numOfLatitudeLines
                };
            }
            
            vertices.push_back(VertexData{ squareVertices[0], normals[0], uv[0] });
            vertices.push_back(VertexData{ squareVertices[1], normals[1], uv[1] });
            vertices.push_back(VertexData{ squareVertices[2], normals[2], uv[2] });
            vertices.push_back(VertexData{ squareVertices[3], normals[3], uv[3] });
            
            NS::UInteger baseIndex = vertices.size() - 4;
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 3);
            indices.push_back(baseIndex + 2);
        }
    }
    
    sphereVertexBuffer = metalDevice->newBuffer(vertices.data(), sizeof(VertexData) * vertices.size(), MTL::ResourceStorageModeShared);
    sphereIndexBuffer = metalDevice->newBuffer(indices.data(), sizeof(uint16_t) * indices.size(), MTL::ResourceStorageModeShared);

    vertexCount = vertices.size();
    indexCount = indices.size();
}

void RenderingEngine::createLight() {
    // Cube for use in a right-handed coordinate system with triangle faces
    // specified with a Counter-Clockwise winding order.
    VertexData lightSource[] = {
        // Front face            // Normals
        {{-0.5,-0.5, 0.5}, {0.0, 0.0, 1.0}},
        {{ 0.5,-0.5, 0.5}, {0.0, 0.0, 1.0}},
        {{ 0.5, 0.5, 0.5}, {0.0, 0.0, 1.0}},
        {{ 0.5, 0.5, 0.5}, {0.0, 0.0, 1.0}},
        {{-0.5, 0.5, 0.5}, {0.0, 0.0, 1.0}},
        {{-0.5,-0.5, 0.5}, {0.0, 0.0, 1.0}},
        
        // Back face
        {{ 0.5,-0.5,-0.5}, {0.0, 0.0,-1.0}},
        {{-0.5,-0.5,-0.5}, {0.0, 0.0,-1.0}},
        {{-0.5, 0.5,-0.5}, {0.0, 0.0,-1.0}},
        {{-0.5, 0.5,-0.5}, {0.0, 0.0,-1.0}},
        {{ 0.5, 0.5,-0.5}, {0.0, 0.0,-1.0}},
        {{ 0.5,-0.5,-0.5}, {0.0, 0.0,-1.0}},

        // Top face
        {{-0.5, 0.5, 0.5}, {0.0, 1.0, 0.0}},
        {{ 0.5, 0.5, 0.5}, {0.0, 1.0, 0.0}},
        {{ 0.5, 0.5,-0.5}, {0.0, 1.0, 0.0}},
        {{ 0.5, 0.5,-0.5}, {0.0, 1.0, 0.0}},
        {{-0.5, 0.5,-0.5}, {0.0, 1.0, 0.0}},
        {{-0.5, 0.5, 0.5}, {0.0, 1.0, 0.0}},

        // Bottom face
        {{-0.5,-0.5,-0.5}, {0.0,-1.0, 0.0}},
        {{ 0.5,-0.5,-0.5}, {0.0,-1.0, 0.0}},
        {{ 0.5,-0.5, 0.5}, {0.0,-1.0, 0.0}},
        {{ 0.5,-0.5, 0.5}, {0.0,-1.0, 0.0}},
        {{-0.5,-0.5, 0.5}, {0.0,-1.0, 0.0}},
        {{-0.5,-0.5,-0.5}, {0.0,-1.0, 0.0}},

        // Left face
        {{-0.5,-0.5,-0.5}, {-1.0,0.0, 0.0}},
        {{-0.5,-0.5, 0.5}, {-1.0,0.0, 0.0}},
        {{-0.5, 0.5, 0.5}, {-1.0,0.0, 0.0}},
        {{-0.5, 0.5, 0.5}, {-1.0,0.0, 0.0}},
        {{-0.5, 0.5,-0.5}, {-1.0,0.0, 0.0}},
        {{-0.5,-0.5,-0.5}, {-1.0,0.0, 0.0}},

        // Right face
        {{ 0.5,-0.5, 0.5}, {1.0, 0.0, 0.0}},
        {{ 0.5,-0.5,-0.5}, {1.0, 0.0, 0.0}},
        {{ 0.5, 0.5,-0.5}, {1.0, 0.0, 0.0}},
        {{ 0.5, 0.5,-0.5}, {1.0, 0.0, 0.0}},
        {{ 0.5, 0.5, 0.5}, {1.0, 0.0, 0.0}},
        {{ 0.5,-0.5, 0.5}, {1.0, 0.0, 0.0}},
    };
    
    lightVertexBuffer = metalDevice->newBuffer(&lightSource, sizeof(lightSource), MTL::ResourceStorageModeShared);
}