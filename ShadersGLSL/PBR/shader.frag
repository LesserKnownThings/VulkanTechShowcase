#version 450

layout (constant_id = 0) const uint MAX_LIGHTS = 32;
layout (constant_id = 1) const uint LIGHT_TYPE_DIRECTIONAL = 1 << 0;
layout (constant_id = 2) const uint LIGHT_TYPE_POINT = 1 << 1;
layout (constant_id = 3) const uint LIGHT_TYPE_SPOT = 1 << 2;

layout(location = 0) out vec4 fragColor;
layout(set = 3, binding = 0) uniform sampler2D albedo;

// Material Data

// *************

layout(location = 0) in VertexData {
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec3 fragPosition;
    vec2 uv;
} vertexData;

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

struct Light {
    // w => type (needs to be converted from float bits to uint)
    vec4 position;
    vec4 direction;
    // a => intensity
    vec4 color;
    // x => range (for point/spot), y => inner angle, z => outer angle
    vec4 params;
};

layout(set = 1, binding = 0) readonly buffer LightData {
    Light lights[MAX_LIGHTS];
} lightData;

const float SPECULAR_STR = 0.5;

const float CONSTANT = 1.0;
const float LINEAR = 0.09f;
const float QUADRATIC = 0.032;

vec3 CalculateDiffuse(vec3 normal, vec3 dir, vec3 lightColor, float intensity) {
    float diff = max(dot(normal, dir), 0.0);
    return diff * lightColor * intensity;
}

vec3 CalculateSpecular(vec3 viewPosition, vec3 normal, vec3 dir, vec3 lightColor) {
    vec3 viewDir = normalize(viewPosition - vertexData.fragPosition);
    vec3 reflectDir = reflect(dir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    return SPECULAR_STR * spec * lightColor;
}

float CalculateAttenuation(vec3 lightPosition, float range) {
    float distance = length(lightPosition - vertexData.fragPosition);

    if(distance > range) {
        return 0.0;
    }

    return 1.0 / (CONSTANT + LINEAR * distance + QUADRATIC * (distance * distance));
}

vec3 CalculateDirectionalLight(Light light, mat3 normalMatrix, vec3 viewPosition, vec3 objectColor) {
    vec3 lightColor = light.color.xyz;
    float intensity = light.color.a;

    vec3 normal = normalize(normalMatrix * vertexData.normal);
    vec3 dir = -light.direction.xyz;

    vec3 diffuse = CalculateDiffuse(normal, dir, lightColor, intensity);
    vec3 specular = CalculateSpecular(viewPosition, normal, dir, lightColor);

    return (diffuse + specular) * objectColor;
}

vec3 CalculatePointLight(Light light, mat3 normalMatrix, vec3 viewPosition, vec3 objectColor) {
    vec3 lightColor = light.color.xyz;
    float intensity = light.color.a;
    float range = light.params.x;

    vec3 normal = normalize(normalMatrix * vertexData.normal);
    vec3 dir = normalize(light.position.xyz - vertexData.fragPosition);

    float attenuation = CalculateAttenuation(light.position.xyz, range);

    vec3 diffuse = CalculateDiffuse(normal, dir, lightColor, intensity) * attenuation;   
    vec3 specular = CalculateSpecular(viewPosition, normal, dir, lightColor) * attenuation;

    return (diffuse + specular) * objectColor;
}

vec3 CalculateSpotLight(Light light, mat3 normalMatrix, vec3 viewPosition, vec3 objectColor) {
    vec3 lightColor = light.color.xyz;
    float intensity = light.color.a;

    vec3 normal = normalize(normalMatrix * vertexData.normal);
    vec3 dir = normalize(light.position.xyz - vertexData.fragPosition);

    float cutOff = light.params.y;
    float outerCutoff = light.params.z;
    float range = light.params.x;

    float theta = dot(dir, normalize(-light.direction.xyz));
    float epsilon = cutOff - outerCutoff;

    float spotIntensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

    float attenuation = CalculateAttenuation(light.position.xyz, range);

    vec3 diffuse = CalculateDiffuse(normal, dir, lightColor, intensity) * spotIntensity * attenuation;
    vec3 specular = CalculateSpecular(viewPosition, normal, dir, lightColor) * spotIntensity * attenuation;

    return (diffuse + specular) * objectColor;
}

void main() {
    vec4 finalColor = texture(albedo, vertexData.uv);
    vec3 result = finalColor.xyz * sharedConstants.ambientStrength;

    mat3 normalMatrix = mat3(sharedConstants.col0.xyz, sharedConstants.col1.xyz, sharedConstants.col2.xyz);
    vec3 viewPosition = vec3(sharedConstants.col0.w, sharedConstants.col1.w, sharedConstants.col2.w);;

    for(int i = 0; i < sharedConstants.lightsCount; ++i) {
        uint lightType = floatBitsToUint(lightData.lights[i].position.w);
        if((lightType & LIGHT_TYPE_DIRECTIONAL) != 0) {
            result += CalculateDirectionalLight(lightData.lights[i], normalMatrix, viewPosition, finalColor.xyz);
        }
        else if ((lightType & LIGHT_TYPE_POINT) != 0) {
            result += CalculatePointLight(lightData.lights[i], normalMatrix, viewPosition, finalColor.xyz);
        }
        else if ((lightType & LIGHT_TYPE_SPOT) != 0) {
            result += CalculateSpotLight(lightData.lights[i], normalMatrix, viewPosition, finalColor.xyz);
        }
    }

    fragColor = vec4(result, finalColor.a);

    // TODO move this to a separate shader for debugging
    // fragColor = vec4(vertexLightData.normal * 0.5 + 0.5, 1.0);
    //fragColor = vec4(vertexData.uv, 0.0, 1.0);
}