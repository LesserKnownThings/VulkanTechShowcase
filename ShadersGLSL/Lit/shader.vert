#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inBitangent;
layout (location = 4) in vec2 inUV;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 tangent;
layout(location = 3) out vec3 bitangent;

layout(set = 0, binding = 0) uniform Camera {
    mat4 projection;
    mat4 view;
} camera;

layout(push_constant) uniform Constants {
    mat4 model;
} constants;

void main() {
    gl_Position = camera.projection * camera.view * constants.model * vec4(position, 1.0);
    uv = inUV;
}