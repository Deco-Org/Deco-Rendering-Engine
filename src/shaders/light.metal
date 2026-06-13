//
//  light.metal
//  Hello-World-Rendering
//
//  Created on 5/14/26.
//

#include <metal_stdlib>
using namespace metal;

#include "vertex_data.hpp"

struct LightVertexData {
    float4 position [[position]];
    float4 normal;
};

vertex LightVertexData lightVertexShader(uint vertexId [[vertex_id]], constant VertexData *vertexData, constant TransformationData *transformationData) {
    LightVertexData out;
//    out.position = float4(vertexData[vertexId].position, 1.0f);
    out.normal = float4(vertexData[vertexId].normal, 1.0f);
    
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * float4(vertexData[vertexId].position, 1.0f);
    return out;
}

fragment float4 lightFragmentShader(VertexData in [[stage_in]], constant float4 &lightColor [[buffer(0)]]) {
    return lightColor;
}
