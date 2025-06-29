#include <glb_renderer.h>

bool GLBModel::load(AAssetManager *assetManager, const std::string &filename) {
    LOG_TID("THREAD_TEST : Thread of load");

    /* Load GLB file from the apk bundle */
//    std::string model_path = "models/jap_rest.glb";
//    AAsset* asset = AAssetManager_open(assetManager, model_path.c_str(), AASSET_MODE_BUFFER);
//    if(!asset) {
//        LOGE("NAT_ERROR : Failed to open asset in GLBModel::load : %s", model_path.c_str());
//        return false;
//    }
//
//    size_t length = AAsset_getLength(asset);
//    std::vector<uint8_t> buffer_a(length);
//    AAsset_read(asset, buffer_a.data(), length);
//    AAsset_close(asset);
    LOGI("SANJU : Before release()");
    release();
    LOGI("SANJU : After release()");

    /* Read the file from local file system into memory */
    /* Reads the file as binary with no character translation. Also keeps the byte order */
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        LOGE("NAT ERROR : Failed to open the model file : %s", filename.c_str());
        return false;
    }
    LOGI("SANJU : Model is successfully opened");

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer_f(size);
    if(!file.read( (buffer_f.data()), size)) {
        LOGE("NAT ERROR : Failed to read the model file : %s", filename.c_str());
        return false;
    }

    std::streamsize bytesRead = file.gcount();
    if(bytesRead != size) {
        LOGE("NAT_ERROR : Failed to read the entire file : expected(%lld) | got(%lld)", (long long)size, (long long)bytesRead);
        return false;
    } else {
        LOGI("MEM_TEST : size = %d", size);
    }

    LOGI("SANJU : Model is successfully read");

//    /* First checking if  the byte size is the same */
//    if(size != length) {
//        LOGE("MEM_TEST : buffer_f and buffer_a size not the same : %zu , %zu", buffer_f.size(), buffer_a.size());
//    }
//
//    /* Testing if buffer_a and buffer_f are the same bitwise */
//    bool fast_compare = memcmp(
//            buffer_a.data(),
//            buffer_f.data(),
//            size
//            ) == 0;
//    if(fast_compare) {
//        LOGI("MEM_TEST : Both are same bitwise");
//    }

    /* Parse the file using Assimp */
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(
            buffer_f.data(), size,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices |
            aiProcess_OptimizeMeshes |
            aiProcess_EmbedTextures |
            aiProcess_FindInstances, "glb"
            );

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOGE("NAT_ERROR : Assimp error : %s", importer.GetErrorString());
        return false;
    }

    LOGI("SANJU : scene->mNumTextures = %d", scene->mNumTextures);
    LOGI("SANJU : scene->HasTextures() = %d", scene->HasTextures());

    /* Process embedded textures */
    processTextures(scene);

    /* Process nodes recursively */
    processNode(scene->mRootNode, scene);

    /* Computing bounding box */
    minBounds = glm::vec3(FLT_MAX);
    maxBounds = glm::vec3(-FLT_MAX);
    for(unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        for(unsigned int v = 0; v < mesh->mNumVertices; v++) {
            aiVector3D pos = mesh->mVertices[v];
            minBounds.x = std::min(minBounds.x, pos.x);
            minBounds.y = std::min(minBounds.y, pos.y);
            minBounds.z = std::min(minBounds.z, pos.z);
            maxBounds.x = std::max(maxBounds.x, pos.x);
            maxBounds.y = std::max(maxBounds.y, pos.y);
            maxBounds.z = std::max(maxBounds.z, pos.z);
        }
     }

    /* Determining the dominant up-axis */
    float yHeight = maxBounds.y - minBounds.y;
    float zHeight = maxBounds.z - minBounds.z;
    isZUp = (zHeight < yHeight);
    LOGI("SANJU : %f , %f", zHeight, yHeight);

    if(isZUp) {
        LOGI("SANJU : isZUp true");
        axis_correction  = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    return true;
}

