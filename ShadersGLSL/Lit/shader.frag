#version 450

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;

// layout(set = 1, binding = 0) uniform sampler2D uAlbedoMap;
// layout(set = 1, binding = 1) uniform sampler2D uNormalMap;
// layout(set = 1, binding = 2) uniform sampler2D uORMMap;
// layout(set = 1, binding = 3) uniform sampler2D uEmissiveMap;

layout(set = 0, binding = 1) uniform Light {
    vec4 color;
    vec3 pos;
} light;

void main() {
    //fragColor = texture(uAlbedoMap, uv);
    fragColor = vec4(1.0);
}