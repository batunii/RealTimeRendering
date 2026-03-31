
#include "Texture.hpp"
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstddef>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 textureCoords;
};

class MeshClass {
public:
  std::vector<Vertex> m_vertices;
  std::vector<glm::uint> m_indices;
  std::vector<TextureClass *> m_textures;
  uint VAO, VBO, EBO;
  MeshClass(const std::vector<Vertex> &vertices,
            const std::vector<uint> &indecies,
            const std::vector<TextureClass *> &textures = {})
      : m_vertices(vertices), m_indices(indecies), m_textures(textures) {
    setup();
  }

  void draw_meshes(uint shaderID, bool useTexture = true) const {
    uint diffuseNr = 1;
    uint specularNr = 1;
    uint normalNr = 1;
    std::cout << "Drawing mesh with " << m_textures.size() << " textrues"
              << std::endl;
    if(useTexture) {    
    for (size_t i = 0; i < m_textures.size(); i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      std::string number;
      std::string name = m_textures[i]->m_type;
      number = name == "texture_diffuse"    ? std::to_string(diffuseNr++)
               : name == "texture_specular" ? std::to_string(specularNr++)
               : name == "texture_normal"   ? std::to_string(normalNr++)
                                            : 0;
      std::string uniformName = name + number;
      std::cout << "Binding " << uniformName << " to texture unit " << i
                << " (ID : " << m_textures[i]->ID << " ) " << std::endl;
      glUniform1i(glGetUniformLocation(shaderID, uniformName.c_str()), i);
      glBindTexture(GL_TEXTURE_2D, m_textures[i]->ID);
    }}

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Reset to default
    glActiveTexture(GL_TEXTURE0);
  }

private:
  void setup() {

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex),
                 m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 m_indices.size() * sizeof(unsigned int), m_indices.data(),
                 GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, normal));

    // TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, textureCoords));
  }
};

class ModelClass {
public:
  std::vector<MeshClass> m_meshes;
  std::vector<TextureClass *> m_loadedTextures;
  std::string m_directory;
  ModelClass(const std::string &path) { load(path); }
  ~ModelClass() {
    for (auto *texture : m_loadedTextures) {
      delete texture;
    }
  }
  void drawModel(uint shaderID, bool useTexture = true) {
    for (const auto &mesh : m_meshes) {
      std::cout << "Mesh rendering" << std::endl;
      mesh.draw_meshes(shaderID, useTexture);
    }
  }

private:
  void load(const std::string &path) {
    Assimp::Importer importer;
    const aiScene *ai_scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_GenNormals |
                  aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs);
    if (!ai_scene || !ai_scene->mRootNode ||
        ai_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
      throw std::runtime_error("Assimp model loading failed : " +
                               std::string(importer.GetErrorString() +
                                           std::string("\n For model : ") +
                                           path));
    };
    std::cout << "\n=== MODEL TEXTURE ANALYSIS ===" << std::endl;
    std::cout << "Number of materials: " << ai_scene->mNumMaterials
              << std::endl;

    for (unsigned int i = 0; i < ai_scene->mNumMaterials; i++) {
      aiMaterial *material = ai_scene->mMaterials[i];

      std::cout << "\nMaterial " << i << ":" << std::endl;
      std::cout << "  Diffuse textures: "
                << material->GetTextureCount(aiTextureType_DIFFUSE)
                << std::endl;
      std::cout << "  Specular textures: "
                << material->GetTextureCount(aiTextureType_SPECULAR)
                << std::endl;
      std::cout << "  Normal textures: "
                << material->GetTextureCount(aiTextureType_NORMALS)
                << std::endl;
      std::cout << "  Height textures: "
                << material->GetTextureCount(aiTextureType_HEIGHT) << std::endl;
    }
    std::cout << "==============================\n" << std::endl;

    m_directory = path.substr(0, path.find_last_of('/'));
    processNode(ai_scene->mRootNode, ai_scene);

