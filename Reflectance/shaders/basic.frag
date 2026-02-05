#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 albedo;

void main()
{
    // Normalize inputs
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);

    // Lambert diffuse term
    float diff = max(dot(N, L), 0.0);

    vec3 diffuse = diff * lightColor * albedo;

    FragColor = vec4(diffuse, 1.0);
}
