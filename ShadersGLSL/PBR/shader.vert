#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 bitangent;
layout (location = 4) in vec2 uv;

layout(location = 0) out VertexLightData {
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec3 fragPosition;
    vec2 uv;
} vertexLightData;

layout(set = 0, binding = 0) uniform Camera {
    mat4 projection;
    mat4 view;
} camera;

layout(push_constant) uniform ModelConstants {
    mat4 model;
} modelConstants;

void main() {
    gl_Position = camera.projection * camera.view * modelConstants.model * vec4(position, 1.0);

    vertexLightData.normal = normal;
    vertexLightData.tangent = tangent;
    vertexLightData.bitangent = bitangent;
    vertexLightData.fragPosition = vec3(modelConstants.model * vec4(position, 1.0));
    vertexLightData.uv = uv;
}