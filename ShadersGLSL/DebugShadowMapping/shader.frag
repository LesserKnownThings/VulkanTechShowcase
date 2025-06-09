#version 460

layout (location = 0) out vec4 fragColor;

layout (location = 0) in vec2 inUV;

layout (set = 0, binding = 1) uniform sampler2DArray depthMap;

layout (set = 1, binding = 0) uniform DebugData {
    float nearPlane;
    float farPlane;
    int layer;
} debugData;

void main() {
    float depthValue = texture(depthMap, vec3(inUV, debugData.layer)).r;
    fragColor = vec4(vec3(depthValue), 1.0);
}