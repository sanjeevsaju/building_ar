#ifndef MY_NATIVE_APP_MODEL_H
#define MY_NATIVE_APP_MODEL_H

#include <vector>
#include <fstream>
#include <string>
#include "glm/glm.hpp"
#include <android/asset_manager.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "stb_image.h"

#define LOG_TAG "Model"
#define LOG_TID(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[TID:%ld] " __VA_ARGS__, syscall(SYS_gettid))
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct Vertex {
    float Position[3];
    float Normal[3];
    float TexCoords[2];
    float Tangent[3];
    float Bitangent[3];
};

struct Texture {
    GLuint id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    /* Mesh data */
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    /* OpenGL buffers */
    GLuint  VAO, VBO, EBO;

    /* Constructor */
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<unsigned int>& indices,
         const std::vector<Texture>& textures);

    /* Render the mesh */
    void Draw(GLuint shaderProgram);

    /* Cleanup OpenGL resources */
    void Cleanup();

private:
    /* Initialize all the buffer objects/arrays */
    void setupMesh();
};

class Model {
public:
    ~Model();
    bool LoadFromFile(AAssetManager* asset_manager, const std::string& path);
    void Render(const glm::mat4& mvp);
    void Cleanup();
    bool SetShaderProgram(GLuint program);
    const std::string& GetLastError() const { return lastError; }

private:

    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;
    GLuint shaderProgram = 0;
    std::string lastError;

    /* Assimp processing functions */
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(
            const aiMaterial* mat,
            aiTextureType type,
            const std::string& typeName,
            const aiScene* scene);
    GLuint TextureFromFileOrEmbedded(const aiTexture* embedTexture);
    GLuint createDefaultTexture();

    bool CheckGLError(const char* operation);
};

#endif //MY_NATIVE_APP_MODEL_H