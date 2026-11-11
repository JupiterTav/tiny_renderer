#version 330 core

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uViewPosition;
uniform vec3 uLightDirection;
uniform vec3 uBaseColor;
uniform sampler2D uTexture;
uniform bool uUseTexture;

out vec4 FragColor;

void main() {
    vec3 albedo = uUseTexture ? texture(uTexture, vTexCoord).rgb : uBaseColor;

    vec3 normal = normalize(vNormal);
    vec3 light = normalize(-uLightDirection);
    vec3 view = normalize(uViewPosition - vWorldPosition);
    vec3 halfway = normalize(light + view);

    float diffuse = max(dot(normal, light), 0.0);
    float specular = pow(max(dot(normal, halfway), 0.0), 64.0) * 0.25;

    vec3 color = albedo * (0.3 + 0.6 * diffuse) + vec3(specular);
    FragColor = vec4(color, 1.0);
}
