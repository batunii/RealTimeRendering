#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include "Texture.hpp"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture*> textures;
    unsigned int VAO{}, VBO{}, EBO{};

    Mesh(const std::vector<Vertex> &v, const std::vector<unsigned int> &i,
         const std::vector<Texture*> &tex = {})
        : vertices(v), indices(i), textures(tex) {
        setup();
    }

    void draw(unsigned int shaderID) const {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        
        std::cout << "Drawing mesh with " << textures.size() << " textures" << std::endl;
        
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            
            std::string number;
            std::string name = textures[i]->type;
            
            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);
            else if (name == "texture_normal")
                number = std::to_string(normalNr++);
            
            std::string uniformName = name + number;
            std::cout << "  Binding " << uniformName << " to texture unit " << i 
                      << " (ID: " << textures[i]->ID << ")" << std::endl;
            
            glUniform1i(glGetUniformLocation(shaderID, uniformName.c_str()), i);
            glBindTexture(GL_TEXTURE_2D, textures[i]->ID);
        }
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
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
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                     vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                     indices.data(), GL_STATIC_DRAW);

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)offsetof(Vertex, Normal));

        // TexCoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)offsetof(Vertex, TexCoords));

        // Tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)offsetof(Vertex, Tangent));

        // Bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void *)offsetof(Vertex, Bitangent));

        glBindVertexArray(0);
    }
};

class Model {
public:
    std::vector<Mesh> meshes;
    std::vector<Texture*> loadedTextures;
    std::string directory;

    explicit Model(const std::string &path) { load(path); }
    
    ~Model() {
        for (auto* tex : loadedTextures) {
            delete tex;
        }
    }

    void draw(unsigned int shaderID) const {
        for (const auto &m : meshes)
            m.draw(shaderID);
    }

private:
    void load(const std::string &path) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(
            path, 
            aiProcess_Triangulate | 
            aiProcess_GenNormals |
            aiProcess_JoinIdenticalVertices | 
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace  // Important for normal mapping!
        );

        if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            throw std::runtime_error("Assimp failed to load model: " + 
                                   std::string(importer.GetErrorString()));
        }

        std::cout << "\n=== MODEL TEXTURE ANALYSIS ===" << std::endl;
        std::cout << "Number of materials: " << scene->mNumMaterials << std::endl;
        
        for(unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* material = scene->mMaterials[i];
            std::cout << "\nMaterial " << i << ":" << std::endl;
            std::cout << "  Diffuse textures: " << material->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
            std::cout << "  Specular textures: " << material->GetTextureCount(aiTextureType_SPECULAR) << std::endl;
            std::cout << "  Normal textures: " << material->GetTextureCount(aiTextureType_NORMALS) << std::endl;
            std::cout << "  Height textures: " << material->GetTextureCount(aiTextureType_HEIGHT) << std::endl;
        }
        std::cout << "==============================\n" << std::endl;       

        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
        
        std::cout << "Total textures loaded: " << loadedTextures.size() << std::endl;
    }

    void processNode(aiNode *node, const aiScene *scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.emplace_back(processMesh(mesh, scene));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture*> textures;

        // Check if mesh has tangents and bitangents
        bool hasTangents = mesh->HasTangentsAndBitangents();
        if (!hasTangents) {
            std::cout << "Warning: Mesh does not have tangents/bitangents!" << std::endl;
        }

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex v;
            v.Position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
            v.Normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
            
            v.TexCoords = mesh->mTextureCoords[0]
                ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                : glm::vec2(0.0f);
            
            // Load tangents and bitangents if available
            if (hasTangents) {
                v.Tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
                v.Bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
            } else {
                v.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                v.Bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            
            vertices.push_back(v);
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
                indices.push_back(mesh->mFaces[i].mIndices[j]);
        }

        // Load material textures
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            
            std::cout << "\nProcessing material " << mesh->mMaterialIndex << std::endl;
            
            // Load diffuse textures
            std::vector<Texture*> diffuseMaps = loadMaterialTextures(
                material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            
            // Load specular textures
            std::vector<Texture*> specularMaps = loadMaterialTextures(
                material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            
            // Load normal maps - try both NORMALS and HEIGHT
            std::vector<Texture*> normalMaps = loadMaterialTextures(
                material, aiTextureType_NORMALS, "texture_normal");
            
            if (normalMaps.empty()) {
                // Try HEIGHT type (common for GLTF, OBJ formats)
                normalMaps = loadMaterialTextures(
                    material, aiTextureType_HEIGHT, "texture_normal");
                if (!normalMaps.empty()) {
                    std::cout << "  Loaded normal map from HEIGHT texture type" << std::endl;
                }
            }
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        }

        std::cout << "Mesh created with " << textures.size() << " textures" << std::endl;
        return Mesh(vertices, indices, textures);
    }

    std::vector<Texture*> loadMaterialTextures(aiMaterial *mat, 
                                               aiTextureType type, 
                                               const std::string& typeName) {
        std::vector<Texture*> textures;
        unsigned int textureCount = mat->GetTextureCount(type);
        
        if (textureCount > 0) {
            std::cout << "  Loading " << textureCount << " texture(s) of type: " 
                      << typeName << std::endl;
        }
        
        for (unsigned int i = 0; i < textureCount; i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            std::string filename = directory + "/" + std::string(str.C_Str());
            
            std::cout << "    Path: " << str.C_Str() << std::endl;
            
            // Check if texture was already loaded
            bool skip = false;
            for (auto* loadedTex : loadedTextures) {
                if (loadedTex->path == str.C_Str()) {
                    textures.push_back(loadedTex);
                    skip = true;
                    std::cout << "    -> Reusing already loaded texture (ID: " 
                              << loadedTex->ID << ")" << std::endl;
                    break;
                }
            }
            
            if (!skip) {
                try {
                    Texture *texture = new Texture(filename);
                    texture->type = typeName;
                    texture->path = str.C_Str();
                    std::cout << "    -> Loaded new texture (ID: " << texture->ID << ")" << std::endl;
                    
                    loadedTextures.push_back(texture);
                    textures.push_back(texture);
                } catch (const std::exception& e) {
                    std::cerr << "    -> ERROR: Failed to load texture: " << e.what() << std::endl;
                }
            }
        }
        
        return textures;
    }
};
