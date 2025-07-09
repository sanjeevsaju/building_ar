#ifndef BUILDING_AR_GLB_RENDERER_ASYNC_H
#define BUILDING_AR_GLB_RENDERER_ASYNC_H

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
#include <fstream>
#include <sstream>
#include <sys/syscall.h>
#include <future>

#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define LOG_TAG "GLBRenderer_Async"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOG_TID(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[TID:%ld] " __VA_ARGS__, syscall(SYS_gettid))

class GLBModelAsync {
public:
    enum State {
        NOT_LOADED, LOADED, READY, ERROR
    };
    State mState = NOT_LOADED;

    void update();

    struct Mesh {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        GLuint vao, vbo, ebo;
        size_t indexCount;
        GLuint textureId;
    };

    ~GLBModelAsync() {
        LOGI("SANJU : ~GLBModelAsync() called");
        release();
    }

    bool load(const std::string& fileName);
    void draw(const float mvp[16]);
    void release();
    void setProgram(GLuint program_) { program = program_; }

private:

    std::unique_ptr<Assimp::Importer> mImporter;
    const aiScene* mScene = nullptr;
    std::future<void> loadingFuture;

    GLuint program = 0;
    std::vector<Mesh> mMeshes;
    std::unordered_map<std::string, GLuint> mTextures;

    struct textureImageData {
        int width, height, channels;
        unsigned char* imageBytes;
    };
    std::unordered_map<std::string, textureImageData> mTextureImages;

    void extractVertAndIndNode(aiNode* node, const aiScene* scene);
    Mesh extractVertAndIndMesh(aiMesh* mesh, const aiScene* scene);

    void bindMesh();
    void bindMeshToTextureNode(aiNode* node, const aiScene* scene, unsigned int& globalMeshIndex);
    void bindMeshToTextureMesh(aiMesh* mesh, const aiScene* scene, unsigned int globalMeshIndex);
    void extractTextureImages(const aiScene *scene);

    void processTextures(const aiScene* scene);
    GLuint bindTextures(aiTexture* texture, const std::string& texName);
    GLuint createDefaultTexture();
};

#endif //BUILDING_AR_GLB_RENDERER_ASYNC_H
