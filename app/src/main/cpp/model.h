#ifndef MY_NATIVE_APP_MODEL_H
#define MY_NATIVE_APP_MODEL_H

#include <vector>
#include <string>
#include "glm/glm.hpp"
#include <android/asset_manager.h>
#include <android/log.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include "assimp/scene.h"

#include "stb_image.h"

#define LOG_TAG "Model"
#define LOG_TID(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[TID:%ld] " __VA_ARGS__, syscall(SYS_gettid))
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class Model {
public:

    bool LoadWithAssimp(AAssetManager* mgr, const char* modelPath);
    void Render(const glm::mat4& mvp);
    void Cleanup();
    bool SetShaderProgram(GLuint program);

    const std::string& GetLastError() const { return lastError; }

private:

    struct Mesh {
        GLuint vao, vbo, ebo;
        GLuint texture;
        int indexCount;
    };

    std::vector<Mesh> meshes;
    GLuint shaderProgram = 0;
    std::string lastError;

    bool ProcessNode(AAssetManager* mgr, aiNode* node, const aiScene* scene);
    Mesh ProcessMesh(AAssetManager* mgr, aiMesh* mesh, const aiScene* scene);
    GLuint LoadMaterialTexture(AAssetManager* mgr, const aiMaterial* mat, aiTextureType type);
    GLuint TextureFromFile(AAssetManager* mgr, const char* path);

    bool CheckGLError(const char* operation);
};

#endif //MY_NATIVE_APP_MODEL_H