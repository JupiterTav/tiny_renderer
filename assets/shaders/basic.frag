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

uniform vec3 uAmbientColor; // MTL Ka
uniform vec3 uDiffuseColor; // MTL Kd
uniform vec3 uSpecularColor; // MTL Ks
uniform float uShininess; // MTL Ns

uniform sampler2D uTexture;
uniform bool uUseTexture;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLight.direction);
    vec3 V = normalize(uViewPosition - vWorldPosition);

    vec3 H = L + V;
    float hLength = length(H);
    H = hLength > 0.0 ? H / hLength : N;

    vec3 albedo = uUseTexture ? texture(uTexture, vTexCoord).rgb : uDiffuseColor;

    float NdotL = max(dot(N, L), 0.0);
    float specular = NdotL > 0.0
        ? pow(max(dot(N, H), 0.0), uShininess) : 0.0;

    vec3 direct = (((uLight.ambientContribution + uAmbientColor) * albedo) + (NdotL * albedo) + (uSpecularColor * specular)) * (uLight.color * uLight.intensity);

    // Textures are uploaded as sRGB and sampled in linear space, so encode
    // the final color back to sRGB for the default framebuffer.
    FragColor = vec4(pow(direct, vec3(1.0 / 2.2)), 1.0);
}
