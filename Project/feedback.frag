#version 330 core
in  vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_prevFrame;
uniform float     u_lambda;
uniform float     u_angle;
uniform vec2      u_texelSize;
uniform float     u_edgeStrength;
uniform float     u_lumaStrength;
uniform float     u_quantSteps;
uniform float     u_quantStrength;

float luminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec3 c = texture(u_prevFrame, v_uv).rgb;

    // Base displacement driven by pixel color (paper Eq. 3)
    vec2 base = 2.0 * c.rg - vec2(1.0);

    // Rotation matrix from ImGui angle
    float cosA = cos(u_angle);
    float sinA = sin(u_angle);
    mat2  rot  = mat2(cosA, -sinA,
                      sinA,  cosA);

    // Fast 4-tap edge detection (saves 5 samples vs full Sobel)
    float right = luminance(texture(u_prevFrame, v_uv + vec2( u_texelSize.x, 0.0)).rgb);
    float left  = luminance(texture(u_prevFrame, v_uv + vec2(-u_texelSize.x, 0.0)).rgb);
    float up    = luminance(texture(u_prevFrame, v_uv + vec2(0.0,  u_texelSize.y)).rgb);
    float down  = luminance(texture(u_prevFrame, v_uv + vec2(0.0, -u_texelSize.y)).rgb);
    float edge  = clamp(length(vec2(right - left, up - down)) * 2.0, 0.0, 1.0);

    // Luma weight — bright areas smear more
    float luma = luminance(c);

    // Blend between flat (1.0) and content-driven weight
    float edgeW = mix(1.0, edge, clamp(u_edgeStrength, 0.0, 3.0) / 3.0);
    float lumaW = mix(1.0, luma, clamp(u_lumaStrength, 0.0, 3.0) / 3.0);

    // Final displacement: rotated, edge-weighted, luma-weighted
    vec2 disp  = u_lambda * edgeW * lumaW * (rot * base);
    vec2 newUV = clamp(v_uv + disp, 0.0, 1.0);

    vec3 displaced = texture(u_prevFrame, newUV).rgb;
    vec3 result    = mix(c, displaced, 0.5);

    // Optional palette quantization
    vec3 quantized = floor(result * u_quantSteps) / u_quantSteps;
    result = mix(result, quantized, u_quantStrength);

    fragColor = vec4(result, 1.0);
}