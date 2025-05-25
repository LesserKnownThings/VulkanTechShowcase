#version 450

layout (constant_id = 0) const uint MAX_LIGHTS = 32;

layout(location = 0) in VertexLightData {
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec3 fragPosition;
    vec2 uv;
} vertexLightData;

layout(location = 0) out vec4 fragColor;

layout(set = 2, binding = 0) uniform sampler2D uAlbedoMap;

layout(push_constant) uniform LightConstants {
    mat3 normalMatrix;
    int lightsCount;
    float ambientStrength;
} lightConstants;

struct Light {
    // w => type (0 = directional, 1 = point, 2 = spot)
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

vec3 EvaluateLight(Light light, vec3 finalColor) {
    vec3 normal = normalize(lightConstants.normalMatrix * vertexLightData.normal);
    vec3 lightDirection = normalize(light.position.xyz - vertexLightData.fragPosition);
    float diff = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = diff * light.color.xyz;
    vec3 ambient = lightConstants.ambientStrength * light.color.xyz;
    return (ambient + diffuse) * finalColor;
}

void main() {
    vec4 finalColor = texture(uAlbedoMap, vertexLightData.uv);

    vec3 result = EvaluateLight(lightData.lights[0], finalColor.xyz);

    // vec3 diffuseLight;
    // CalculateDiffuseLight(finalColor, diffuseLight);

    //fragColor = vec4(result, finalColor.a);
    fragColor = finalColor;
}