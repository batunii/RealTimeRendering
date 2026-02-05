#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Material parameters (from CPU)
uniform vec3  albedo;     // sRGB color
uniform float metallic;   // 0 = dielectric, 1 = metal
uniform float roughness;  // 0 = smooth, 1 = rough
uniform float ao;

const float PI = 3.14159265359;

// -------------------- GGX / Cook–Torrance --------------------

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// -------------------- Main --------------------

void main()
{
    // Normalized vectors
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(lightPos - FragPos);
    vec3 H = normalize(V + L);

    // Distance attenuation (physically correct, softened)
    float distance    = length(lightPos - FragPos);
    float attenuation = 1.0 / (distance * distance + 1.0);
    vec3 radiance     = lightColor * attenuation;

    // ---- COLOR MANAGEMENT ----
    // Convert albedo from sRGB to linear
    vec3 albedoLinear = pow(albedo, vec3(2.2));

    // Base reflectivity
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedoLinear, metallic);

    // Cook–Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denom = 4.0 * max(dot(N, V), 0.0)
                       * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denom;

    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    vec3 Lo = (kD * albedoLinear / PI + specular)
              * radiance * NdotL;

    // Simple ambient (IBL placeholder)
    vec3 ambient = vec3(0.03)* albedoLinear * ao;

    vec3 color = ambient + Lo;

    // ---- TONE MAPPING ----
    color = color / (color + vec3(1.0));

    // ---- GAMMA CORRECTION ----
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
