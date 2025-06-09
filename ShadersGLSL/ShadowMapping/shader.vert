#version 460

layout (constant_id = 0) const uint MAX_BONES = 100;
layout (constant_id = 1) const uint MAX_BONE_INFLUENCE = 4;

layout (location = 0) in vec3 position;
layout (location = 1) in ivec4 boneIDS;
layout (location = 2) in vec4 weights;

layout(push_constant, std430) uniform SharedConstants {
    mat4 model;
    uint hasAnimation;
} sharedConstants;

layout(set = 0, binding = 0) readonly buffer Animation {
    mat4 boneMatrices[MAX_BONES];
}animation;

void main() {
    vec4 skinnedPosition = vec4(0.0);

    if(sharedConstants.hasAnimation == 1) {
        for(int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if(boneIDS[i] == -1) {
                continue;
            }
            if(boneIDS[i] >= MAX_BONES) {
                skinnedPosition = vec4(position, 1.0);
                break;
            }

            uint boneID = boneIDS[i];
            mat4 boneMatrix = animation.boneMatrices[boneID];

            vec4 localPosition = boneMatrix * vec4(position, 1.0);
            skinnedPosition += localPosition * weights[i];
        }
    }
    else {
        skinnedPosition = vec4(position, 1.0);
    }

    gl_Position = sharedConstants.model * skinnedPosition;
}
