#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 ViewDir;

uniform sampler2D envMap;
uniform float ior;
uniform float chromaticDispersion;
uniform float reflectivity;
uniform int renderMode;  // 0 = Pure Refraction, 1 = Pure Reflection, 2 = Fresnel

// Simple spherical mapping for environment
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
    
    if (renderMode == 0) {
        // PURE REFRACTION
        float etaR = 1.0 / max(ior - chromaticDispersion, 1.001);
        float etaG = 1.0 / max(ior, 1.001);
        float etaB = 1.0 / max(ior + chromaticDispersion, 1.001);
        
        vec3 refractR = refract(I, N, etaR);
        vec3 refractG = refract(I, N, etaG);
        vec3 refractB = refract(I, N, etaB);
        
        // Fallback to reflection if total internal reflection
        vec3 fallback = reflect(I, N);
        if (length(refractR) < 0.1) refractR = fallback;
        if (length(refractG) < 0.1) refractG = fallback;
        if (length(refractB) < 0.1) refractB = fallback;
        
        // NEGATE refraction directions to see "through" the object
        vec2 uvR = dirToSphericalUV(-refractR);
        vec2 uvG = dirToSphericalUV(-refractG);
        vec2 uvB = dirToSphericalUV(-refractB);
        
        float r = texture(envMap, uvR).r;
        float g = texture(envMap, uvG).g;
        float b = texture(envMap, uvB).b;
        
        finalColor = vec3(r, g, b);
        
    } else if (renderMode == 1) {
        // PURE REFLECTION
        vec3 R = reflect(I, N);
        
        vec2 uv = -dirToSphericalUV(R);
        finalColor = texture(envMap, uv).rgb;
        
    } else {
        // FRESNEL MIX
        //float F0 = pow((1.0 - ior) / (1.0 + ior), 2.0);
        float cosTheta = abs(dot(N, I));
        float fresnel = fresnelSchlick(cosTheta,reflectivity);
        
        // Reflection (don't negate)
        vec3 R = reflect(I, N);
        vec2 reflectUV = -dirToSphericalUV(R);
        vec3 reflectColor = texture(envMap, reflectUV).rgb;
        
        // Refraction with chromatic dispersion (negate)
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
        
        // Negate refraction directions
        vec2 uvR = dirToSphericalUV(-refractR);
        vec2 uvG = dirToSphericalUV(-refractG);
        vec2 uvB = dirToSphericalUV(-refractB);
        
        float r = texture(envMap, uvR).r;
        float g = texture(envMap, uvG).g;
        float b = texture(envMap, uvB).b;
        
        vec3 refractColor = vec3(r, g, b);
        
        // Mix using Fresnel
        finalColor = mix(refractColor, reflectColor, fresnel);
    }
    
    FragColor = vec4(finalColor, 1.0);
}
