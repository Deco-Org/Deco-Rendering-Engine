#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <stdio.h>
#include <filesystem>
#include "mesh/mesh.hpp"

static const MeshData cubeMeshData = {
    .vertices = std::vector<VertexData> {
        (VertexData){{1, 1, 1}, {0, 0, 1}, {0.5, ((float)2/3)}},
        {{-1, 1, 1}, {0, 0, 1}, {0.5, 1}},
        {{-1, -1, 1}, {0, 0, 1}, {0.25, 1}},
        {{1, -1, 1}, {0, 0, 1}, {0.25, ((float)2/3)}},
        {{1, -1, -1}, {0, -1, 0}, {0.25, ((float)1/3)}},
        {{1, -1, 1}, {0, -1, 0}, {0.25, ((float)2/3)}},
        {{-1, -1, 1}, {0, -1, 0}, {0, ((float)2/3)}},
        {{-1, -1, -1}, {0, -1, 0}, {0, ((float)1/3)}},
        {{-1, -1, -1}, {-1, 0, 0}, {1, ((float)1/3)}},
        {{-1, -1, 1}, {-1, 0, 0}, {1, ((float)2/3)}},
        {{-1, 1, 1}, {-1, 0, 0}, {0.75, ((float)2/3)}},
        {{-1, 1, -1}, {-1, 0, 0}, {0.75, ((float)1/3)}},
        {{-1, 1, -1}, {0, 0, -1}, {0.5, 0}},
        {{1, 1, -1}, {0, 0, -1}, {0.5, ((float)1/3)}},
        {{1, -1, -1}, {0, 0, -1}, {0.25, ((float)1/3)}},
        {{-1, -1, -1}, {0, 0, -1}, {0.25, 0}},
        {{1, 1, -1}, {1, 0, 0}, {0.5, ((float)1/3)}},
        {{1, 1, 1}, {1, 0, 0}, {0.5, ((float)2/3)}},
        {{1, -1, 1}, {1, 0, 0}, {0.25, ((float)2/3)}},
        {{1, -1, -1}, {1, 0, 0}, {0.25, ((float)1/3)}},
        {{-1, 1, -1}, {0, 1, 0}, {0.75, ((float)1/3)}},
        {{-1, 1, 1}, {0, 1, 0}, {0.75, ((float)2/3)}},
        {{1, 1, 1}, {0, 1, 0}, {0.5, ((float)2/3)}},
        {{1, 1, -1}, {0, 1, 0}, {0.5, ((float)1/3)}},
    },
    .indices = { 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 8, 9, 10, 10, 11, 8, 12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20 }
};

void REQUIRE_FLOAT2_FLOAT2_NEAR(
    const simd::float2& actual,
    const simd::float2& expected,
    double margin = 1e-6
)
{
    REQUIRE_THAT(actual[0], Catch::Matchers::WithinAbs(expected[0], margin));
    REQUIRE_THAT(actual[1], Catch::Matchers::WithinAbs(expected[1], margin));
}

void REQUIRE_FLOAT3_NEAR(
    const simd::float3& actual,
    const simd::float3& expected,
    double margin = 1e-6
)
{
    REQUIRE_THAT(actual[0], Catch::Matchers::WithinAbs(expected[0], margin));
    REQUIRE_THAT(actual[1], Catch::Matchers::WithinAbs(expected[1], margin));
    REQUIRE_THAT(actual[2], Catch::Matchers::WithinAbs(expected[2], margin));
}

void REQUIRE_MESHDATA_NEAR(
    const MeshData& actual,
    const MeshData& expected,
    double margin = 1e-6)
    {
        // Comparing the vertex data members
        const std::vector<VertexData> actualVertexData = actual.vertices;
        const std::vector<VertexData> expectedVertexData = expected.vertices;
        REQUIRE(actualVertexData.size() == expectedVertexData.size());

        for (size_t i = 0; i < actualVertexData.size(); ++i)
        {
            const simd::float3 actualPosition = actualVertexData[i].position;
            const simd::float3 actualNormal = actualVertexData[i].normal;
            const simd::float2 actualTextureCoordinate = actualVertexData[i].textureCoordinate;

            const simd::float3 expectedPosition = expectedVertexData[i].position;
            const simd::float3 expectedNormal = expectedVertexData[i].normal;
            const simd::float2 expectedTextureCoordinate = expectedVertexData[i].textureCoordinate;

            REQUIRE_FLOAT3_NEAR(actualPosition, expectedPosition);
            REQUIRE_FLOAT3_NEAR(actualNormal, expectedNormal);
            REQUIRE_FLOAT2_FLOAT2_NEAR(actualTextureCoordinate, expectedTextureCoordinate);
        }
    }

TEST_CASE(" assets directory should exist ")
{
    std::filesystem::path directoryPath = "assets/";
    REQUIRE(std::filesystem::exists(directoryPath));

}

TEST_CASE(" models can be loaded ")
{
    SECTION( " an individual mesh can be loaded ")
    {
        MeshData *meshData = Mesh::loadModel("assets/test_cube.fbx");

        REQUIRE_MESHDATA_NEAR(*meshData, cubeMeshData);

        delete meshData;
        meshData = nullptr;
    }
}