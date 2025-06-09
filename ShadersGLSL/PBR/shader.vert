#version 460

layout (constant_id = 0) const uint MAX_BONES = 100;
layout (constant_id = 1) const uint MAX_BONE_INFLUENCE = 4;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 bitangent;
layout (location = 4) in vec2 uv;

layout (location = 5) in ivec4 boneIDS;
layout (location = 6) in vec4 weights;

layout(location = 0) out VertexData {
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec3 fragPosition;
    vec2 uv;
} vertexData;

layout(set = 0, binding = 0) uniform Camera {
    mat4 projection;
    mat4 view;
} camera;

// Dynamic Buffer
layout(set = 3, binding = 0) readonly buffer Animation {
    mat4 boneMatrices[MAX_BONES];
}animation;

layout(push_constant, std430) uniform SharedConstants {
    mat4 model;
    // The col a contains the position of the view
    vec4 col0;
    vec4 col1;
    vec4 col2;
    int lightsCount;
    float ambientStrength;
    uint hasAnimation;
} sharedConstants;

void main() {
    vec4 skinnedPosition = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);

    if(sharedConstants.hasAnimation == 1) {
        for(int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if(boneIDS[i] == -1) {
                continue;
            }
            if(boneIDS[i] >= MAX_BONES) {
                skinnedPosition = vec4(position, 1.0);
                skinnedNormal = normal;
                break;
            }

            uint boneID = boneIDS[i];
            mat4 boneMatrix = animation.boneMatrices[boneID];

            vec4 localPosition = boneMatrix * vec4(position, 1.0);
            skinnedPosition += localPosition * weights[i];

            skinnedNormal += mat3(boneMatrix) * normal;
        }
    }
    else {
        skinnedPosition = vec4(position, 1.0);
        skinnedNormal = normal;
    }

    vertexData.normal = skinnedNormal;
    vertexData.tangent = tangent;
    vertexData.bitangent = bitangent;
    vertexData.fragPosition = vec3(sharedConstants.model * skinnedPosition);
    vertexData.uv = uv;

    gl_Position = camera.projection * camera.view * sharedConstants.model * skinnedPosition;
}