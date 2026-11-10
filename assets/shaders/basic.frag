#version 330 core

in vec3 vWorldPosition;
in vec3 vNormal;

uniform vec3 uViewPosition;
uniform vec3 uLightDirection;
uniform vec3 uBaseColor;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 light = normalize(-uLightDirection);
    vec3 view = normalize(uViewPosition - vWorldPosition);
    vec3 halfway = normalize(light + view);

    float diffuse = max(dot(normal, light), 0.0);
    float specular = pow(max(dot(normal, halfway), 0.0), 64.0) * 0.25;

    vec3 color = uBaseColor * (0.2 + 0.8 * diffuse) + vec3(specular);
    FragColor = vec4(color, 1.0);
}
