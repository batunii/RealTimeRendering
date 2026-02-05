#pragma once

#include <glad/glad.h>
#include <string>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class Texture {
public:
    unsigned int ID;
    int width, height, channels;

    explicit Texture(const std::string& path)
    {
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        // Texture parameters (safe defaults)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_set_flip_vertically_on_load(true);

        unsigned char* data =
            stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!data)
            throw std::runtime_error("Failed to load texture: " + path);

        GLenum format = GL_RGB;
        if (channels == 1) format = GL_RED;
        if (channels == 3) format = GL_RGB;
        if (channels == 4) format = GL_RGBA;

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            width,
            height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            data
        );
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }

    void bind(unsigned int unit = 0) const
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, ID);
    }
};
