#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_emissive1;
uniform sampler2D texture_normal;
void main()
{
  //FragColor = vec4(1.0f);
  FragColor = texture(texture_emissive1, vTexCoord);
}
