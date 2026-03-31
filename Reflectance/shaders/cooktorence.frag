#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform vec3  albedo;     // sRGB
uniform float roughness;  // [0,1]
uniform float metallic;
uniform float ao;

const float PI = 3.14159265359;

// -------------------- Beckmann NDF --------------------

float DistributionBeckmann(vec3 N, vec3 H, float roughness)
{
    float NdotH = max(dot(N, H), 0.0);
    float alpha = roughness * roughness;

    float cos2 = NdotH * NdotH;
    float tan2 = (1.0 - cos2) / cos2;

    float denom = PI * alpha * alpha * cos2 * cos2;
    return exp(-tan2 / (alpha * alpha)) / denom;
}

// -------------------- Geometry Term --------------------

float GeometryCookTorrance(vec3 N, vec3 V, vec3 L)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float VdotH = max(dot(V, normalize(V + L)), 0.0);
    float NdotH = max(dot(N, normalize(V + L)), 0.0);

    float g1 = (2.0 * NdotH * NdotV) / VdotH;
    float g2 = (2.0 * NdotH * NdotL) / VdotH;

    return min(1.0, min(g1, g2));
}

// -------------------- Fresnel --------------------

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// -------------------- Main --------------------

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(lightPos - FragPos);
    vec3 H = normalize(V + L);

    // Fix inverted normals if needed
    N = faceforward(N, -V, N);

    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (distance * distance + 1.0);
    vec3 radiance = lightColor * attenuation;

    // sRGB → linear
    vec3 albedoLinear = pow(albedo, vec3(2.2));

    // Base reflectance
    vec3 F0 = mix(vec3(0.04), albedoLinear, metallic);

    float D = DistributionBeckmann(N, H, roughness);
    float G = GeometryCookTorrance(N, V, L);
    vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = D * G * F;
    float denom = 4.0 * max(dot(N, V), 0.0)
                       * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denom;

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);

    vec3 Lo = (kD * albedoLinear / PI + specular)
              * radiance * NdotL;

    vec3 ambient = albedoLinear * ao;
    vec3 color = ambient + Lo;

    // Tone mapping
    color = color / (color + vec3(1.0));

    // Gamma
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
