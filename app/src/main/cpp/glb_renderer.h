#ifndef BUILDING_AR_GLB_RENDERER_H
#define BUILDING_AR_GLB_RENDERER_H

#include <android/asset_manager.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>

#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define LOG_TAG "GLBRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class GLBModel {
public:

    /* For up axis detection and correction */
    glm::vec3 minBounds;    // Bounding box min
    glm::vec3 maxBounds;    // Bounding box max
    bool isZUp = false;
    glm::mat4 axis_correction = glm::mat4(1.0f);

    struct Mesh {
        GLuint vao, vbo, ebo;
        size_t indexCount;
        GLuint textureId;
    };

    ~GLBModel() {
        release();
    }

    bool load(AAssetManager* assetManager,const std::string& filename);
    void draw(const float mvp[16]);
    void release();
    void setProgram(GLuint program_) { program = program_; }

private:
    GLuint program = 0;
    std::vector<Mesh> mMeshes;
    std::unordered_map<std::string, GLuint> mTextures;

    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    Mesh setupMesh(const std::vector<float>& vertices,
                   const std::vector<unsigned int>& indices,
                   GLuint textureId);
    void processTextures(const aiScene* scene);
    GLuint createTextureFromEmbedded(aiTexture* texture);
    GLuint createDefaultTexture();

    GLuint createProgram(const char* vertexSrc, const char* fragmentSrc);
    GLuint compileShader(GLenum type, const char* source);
};

#endif //BUILDING_AR_GLB_RENDERER_H
