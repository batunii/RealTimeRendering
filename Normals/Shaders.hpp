#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

class Shader {
public:
  unsigned int ID;

  Shader(const std::string &vertexPath, const std::string &fragmentPath) {
    std::string vertexCode;
    std::string fragmentCode;

    std::ifstream vShaderFile(vertexPath);
    std::ifstream fShaderFile(fragmentPath);

    if (!vShaderFile || !fShaderFile)
      throw std::runtime_error("Failed to open shader file");

    std::stringstream vStream, fStream;
    vStream << vShaderFile.rdbuf();
    fStream << fShaderFile.rdbuf();

    vertexCode = vStream.str();
    fragmentCode = fStream.str();

    const char *vCode = vertexCode.c_str();
    const char *fCode = fragmentCode.c_str();

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vCode, nullptr);
    std::cout<<"Loading Shader : "<< vertexPath<<std::endl;
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fCode, nullptr);
    glCompileShader(fragment);
    std::cout<<"Loading Shader : "<< fragmentPath<<std::endl;
    checkCompileErrors(fragment, "FRAGMENT");

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
  }

  void use() const { glUseProgram(ID); }

  void setBool(const std::string &name, bool value) const {
    glUniform1i(getLocation(name), (int)value);
  }

  void setInt(const std::string &name, int value) const {
    glUniform1i(getLocation(name), value);
  }

  void setFloat(const std::string &name, float value) const {
    glUniform1f(getLocation(name), value);
  }

  void setVec3(const std::string &name, const glm::vec3 &v) const {
    glUniform3fv(getLocation(name), 1, &v[0]);
  }

  void setMat4(const std::string &name, const glm::mat4 &m) const {
    glUniformMatrix4fv(getLocation(name), 1, GL_FALSE, &m[0][0]);
  }

private:
  mutable std::unordered_map<std::string, int> cache;

  int getLocation(const std::string &name) const {
    if (cache.find(name) != cache.end())
      return cache[name];

    int loc = glGetUniformLocation(ID, name.c_str());
    cache[name] = loc;
    return loc;
  }

  void checkCompileErrors(unsigned int shader, std::string type) {
    int success;
    char infoLog[1024];

    if (type != "PROGRAM") {
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        throw std::runtime_error(type + " SHADER ERROR:\n" + infoLog);
      }
    } else {
      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (!success) {
        glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
        throw std::runtime_error("PROGRAM LINK ERROR:\n" +
                                 std::string(infoLog));
      }
    }
  }
};
