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

// Used to draw the light-position marker as a flat, unlit color.
uniform bool uUnlit;
uniform vec3 uOverrideColor;

uniform vec3 uAmbientColor; // MTL Ka
uniform vec3 uDiffuseColor; // MTL Kd
uniform vec3 uSpecularColor; // MTL Ks
uniform float uShininess; // MTL Ns

uniform sampler2D uTexture;
uniform bool uUseTexture;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 light = normalize(-uLight.direction);
    vec3 eye = normalize(uViewPosition - vWorldPosition);

    vec3 halfway = normalize(light + eye);

    vec3 albedo = uUseTexture ? texture(uTexture, vTexCoord).rgb : uDiffuseColor;

    float NdotL = max(dot(normal, light), 0.0);
    float specular = NdotL > 0.0
        ? pow(max(dot(normal, halfway), 0.0), uShininess) : 0.0;

    vec3 color = (uLight.ambientContribution * albedo) + ((uAmbientColor * NdotL * albedo) + (uSpecularColor * specular * albedo)) * (uLight.color * uLight.intensity);

    // Gamma correction:
    // Textures are uploaded as sRGB and sampled in linear space, so encode
    // the final color back to sRGB for the default framebuffer.
    FragColor = uUnlit ? vec4(uOverrideColor, 1.0)
                       : vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
