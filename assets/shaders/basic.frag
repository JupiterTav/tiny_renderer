#version 330 core

//Light struct in include/Light.hpp.
struct LightProperties {
    vec3 direction; // world-space direction the light travels
    vec3 color; // direct light color
    float intensity;
    float ambientContribution;
};

uniform LightProperties uLight;

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uViewPosition;

uniform bool uUnlit;
uniform vec3 uOverrideColor;

uniform vec3 uAmbientColor; // MTL Ka
uniform vec3 uDiffuseColor; // MTL Kd
uniform vec3 uSpecularColor; // MTL Ks
uniform float uShininess; // MTL Ns

uniform sampler2D uTexture;
uniform bool uUseTexture;

// Data maps (sampled as linear values). The red channel of the metallic map
// controls how metallic the surface is; roughness controls the specular lobe.
uniform sampler2D uMetallicMap;
uniform bool uUseMetallicMap;
uniform sampler2D uRoughnessMap;
uniform bool uUseRoughnessMap;
uniform sampler2D uNormalMap;
uniform bool uUseNormalMap;

out vec4 FragColor;

// Build a tangent basis from screen-space derivatives. This avoids needing
// tangent attributes in the mesh while still supporting any UV mapping.
vec3 perturbNormal(vec3 N, vec3 worldPosition, vec2 uv, vec3 sampledNormal) {
    vec3 mapN = sampledNormal * 2.0 - 1.0;

    vec3 dp1 = dFdx(worldPosition);
    vec3 dp2 = dFdy(worldPosition);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // Normalize the largest of T and B to keep the basis stable.
    float invMax = inversesqrt(max(dot(T, T), dot(B, B)));
    mat3 TBN = mat3(T * invMax, B * invMax, N);
    return normalize(TBN * mapN);
}

void main() {
    vec3 normal = normalize(vNormal);
    if (uUseNormalMap) {
        vec3 sampledNormal = texture(uNormalMap, vTexCoord).rgb;
        normal = perturbNormal(normal, vWorldPosition, vTexCoord,
                sampledNormal);
    }
    vec3 light = normalize(-uLight.direction);
    vec3 eye = normalize(uViewPosition - vWorldPosition);

    vec3 halfway = normalize(light + eye);

    vec3 albedo = uUseTexture ? texture(uTexture, vTexCoord).rgb : uDiffuseColor;

    // Metallic map: reduce diffuse where metallic and tint the specular
    // reflection with the albedo (F0 of a conductor).
    float metallic = uUseMetallicMap ? texture(uMetallicMap, vTexCoord).r : 0.0;
    vec3 diffuseColor = albedo * (1.0 - metallic);
    vec3 specularColor = mix(uSpecularColor, albedo, metallic);

    // Roughness map: convert roughness to a Blinn-Phong exponent. Smooth
    // surfaces give tight highlights; rough surfaces give broad ones.
    float shininess = uShininess;
    if (uUseRoughnessMap) {
        float roughness = max(texture(uRoughnessMap, vTexCoord).r, 0.045);
        shininess = clamp(2.0 / (roughness * roughness) - 2.0, 1.0, 256.0);
    }

    float NdotL = max(dot(normal, light), 0.0);
    float specular = NdotL > 0.0
        ? pow(max(dot(normal, halfway), 0.0), shininess) : 0.0;

    vec3 color = (uLight.ambientContribution * diffuseColor)
            + ((uAmbientColor * NdotL * diffuseColor) + (specularColor * specular))
                * (uLight.color * uLight.intensity);

    // Gamma correction:
    // Textures are uploaded as sRGB and sampled in linear space, so encode
    // the final color back to sRGB for the default framebuffer.
    FragColor = uUnlit ? vec4(uOverrideColor, 1.0) : vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
