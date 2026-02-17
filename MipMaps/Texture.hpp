#pragma once

#include <glad/glad.h>
#include <iostream>
#include <stdexcept>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"
using uint = unsigned int;

class TextureClass {
public:
  uint ID;
  int m_height, m_width, m_channels;
  std::string m_type, m_path;

  TextureClass(const std::string &path) {
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data =
        stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data)
      throw std::runtime_error("Failed to load image " + path);
    else
      std::cout << "Loaded texture file from : " << path << std::endl;
    GLenum format = GL_RGB;
    if (m_channels == 1)
      format = GL_RED;
    if (m_channels == 3)
      format = GL_RGB;
    if (m_channels == 4)
      format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
  }
  void bind(uint unit = 0) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
  }
 void change_mipmap(GLuint min, GLuint mag, uint unit = 0) const {
    bind(unit);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    if (min == GL_LINEAR_MIPMAP_LINEAR ||
        min == GL_NEAREST_MIPMAP_LINEAR ||
        min == GL_NEAREST_MIPMAP_NEAREST ||
        min == GL_LINEAR_MIPMAP_NEAREST) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}
};
