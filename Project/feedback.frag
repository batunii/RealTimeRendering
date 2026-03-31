#version 330 core
in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_prevFrame;
uniform float u_lambda;  // displacement strength, 0.01-0.1

void main() {
    // Sample current pixel color (Section 3)
    vec3 c = texture(u_prevFrame, v_uv).rgb;
    
    // Compute per-pixel displacement from color (Eq 3)
    vec2 disp = u_lambda * (2.0 * c.rg - vec2(1.0));
    
    // Sample displaced location
    vec2 newUV = clamp(v_uv + disp, 0.0, 1.0);
    vec3 displaced = texture(u_prevFrame, newUV).rgb;
    
    // Mix original + displaced → painterly smudge
    fragColor = vec4(mix(c, displaced, 0.5), 1.0);
}