void GLBModel::draw(const float *mvp) {
    glUseProgram(program);

    glEnable(GL_DEPTH_TEST);
    /* Accept fragment if it closer to the camera than the former one */
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Set uniforms */
    GLuint mvpLoc = glGetUniformLocation(program, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);

    /* Drawing all meshes */
    for(const auto& mesh : mMeshes) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.textureId);

        if(glGetError() != GL_NO_ERROR) {
            LOGE("NAT_ERROR : Error binding texture %d", mesh.textureId);
        }

        GLint texLoc = glGetUniformLocation(program, "uTexture");
        glUniform1i(texLoc, 0);

        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void GLBModel::release() {
    for(auto& mesh : mMeshes) {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
    }
    mMeshes.clear();

    for(auto& tex : mTextures) {
        glDeleteTextures(1, &tex.second);
    }
    mTextures.clear();
}

void GLBModel::processNode(aiNode *node, const aiScene *scene) {
    /* Process all meshes in node */
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        mMeshes.push_back(processMesh(mesh, scene));
    }

    /* Process children recursively */
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

GLBModel::Mesh GLBModel::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    GLuint textureId = 0;

    /* Process vertices */
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        /* Position */
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);

        /* Normals */
        vertices.push_back(mesh->mNormals[i].x);
        vertices.push_back(mesh->mNormals[i].y);
        vertices.push_back(mesh->mNormals[i].z);

        /* Texture coordinates */
        if(mesh->mTextureCoords[0]){
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    /* Process indices */
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    /* Process material/texture */
    if(mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString texPath;
        if(material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            LOGI("SANJU : Inside GetTexture()");
            std::string pathStr(texPath.C_Str());
            LOGI("SANJU : %s", pathStr.c_str());
            if(mTextures.find(pathStr) != mTextures.end()) {
                textureId = mTextures[pathStr];
                LOGI("SANJU : textureId = %d", textureId);
            }
        }
    }

    /* Create default texture if none found */
    if(textureId == 0) {
        textureId = createDefaultTexture();
    }

    return setupMesh(vertices, indices, textureId);
}

GLBModel::Mesh GLBModel::setupMesh(const std::vector<float> &vertices, const std::vector<unsigned int> &indices,
                    GLuint textureId) {
    Mesh mesh;
    mesh.textureId = textureId;
    mesh.indexCount = indices.size();

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    /* Vertex buffer */
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    /* Element buffer */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    /* Vertex attributes */
    /* Position (location n= 0) */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    /* Normal (location = 1) */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    /* Texture Coords. (location = 2) */
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
    return mesh;
}

void GLBModel::processTextures(const aiScene *scene) {
    for(unsigned int i = 0; i < scene->mNumTextures; i++) {
        aiTexture* texture = scene->mTextures[i];
        std::string texName = "*" + std::to_string(i);
        mTextures[texName] = createTextureFromEmbedded(texture);
    }
}

GLuint GLBModel::createTextureFromEmbedded(aiTexture *texture) {
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    /* Handle compressed texture */
    if(texture->mHeight == 0) {
        LOGI("SANJU : Compressed");
        /* Use compressed texture data directly */
        int width, height, channels;
        unsigned char* imageData = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(texture->pcData),
                texture->mWidth,
                &width, &height, &channels, STBI_rgb_alpha
                );
        if(imageData) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
            stbi_image_free(imageData);
        }
    } else {
        LOGI("SANJU : Uncompressed");
        /* Uncompressed ARGB8888 format */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->mWidth, texture->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->pcData);
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

GLuint GLBModel::createDefaultTexture() {
    LOGI("SANJU : Inside createDefaultTexture()");
    GLuint textureId;
    const uint32_t texData[] = {
            0xFFFFFFFF, 0xFF000000,
            0xFF000000, 0xFFFFFFFF
    };

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);


    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}













