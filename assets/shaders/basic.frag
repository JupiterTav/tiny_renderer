#version 330 core

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uViewPosition;
uniform vec3 uLightDirection;
uniform vec3 uSpecularColor;
uniform vec3 uDiffuseColor;
uniform float uShininess;
uniform sampler2D uTexture;
uniform bool uUseTexture;

uniform int uUseAmbientLight;
uniform vec3 uAmbientLight;
uniform float uAmbientLightIntensity;

out vec4 FragColor;

void main() {
    vec3 albedo = uUseTexture ? texture(uTexture, vTexCoord).rgb : uDiffuseColor;

    vec3 ambientLight = step(1.0, uUseAmbientLight) * uAmbientLightIntensity * uAmbientLight;

    vec3 normal = normalize(vNormal);
    vec3 light = normalize(-uLightDirection);
    vec3 view = normalize(uViewPosition - vWorldPosition);
    vec3 halfway = normalize(light + view);

    float diffuse = max(dot(normal, light), 0.0);
    vec3 specular = uSpecularColor * pow(max(dot(normal, halfway), 0.0), uShininess);

    vec3 color = albedo * (ambientLight + diffuse + specular) * ambientLight;
    FragColor = vec4(color, 1.0);
}
