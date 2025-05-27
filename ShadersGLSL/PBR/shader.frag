#version 450

layout (constant_id = 0) const uint MAX_LIGHTS = 32;
layout (constant_id = 1) const uint LIGHT_TYPE_DIRECTIONAL = 1 << 0;
layout (constant_id = 2) const uint LIGHT_TYPE_POINT = 1 << 1;
layout (constant_id = 3) const uint LIGHT_TYPE_SPOT = 1 << 2;

layout(location = 0) in VertexLightData {
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec3 fragPosition;
    vec2 uv;
} vertexLightData;

layout(location = 0) out vec4 fragColor;

layout(set = 2, binding = 0) uniform sampler2D albedo;

layout(push_constant, std430) uniform LightConstants {
    layout(offset = 64) mat3 normalMatrix;
    int lightsCount;
    float ambientStrength;
} lightConstants;

struct Light {
    // w => type (needs to be converted from float bits to uint)
    vec4 position;
    vec4 direction;
    // a => intensity
    vec4 color;
    // x => range (for point/spot), y => inner cone, z => outer cone
    vec4 params;
};

layout(set = 1, binding = 0) readonly buffer LightData {
    Light lights[MAX_LIGHTS];
} lightData;

// vec3 EvaluateLight(Light light, vec3 finalColor) {
//     vec3 normal = normalize(lightConstants.normalMatrix * vertexLightData.normal);
//     vec3 lightDirection = normalize(light.position.xyz - vertexLightData.fragPosition);
//     float diff = max(dot(normal, lightDirection), 0.0);
//     vec3 diffuse = diff * light.color.xyz;
//     vec3 ambient = lightConstants.ambientStrength * light.color.xyz;
//     return (ambient + diffuse) * finalColor;
// }

vec3 EvaluateDirectionalLight(Light light, vec3 color) {
    vec3 lightColor = light.color.xyz;
    float intensity = light.color.a;

    vec3 normal = normalize(vertexLightData.normal);
    vec3 dir = -light.direction.xyz;

    // Diffuse
    float diff = max(dot(normal, dir), 0.0);
    vec3 diffuse = diff * lightColor * intensity;

    // Ambient
    vec3 ambient = lightConstants.ambientStrength * lightColor;

    return (ambient + diffuse) * color;
}

void main() {
    vec4 finalColor = texture(albedo, vertexLightData.uv);
    vec3 result = finalColor.xyz * lightConstants.ambientStrength;

    for(int i = 0; i < lightConstants.lightsCount; ++i) {
        uint lightType = floatBitsToUint(lightData.lights[i].position.w);
        if((lightType & LIGHT_TYPE_DIRECTIONAL) != 0) {
            result += EvaluateDirectionalLight(lightData.lights[i], finalColor.xyz);
        }
    }

    fragColor = vec4(result, finalColor.a);
    // fragColor = vec4(vertexLightData.normal * 0.5 + 0.5, 1.0);
}