    std::cout << "Total textures loaded: " << m_loadedTextures.size()
              << std::endl;
  }
  void processNode(aiNode *node, const aiScene *scene) {
    for (uint i = 0; i < node->mNumMeshes; i++) {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      m_meshes.emplace_back(processMesh(mesh, scene));
    }
    for (uint i = 0; i < node->mNumChildren; i++) {
      processNode(node->mChildren[i], scene);
    }
  }

  MeshClass processMesh(aiMesh *mesh_node, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<uint> indices;
    std::vector<TextureClass *> textures;

    // Process vertices
    for (unsigned int i = 0; i < mesh_node->mNumVertices; i++) {
      Vertex v;
      v.position = {mesh_node->mVertices[i].x, mesh_node->mVertices[i].y,
                    mesh_node->mVertices[i].z};
      v.normal = {mesh_node->mNormals[i].x, mesh_node->mNormals[i].y,
                  mesh_node->mNormals[i].z};

      v.textureCoords = mesh_node->mTextureCoords[0]
                            ? glm::vec2(mesh_node->mTextureCoords[0][i].x,
                                        mesh_node->mTextureCoords[0][i].y)
                            : glm::vec2(0.0f);

      vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh_node->mNumFaces; i++) {
      for (unsigned int j = 0; j < mesh_node->mFaces[i].mNumIndices; j++)
        indices.push_back(mesh_node->mFaces[i].mIndices[j]);
    }

    if (mesh_node->mMaterialIndex >= 0) {
      aiMaterial *material = scene->mMaterials[mesh_node->mMaterialIndex];

      std::cout << "\nProcessing material " << mesh_node->mMaterialIndex
                << std::endl;

      // Load diffuse textures
      std::vector<TextureClass *> diffuseMaps = loadMaterialTextures(
          material, aiTextureType_DIFFUSE, "texture_diffuse");
      textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

      // Load specular textures
      std::vector<TextureClass *> specularMaps = loadMaterialTextures(
          material, aiTextureType_SPECULAR, "texture_specular");
      textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

      // Load normal maps - try both NORMALS and HEIGHT
      std::vector<TextureClass *> normalMaps = loadMaterialTextures(
          material, aiTextureType_NORMALS, "texture_normal");

      if (normalMaps.empty()) {
        // Try HEIGHT type (common for GLTF, OBJ formats)
        normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT,
                                          "texture_normal");
        if (!normalMaps.empty()) {
          std::cout << "  Loaded normal map from HEIGHT texture type"
                    << std::endl;
        }
      }
      textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    }

    std::cout << "Mesh created with " << textures.size() << " textures"
              << std::endl;
    return MeshClass(vertices, indices, textures);
  }
  std::vector<TextureClass *>
  loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                       const std::string &typeName) {
    std::vector<TextureClass *> textures;
    unsigned int textureCount = mat->GetTextureCount(type);

    if (textureCount > 0) {
      std::cout << "  Loading " << textureCount
                << " texture(s) of type: " << typeName << std::endl;
    }

    for (unsigned int i = 0; i < textureCount; i++) {
      aiString str;
      mat->GetTexture(type, i, &str);
      std::string filename = m_directory + "/" + std::string(str.C_Str());

      std::cout << "    Path: " << str.C_Str() << std::endl;

      // Check if texture was already loaded
      bool skip = false;
      for (auto *loadedTex : m_loadedTextures) {
        if (loadedTex->m_path == str.C_Str()) {
          textures.push_back(loadedTex);
          skip = true;
          std::cout << "    -> Reusing already loaded texture (ID: "
                    << loadedTex->ID << ")" << std::endl;
          break;
        }
      }

      if (!skip) {
        try {
          TextureClass *texture = new TextureClass(filename);
          texture->m_type = typeName;
          texture->m_path = str.C_Str();
          std::cout << "    -> Loaded new texture (ID: " << texture->ID << ")"
                    << std::endl;

          m_loadedTextures.push_back(texture);
          textures.push_back(texture);
        } catch (const std::exception &e) {
          std::cerr << "    -> ERROR: Failed to load texture: " << e.what()
                    << std::endl;
        }
      }
    }

    return textures;
  }
};
