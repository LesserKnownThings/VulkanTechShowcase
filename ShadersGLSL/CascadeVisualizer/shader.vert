#version 460

layout (location = 0) in vec3 position;

layout(set = 0, binding = 0) uniform Camera {
    mat4 projView;
} camera;

void main() {
    gl_Position = camera.projView * vec4(position, 1.0);
}