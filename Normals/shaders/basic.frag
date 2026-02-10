#version 330 core

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;

// Light properties
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Material properties
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;

// Normal mapping toggle
uniform bool useNormalMap;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;
in vec3 WorldNormal;

out vec4 FragColor;

void main() {
    // Sample diffuse texture
    vec3 albedo = texture(texture_diffuse1, TexCoords).rgb;
    
    // Choose between normal map or geometry normal
    vec3 normal;
    if (useNormalMap) {
        // Sample and decode normal map
        vec3 normalMap = texture(texture_normal1, TexCoords).rgb;
        normalMap = normalize(normalMap * 2.0 - 1.0);
	normalMap.y = - normalMap.y;
        normal = normalize(TBN * normalMap);
    } else {
        // Use geometry normal only
        normal = normalize(WorldNormal);
    }
    
    // Ambient lighting
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine lighting with albedo
    vec3 result = (ambient + diffuse + specular) * albedo;
    
    FragColor = vec4(result, 1.0);
}
