#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;
uniform sampler2D uTexture;

void main()
{
  //FragColor = vec4(1.0f);
   FragColor = texture(uTexture, vTexCoord);
}
