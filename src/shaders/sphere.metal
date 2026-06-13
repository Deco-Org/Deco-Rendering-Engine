//
//  sphere.metal
//  Hello-World-Rendering
//
//  Created on 5/3/26.
//

#include <metal_stdlib>
using namespace metal;

#include "vertex_data.hpp"

struct VertexOut {
    // The [[position]] attribute of this member indicates that this value
    // is the clip space position of the vertex when this structure is
    // returned from the vertex function.
    float4 position [[position]];
    float4 normal;
    float4 fragmentPosition;

    // Since this member does not have a special attribute, the rasterizer
    // interpolates its value with the values of the other triangle vertices
    // and then passes the interpolated value to the fragment shader for each
    // fragment in the triangle.
//    float2 textureCoordinate;
};

vertex VertexOut sphereVertexShader(
                                    uint vertexId [[vertex_id]],
                                    constant VertexData* vertexData                 [[buffer(0)]],
                                    constant TransformationData* transformationData [[buffer(1)]])
{
    VertexOut out;
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * float4(vertexData[vertexId].position, 1.0f);
//    out.normal = float4(vertexData[vertexId].normal, 1.0f);
    out.normal = float4(normalize((transformationData->normalMatrix * float4(vertexData[vertexId].normal, 0.0f)).xyz), 0.0f);
    out.fragmentPosition = transformationData->modelMatrix * float4(vertexData[vertexId].position, 1.0f);
    
    return out;
}

fragment float4 sphereFragmentShader(VertexOut in [[stage_in]],
                               constant float4& sphereColor                      [[buffer(0)]],
                               constant float4& lightColor                     [[buffer(1)]],
                               constant float4& lightPosition                  [[buffer(2)]],
                               constant float4& cameraPosition                 [[buffer(3)]])
{
    // Ambient
    float ambientStrength = 0.2f;
    float4 ambient = ambientStrength * lightColor;
    
    // Diffuse
    float3 norm = normalize(in.normal.xyz);
    float3 lightDirection = normalize(lightPosition.xyz - in.fragmentPosition.xyz);
    float diff = max(dot(norm, lightDirection.xyz), 0.0);
    float4 diffuse = diff * lightColor;
    
    float specularStrength = 1.0f;
    float3 viewDirection = normalize(cameraPosition.xyz - in.fragmentPosition.xyz);
    float3 halfwayDirection = normalize(lightDirection + viewDirection);
//    float spec = pow(max(dot(float4(norm, 1.0), halfwayDirection), 0.0), 32);
    float spec = pow(max(dot(norm, halfwayDirection), 0.0), 32);
    float4 specular = specularStrength * spec * lightColor;
    
    float4 finalColor = float4((ambient + diffuse + specular) * sphereColor);
    return finalColor;
}
