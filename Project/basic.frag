#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_emissive1;
uniform sampler2D texture_normal;
uniform bool flipuvs;
void main()
{
  //FragColor = vec4(1.0f);
  vec2 uv = flipuvs ? vec2(vTexCoord.x, 1.0 - vTexCoord.y) : vTexCoord;
  FragColor = texture(texture_emissive1, uv);
}
