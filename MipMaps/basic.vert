#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;  // vec3 now
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float tiles;
void main()
{
    vTexCoord = aTexCoord*tiles;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
