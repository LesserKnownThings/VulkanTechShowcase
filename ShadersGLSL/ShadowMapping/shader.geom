#version 460

layout (constant_id = 0) const uint MAX_SM = 16;

const uint MAX_VERTICES = 3;

layout (triangles, invocations = 5) in;
layout (triangle_strip, max_vertices = MAX_VERTICES) out;

layout (set = 1, binding = 0) readonly uniform LightSM {
    mat4 matrices[MAX_SM];
} lightSM;

void main() {
    for (int i = 0; i < MAX_VERTICES; ++i) {
        gl_Position = lightSM.matrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}