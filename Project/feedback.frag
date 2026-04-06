#version 330 core
in  vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_prevFrame;   // unit 0: ping-pong color
uniform sampler2D u_depthTex;    // unit 1: G-buffer depth
uniform sampler2D u_normalTex;   // unit 2: G-buffer view normals

uniform float u_lambda;
uniform float u_angle;
uniform vec2  u_texelSize;
uniform float u_edgeStrength;
uniform float u_lumaStrength;
uniform float u_quantSteps;
uniform float u_quantStrength;

// Depth
uniform int   u_useDepth;
uniform float u_depthStrength;
uniform int   u_depthMode;   // 0=near smears, 1=far smears, 2=mid-focus sharp

// Normals
uniform int   u_useNormals;
uniform float u_normalStrength;
uniform int   u_normalMode;  // 0=normals only, 1=color+normal blend, 2=auto toggle

float luminance(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

float lineariseDepth(float d) {
    float near = 0.1, far = 10000.0;
    return (2.0 * near) / (far + near - d * (far - near));
}

void main() {
    vec3 c    = texture(u_prevFrame, v_uv).rgb;
    vec2 base = 2.0 * c.rg - vec2(1.0);

    // Rotation matrix for stroke angle
    float cosA = cos(u_angle);
    float sinA = sin(u_angle);
    mat2  rot  = mat2(cosA, -sinA, sinA, cosA);

    // 4-tap edge detection
    float right = luminance(texture(u_prevFrame, v_uv + vec2( u_texelSize.x, 0.0)).rgb);
    float left  = luminance(texture(u_prevFrame, v_uv + vec2(-u_texelSize.x, 0.0)).rgb);
    float up    = luminance(texture(u_prevFrame, v_uv + vec2(0.0,  u_texelSize.y)).rgb);
    float down  = luminance(texture(u_prevFrame, v_uv + vec2(0.0, -u_texelSize.y)).rgb);
    float edge  = clamp(length(vec2(right - left, up - down)) * 2.0, 0.0, 1.0);

    float luma  = luminance(c);
    float edgeW = mix(1.0, edge, clamp(u_edgeStrength, 0.0, 3.0) / 3.0);
    float lumaW = mix(1.0, luma, clamp(u_lumaStrength, 0.0, 3.0) / 3.0);

    // Depth weight
    float depthW = 1.0;
    if (u_useDepth == 1) {
        float linDepth = lineariseDepth(texture(u_depthTex, v_uv).r);
        if (u_depthMode == 0) {
            // Near smears more
            depthW = mix(1.0, 1.0 - linDepth, u_depthStrength / 3.0);
        } else if (u_depthMode == 1) {
            // Far smears more
            depthW = mix(1.0, linDepth, u_depthStrength / 3.0);
        } else {
            // Mid-focus sharp: near + far smear, midground stays clean
            float midFocus = 1.0 - abs(linDepth - 0.5) * 2.0;
            depthW = mix(1.0, midFocus, u_depthStrength / 3.0);
        }
        depthW = clamp(depthW, 0.0, 1.0);
    }

    // Normal-driven displacement
    vec2 normalDisp = vec2(0.0);
    if (u_useNormals == 1) {
        // Unpack view-space normal from [0,1] to [-1,1]
        vec3 N = normalize(texture(u_normalTex, v_uv).rgb * 2.0 - 1.0);
        // Use XY of view-space normal as stroke direction
        // This aligns strokes with surface contours in screen space
        normalDisp = N.xy * u_normalStrength * u_lambda;
    }

    // Assemble final displacement
    vec2 colorDisp = u_lambda * edgeW * lumaW * depthW * (rot * base);
    vec2 disp;

    if (u_useNormals == 1) {
        if (u_normalMode == 0) {
            // Normals only
            disp = normalDisp * depthW;
        } else if (u_normalMode == 1) {
            // Blend color displacement + normal displacement
            disp = mix(colorDisp, normalDisp * depthW, 0.5);
        } else {
            // Auto toggle: whichever is stronger wins
            disp = length(normalDisp) > length(colorDisp)
                   ? normalDisp * depthW
                   : colorDisp;
        }
    } else {
        disp = colorDisp;
    }

    vec2 newUV     = clamp(v_uv + disp, 0.0, 1.0);
    vec3 displaced = texture(u_prevFrame, newUV).rgb;
    vec3 result    = mix(c, displaced, 0.5);

    // Palette quantisation
    vec3 quantized = floor(result * u_quantSteps) / u_quantSteps;
    result = mix(result, quantized, u_quantStrength);

    fragColor = vec4(result, 1.0);
}