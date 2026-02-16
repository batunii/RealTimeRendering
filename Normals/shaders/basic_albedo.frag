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
uniform float bumpStrength;
// Normal mapping toggle
uniform bool useNormalMap;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;
in vec3 WorldNormal;

out vec4 FragColor;

void main() {
  vec3 albedo = vec3(1.0f); 
    vec3 normal;
    if (useNormalMap) {
        vec3 normalMap = texture(texture_normal1, TexCoords).rgb;
        normalMap = normalize(normalMap * 2.0 - 1.0);
	normalMap.xy *= bumpStrength;
        normal = normalize(TBN * normalMap);
    } else {
        normal = normalize(WorldNormal);
    }
    
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * albedo;
    
    FragColor = vec4(result, 1.0);
}
