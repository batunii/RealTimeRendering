#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 albedo;
uniform float levels;

void main()
{
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);

    float diff = max(dot(N, L), 0.0);

    // Quantize lighting into bands
    diff = floor(diff * levels) / levels;

    vec3 color = diff * albedo * lightColor;

    FragColor = vec4(color, 1.0);
}
