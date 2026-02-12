#version 330 core

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;

// Light properties
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform bool useNormalMap;
uniform float bumpStrength;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;
in vec3 WorldNormal;

out vec4 FragColor;

void main()
{
    vec3 albedo = texture(texture_diffuse1, TexCoords).rgb;
    
    vec3 normal;
    if (useNormalMap) {
        // Sample and decode normal map
        vec3 normalMap = texture(texture_normal1, TexCoords).rgb;
        normalMap = normalize(normalMap * 2.0 - 1.0);
	normalMap.xy *= bumpStrength;
        normal = normalize(TBN * normalMap);
    } else {
        // Use geometry normal only
        normal = normalize(WorldNormal);
    }
    vec3 L = normalize(lightPos - FragPos);

    float diff = max(dot(normal, L), 0.0);
    // Quantize lighting into bands
    diff = floor(diff * 4) / 4;

    vec3 color = diff * albedo * lightColor;

    FragColor = vec4(color, 1.0);
}
