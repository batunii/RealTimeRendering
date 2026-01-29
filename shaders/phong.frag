#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 albedo;

uniform float shininess;
void main()
{
    vec3 N = normalize(Normal);

    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(viewPos - FragPos);

    // Diffuse
    float diff = max(dot(N, L), 0.0);

    // Phong specular (reflection vector)
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), shininess);

    vec3 color =
        albedo * diff +
            lightColor * spec;

    FragColor = vec4(color, 1.0);
}
