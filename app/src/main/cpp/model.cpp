#include "model.h"
#include "glm/gtc/type_ptr.hpp"
#include <android/asset_manager.h>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

bool Model::CheckGLError(const char *operation) {
    GLenum err = glGetError();
    if(err != GL_NO_ERROR) {
        lastError = std::string("OpenGL error during ") + operation + ": " + std::to_string(err);
        LOGE("NAT_ERROR : %s", lastError.c_str());
        return true;
    }
    return false;
}

bool Model::LoadWithAssimp(AAssetManager *mgr, const char *modelPath) {
    lastError.clear();
    Assimp::Importer importer;

    AAsset* objAsset = AAssetManager_open(mgr, modelPath, AASSET_MODE_BUFFER);

    /* Error Handling */
    if(!objAsset) {
        lastError = "Failed to open obj asset: " + std::string(modelPath);
        LOGE("NAT_ERROR : %s", lastError.c_str());
        return false;
    }

    const void* objData = AAsset_getBuffer(objAsset);
    size_t objSize = AAsset_getLength(objAsset);

    /* Error handling */
    if(!objData || objSize == 0) {
        lastError = "Empty or invalid obj asset data: " + std::string(modelPath);
        LOGE("NAT_ERROR : %s", lastError.c_str());
        AAsset_close(objAsset);
        return false;
    }

    std::string mtlPath = std::string(modelPath).substr(0, std::string(modelPath).find_last_of('.')) + ".mtl";
    AAsset* mtlAsset = AAssetManager_open(mgr, mtlPath.c_str(), AASSET_MODE_BUFFER);

    if(!mtlAsset) {
        lastError = "Failed to open mtl asset: " + std::string(modelPath);
        LOGE("NAT_ERROR : %s", lastError.c_str());
        return false;
    }

    const void* mtlData = AAsset_getBuffer(mtlAsset);
    ssize_t mtlSize = AAsset_getLength(mtlAsset);


    if(!mtlData || mtlSize == 0) {
        lastError = "Empty or invalid mtl asset data: " + std::string(mtlPath);
        LOGE("NAT_ERROR : %s", lastError.c_str());
        AAsset_close(mtlAsset);
        return false;
    }

    /* Combining both files into a single memory buffer for Assimp */
    std::vector<char> combinedData(objSize + mtlSize);
    memcpy(combinedData.data(), objData, objSize);
    memcpy(combinedData.data() + objSize, mtlData, mtlSize);

    /* The pHint has some kind of significance here */
    const aiScene* scene = importer.ReadFileFromMemory(
            combinedData.data(), combinedData.size(),
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenNormals |
            aiProcess_OptimizeMeshes, "obj"
    );

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        lastError = "Assimp Importer error :" + std::string(modelPath);
        LOGE("NAT_ERROR : %s", lastError.c_str());
        AAsset_close(objAsset);
        AAsset_close(mtlAsset);
        LOGI("SANJU : LoadWithAssimp : Importer failed");
        return false;
    }

    if(!ProcessNode(mgr, scene->mRootNode, scene)) {
        LOGE("NAT_ERROR : %s", lastError.c_str());
        AAsset_close(objAsset);
        AAsset_close(mtlAsset);
        return false;
    }

    AAsset_close(objAsset);
    AAsset_close(mtlAsset);
    return true;
}

bool Model::ProcessNode(AAssetManager *mgr, aiNode *node, const aiScene *scene) {
    lastError.clear();

    /* Process node's meshes */
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        Mesh processed = ProcessMesh(mgr, mesh, scene);
        if(processed.vao == 0 || processed.vbo == 0 || processed.ebo == 0) {
            lastError = "Failed to process mesh";
            return false;
        }
        meshes.push_back(processed);
    }

    /* Process children recursively */
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        if(!ProcessNode(mgr, node->mChildren[i], scene)) {
            return false;
        }
    }

    return true;
}

