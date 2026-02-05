#pragma once

#include <glad/glad.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stdexcept>
#include <string>
#include <vector>

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
};

class Mesh {
public:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  unsigned int VAO{}, VBO{}, EBO{};

  Mesh(const std::vector<Vertex> &v, const std::vector<unsigned int> &i)
      : vertices(v), indices(i) {
    setup();
  }

  void draw() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }

private:
  void setup() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
  }
};

class Model {
public:
  std::vector<Mesh> meshes;

  explicit Model(const std::string &path) { load(path); }

  void draw() const {
    for (const auto &m : meshes)
      m.draw();
  }

private:
  void load(const std::string &path) {
    Assimp::Importer importer;
    const aiScene *scene =
        importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals |
                                    aiProcess_JoinIdenticalVertices);

    if (!scene || !scene->mRootNode)
      throw std::runtime_error("Assimp failed to load model");

    processNode(scene->mRootNode, scene);
  }

  void processNode(aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      meshes.emplace_back(processMesh(mesh));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
      processNode(node->mChildren[i], scene);
  }

  Mesh processMesh(aiMesh *mesh) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
      Vertex v;
      v.Position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                    mesh->mVertices[i].z};
      v.Normal = {mesh->mNormals[i].x, mesh->mNormals[i].y,
                  mesh->mNormals[i].z};
      vertices.push_back(v);
      v.TexCoords = mesh->mTextureCoords[0]
                        ? glm::vec2(mesh->mTextureCoords[0][i].x,
                                    mesh->mTextureCoords[0][i].y)
                        : glm::vec2(0.0f);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
      for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
        indices.push_back(mesh->mFaces[i].mIndices[j]);
    }

    return Mesh(vertices, indices);
  }
};
