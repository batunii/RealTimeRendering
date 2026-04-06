#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;
out vec3 vViewNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normalMat;

void main() {
    vTexCoord   = aTexCoord;
    // View-space normal for G-buffer
    vViewNormal = normalize(mat3(normalMat) * aNormal);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}