#version 330 core

// MRT: two color outputs
layout(location = 0) out vec4 FragColor;   // attachment 0 -> sceneTex  (color)
layout(location = 1) out vec4 NormalColor; // attachment 1 -> normalTex (view normals)

in vec2 vTexCoord;
in vec3 vViewNormal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_emissive1;
uniform sampler2D texture_normal;
uniform bool flipuvs;

void main() {
    vec2 uv = flipuvs ? vec2(vTexCoord.x, 1.0 - vTexCoord.y) : vTexCoord;

    // Attachment 0: emissive color (unchanged from before)
    vec4 emissive = texture(texture_emissive1, uv);
    // Fallback to diffuse if emissive is black
    if (dot(emissive.rgb, vec3(1.0)) < 0.01)
        emissive = texture(texture_diffuse1, uv);
    FragColor = emissive;

    // Attachment 1: pack view-space normal into [0,1]
    NormalColor = vec4(normalize(vViewNormal) * 0.5 + 0.5, 1.0);
}