Model::Mesh Model::ProcessMesh(AAssetManager *mgr, aiMesh *mesh, const aiScene *scene) {
    Mesh result;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    /* Process vertices */
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        /* Position */
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);

        /* Texture Coordinates */
        if(mesh->mTextureCoords[0]) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }

        /* Normals */
        vertices.push_back(mesh->mNormals[i].x);
        vertices.push_back(mesh->mNormals[i].y);
        vertices.push_back(mesh->mNormals[i].z);
    }

    /* Process Indices */
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    /* Process Material */
    if(mesh->mMaterialIndex >= 0) {
        LOGI("SANJU : mMaterialIndex = %d", mesh->mMaterialIndex);
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        result.texture = LoadMaterialTexture(mgr, material, aiTextureType_DIFFUSE);
    }

    /* Setup Buffers */
    glGenVertexArrays(1, &result.vao);
    glGenBuffers(1, &result.vbo);
    glGenBuffers(1, &result.ebo);

    if(result.vao == 0 || result.vbo == 0 || result.ebo == 0) {
        LOGE("NAT_ERROR : Failed to generate OpenGL Buffers");
        return result;
    }

    glBindVertexArray(result.vao);

    /* VBO */
    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    /* EBO */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    /* Attributes */
    /* Position */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    /* TexCoords */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    /* Normals */
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

    glBindVertexArray(0);
    result.indexCount = indices.size();

    /* Checking for Errors */
    if(CheckGLError("Process Mesh Buffers")){
        return Mesh{};
    }

    return result;
}

GLuint Model::LoadMaterialTexture(AAssetManager *mgr, const aiMaterial *mat, aiTextureType type) {
    aiString str;
    mat->GetTexture(type, 0, &str);
    LOGI("SANJU : (LoadMaterialTexture) : str : %s", str.C_Str());
    std::string fullPath = "" + std::string(str.C_Str());
    return TextureFromFile(mgr, fullPath.c_str());
}

GLuint Model::TextureFromFile(AAssetManager *mgr, const char *path) {
    GLuint textureID = 0;

    if(!path || strlen(path) == 0) {
        LOGE("NAT_ERROR : Invalid Texture Path");
        return textureID;
    }

    std::string fullPath = "textures/" + std::string(path);
    AAsset* asset = AAssetManager_open(mgr, fullPath.c_str(), AASSET_MODE_BUFFER);
    if(!asset) {
        LOGE("NAT_ERROR : Texture not found: %s", path);
        return textureID;
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load_from_memory(
            static_cast<const unsigned char*>(AAsset_getBuffer(asset)),
            AAsset_getLength(asset),
            &width, &height, &channels, STBI_rgb_alpha
    );

    if(!data) {
        LOGE("NAT_ERROR : Failed to load texture data : %s", path);
        AAsset_close(asset);
        return textureID;
    }

    AAsset_close(asset);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    if(CheckGLError("Texture Loading")) {
        LOGE("NAT_ERROR : Texture Loading");
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }

    return textureID;
}

void Model::Render(const glm::mat4 &mvp) {
    if(shaderProgram == 0) return;

    glEnable(GL_DEPTH_TEST);
    /* Accept fragment if it closer to the camera than the former one */
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shaderProgram);

    GLuint mvpLocation = glGetUniformLocation(shaderProgram, "mvp");
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));

    for(auto& mesh : meshes) {
        if(mesh.texture != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.texture);
            glUniform1i(glGetUniformLocation(shaderProgram,"textureSampler"), 0);
        }

        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Model::Cleanup() {
    for(auto& mesh : meshes) {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
        if(mesh.texture) glDeleteTextures(1, &mesh.texture);
    }
}

bool Model::SetShaderProgram(GLuint program) {
    if(program == 0) {
        lastError = "Invalid shader program (0)";
        return false;
    }
    shaderProgram = program;
    return true;
}