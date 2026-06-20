//
//  sphere.metal
//  Hello-World-Rendering
//
//  Created on 5/3/26.
//

#include <metal_stdlib>
using namespace metal;

#include "vertex_data.hpp"

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
};

struct VertexOut {
    // The [[position]] attribute of this member indicates that this value
    // is the clip space position of the vertex when this structure is
    // returned from the vertex function.
    float4 position [[position]];
    float4 normal [[user(normal)]];
    float4 fragmentPosition [[user(fragpos)]];
    float2 uv [[user(uv)]];

    // Since this member does not have a special attribute, the rasterizer
    // interpolates its value with the values of the other triangle vertices
    // and then passes the interpolated value to the fragment shader for each
    // fragment in the triangle.
//    float2 textureCoordinate;
};

vertex VertexOut sphereVertexShader(
    VertexIn in [[stage_in]],
    constant TransformationData* transformationData [[buffer(1)]])
{
    VertexOut out;
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * float4(in.position, 1.0f);
    out.normal = float4(normalize((transformationData->normalMatrix * float4(in.normal, 0.0f)).xyz), 0.0f);
    out.fragmentPosition = transformationData->modelMatrix * float4(in.position, 1.0f);
    out.uv = in.uv;
    
    return out;
}

fragment float4 sphereFragmentShader(VertexOut in [[stage_in]],
                               // constant float4& sphereColor                      [[buffer(0)]],
                               texture2d<float> texture                        [[texture(0)]],
                               constant float4& lightColor                     [[buffer(1)]],
                               constant float4& lightPosition                  [[buffer(2)]],
                               constant float4& cameraPosition                 [[buffer(3)]])
{
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    float4 sphereColor = texture.sample(textureSampler, in.uv);

    // Ambient
    float ambientStrength = 0.2f;
    float4 ambient = ambientStrength * lightColor;
    
    // Diffuse
    float3 norm = normalize(in.normal.xyz);
    float3 lightDirection = normalize(lightPosition.xyz - in.fragmentPosition.xyz);
    float diff = max(dot(norm, lightDirection.xyz), 0.0);
    float4 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.75f;
    float3 viewDirection = normalize(cameraPosition.xyz - in.fragmentPosition.xyz);
    float3 halfwayDirection = normalize(lightDirection + viewDirection);
//    float spec = pow(max(dot(float4(norm, 1.0), halfwayDirection), 0.0), 32);
    float spec = pow(max(dot(norm, halfwayDirection), 0.0), 32);
    float4 specular = specularStrength * spec * lightColor;
    
    float4 finalColor = float4((ambient + diffuse + specular) * sphereColor);
    return finalColor;
}
