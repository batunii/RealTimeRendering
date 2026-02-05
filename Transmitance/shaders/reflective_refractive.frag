#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 ViewDir;

uniform sampler2D envMap;
uniform float ior;
uniform float chromaticDispersion;
uniform float reflectivity;
vec2 dirToSphericalUV(vec3 dir) {
    vec3 d = normalize(dir);
    
    float u = 0.5 + atan(d.z, d.x) / (2.0 * 3.14159265359);
    float v = 0.5 - asin(d.y) / 3.14159265359;
    
    return vec2(u, v);
}

float fresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 I = normalize(ViewDir);
    
    vec3 finalColor;
    
        float cosTheta = abs(dot(N, I));
        float fresnel = fresnelSchlick(cosTheta,reflectivity);
        
        vec3 R = reflect(I, N);
        vec2 reflectUV = -dirToSphericalUV(R);
        vec3 reflectColor = texture(envMap, reflectUV).rgb;
        
        float etaR = 1.0 / max(ior - chromaticDispersion, 1.001);
        float etaG = 1.0 / max(ior, 1.001);
        float etaB = 1.0 / max(ior + chromaticDispersion, 1.001);
        
        vec3 refractR = refract(I, N, etaR);
        vec3 refractG = refract(I, N, etaG);
        vec3 refractB = refract(I, N, etaB);
        
        vec3 fallback = reflect(I, N);
        if (length(refractR) < 0.1) refractR = fallback;
        if (length(refractG) < 0.1) refractG = fallback;
        if (length(refractB) < 0.1) refractB = fallback;
        
        vec2 uvR = dirToSphericalUV(-refractR);
        vec2 uvG = dirToSphericalUV(-refractG);
        vec2 uvB = dirToSphericalUV(-refractB);
        
        float r = texture(envMap, uvR).r;
        float g = texture(envMap, uvG).g;
        float b = texture(envMap, uvB).b;
        
        vec3 refractColor = vec3(r, g, b);
        
        finalColor = mix(refractColor, reflectColor, fresnel);
    
    FragColor = vec4(finalColor, 1.0);
}
