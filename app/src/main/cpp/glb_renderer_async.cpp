#include <glb_renderer_async.h>

bool GLBModelAsync::load(const std::string &fileName) {
    LOGI("SANJU : GLBModelAsync::load");
    release();

    loadingFuture = std::async(std::launch::async, [this, fileName](){
        try {
            std::ifstream file(fileName, std::ios::binary | std::ios::ate);
            if(!file.is_open()) {
                LOGE("NAT_ERROR : Failed to open the model file : %s", fileName.c_str());
                return;
            }
            LOGI("SANJU : Model file is successfully opened!");

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            /* Reading the file into memory */
            std::vector<char> buffer(size);
            if(!file.read(buffer.data(), size)) {
                LOGE("NAT_ERROR : Failed to read the model file: %s", fileName.c_str());
                return;
            }

            /* Now parsing the model using Assimp */
            mImporter = std::make_unique<Assimp::Importer>();
            mScene = mImporter->ReadFileFromMemory(
                    buffer.data(), size,
                    aiProcess_Triangulate |
                    aiProcess_GenNormals |
                    aiProcess_FlipUVs |
                    aiProcess_JoinIdenticalVertices |
                    aiProcess_OptimizeMeshes |
                    aiProcess_EmbedTextures |
                    aiProcess_FindInstances, "glb"
            );
            if(!mScene || mScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !mScene->mRootNode) {
                LOGE("NAT_ERROR : Assimp Error | %s", mImporter->GetErrorString());
                return;
            }

            LOGI("SANJU : scene->mNumTextures = %d", mScene->mNumTextures);

            /* These two methods are not depended on Gl thread and thus can be made to run in std::async() */
            // This call fills  std::vector<Mesh> mMeshes(without textureId and vao/vbo/ebo)
            LOG_TID("THREAD_TEST : Thread of GLBModelAsync::load() -> async");
            extractVertAndIndNode(mScene->mRootNode, mScene);
            extractTextureImages(mScene);
            LOGI("SANJU : mMeshes.vertices.size = %d", mMeshes.size());
            mState = LOADED;
        } catch(...) {

        }
    });
    return true;
}

void GLBModelAsync::update() {
    LOGI("SANJU : GLBModelAsync::update");
    processTextures(mScene);
    unsigned int globalMeshIndex = 0;
    bindMeshToTextureNode(mScene->mRootNode, mScene, globalMeshIndex);
    bindMesh();

    mImporter.reset();
    mScene = nullptr;
    mState = READY;
}

void GLBModelAsync::extractVertAndIndNode(aiNode *node, const aiScene *scene) {
    LOGI("SANJU : GLBModelAsync::extractVertAndIndNode");
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
//        LOGI("SANJU : Inside first loop");
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        mMeshes.push_back(extractVertAndIndMesh(mesh, scene));
    }

    /* Process child nodes */
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
//        LOGI("SANJU : Inside second loop");
        extractVertAndIndNode(node->mChildren[i], scene);
    }
}

GLBModelAsync::Mesh GLBModelAsync::extractVertAndIndMesh(aiMesh *mesh, const aiScene *scene) {
    LOGI("SANJU :  GLBModelAsync::extractVertAndIndMesh : [mesh->mNumVertices = %d]", mesh->mNumVertices);
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    /* Just to fill the Mesh{} */
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
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

        /* Texture Coordinates */
        if(mesh->mTextureCoords[0]) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    /* Process Indices */
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return Mesh {
        vertices, indices,
        vao, vbo, ebo,
        indices.size(),
        textureId
    };
}

/* Can be run on background thread using std::async() */
void GLBModelAsync::extractTextureImages(const aiScene *scene) {
    LOGI("SANJU : GLBModelAsync::extractTextureImages");
    for(unsigned  int i = 0; i < scene->mNumTextures; i++) {
        aiTexture* texture = scene->mTextures[i];
        std::string texName = "*" + std::to_string(i);
        textureImageData texImageData{};
        texImageData.imageBytes =  stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(texture->pcData),
                texture->mWidth,
                &texImageData.width, &texImageData.height, &texImageData.channels, STBI_rgb_alpha
        );

        mTextureImages[texName] = texImageData;
    }
    LOGI("SANJU : mTextureImages.size() - %d", mTextureImages.size());
}

/* Running on the GL thread */
void GLBModelAsync::processTextures(const aiScene *scene) {
    LOGI("SANJU : GLBModelAsync::processTextures");
    for(unsigned int i = 0; i < scene->mNumTextures; i++) {
        aiTexture* texture = scene->mTextures[i];
        std::string texName = "*" + std::to_string(i);
        mTextures[texName] = bindTextures(texture, texName);
    }
}

GLuint GLBModelAsync::bindTextures(aiTexture *texture, const std::string& texName) {
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    textureImageData texImgData = mTextureImages[texName];

    /* Handling compressed texture */
    if(texture->mHeight == 0) {
        if(texImgData.imageBytes) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texImgData.width, texImgData.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texImgData.imageBytes);
        }
    } else {
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

void GLBModelAsync::bindMesh() {
    for(unsigned int i = 0; i < mMeshes.size(); i++) {
        glGenVertexArrays(1, &mMeshes[i].vao);
        glGenBuffers(1, &mMeshes[i].vbo);
        glGenBuffers(1, &mMeshes[i].ebo);

        glBindVertexArray(mMeshes[i].vao);

        /* Vertex buffer */
        glBindBuffer(GL_ARRAY_BUFFER, mMeshes[i].vbo);
        glBufferData(GL_ARRAY_BUFFER, mMeshes[i].vertices.size() * sizeof(float), mMeshes[i].vertices.data(), GL_STATIC_DRAW);

        /* Element buffer */
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshes[i].ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mMeshes[i].indices.size() * sizeof(unsigned int), mMeshes[i].indices.data(), GL_STATIC_DRAW);

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
    }
}

void GLBModelAsync::bindMeshToTextureNode(aiNode* node, const aiScene* scene, unsigned int& globalMeshIndex) {
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        bindMeshToTextureMesh(mesh, scene, globalMeshIndex);
        globalMeshIndex++;
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        bindMeshToTextureNode(node->mChildren[i], scene, globalMeshIndex);
    }
}

void GLBModelAsync::bindMeshToTextureMesh(aiMesh* mesh, const aiScene* scene, unsigned int globalMeshIndex) {
    if(mesh->mMaterialIndex >= 0 ) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString texPath;
        if(material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            LOGI("SANJU : Inside GetTexture()");
            std::string pathStr(texPath.C_Str());
            LOGI("SANJU : %s", pathStr.c_str());
            if(mTextures.find(pathStr) != mTextures.end()) {
                mMeshes[globalMeshIndex].textureId = mTextures[pathStr];
                LOGI("SANJU : textureId = %d", mMeshes[globalMeshIndex].textureId);
            }
        }
    }

    if(mMeshes[globalMeshIndex].textureId == 0) {
        mMeshes[globalMeshIndex].textureId = createDefaultTexture();
    }
}

GLuint GLBModelAsync::createDefaultTexture() {
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

void GLBModelAsync::draw(const float *mvp) {
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

void GLBModelAsync::release() {
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
