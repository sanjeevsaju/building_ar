#include <model.h>
#include <glm/gtc/type_ptr.hpp>
#include <android/asset_manager.h>

bool Model::CheckGLError(const char *operation) {
    GLenum err = glGetError();
    if(err != GL_NO_ERROR) {
        lastError = std::string("OpenGL error during ") + operation + ": " + std::to_string(err);
        LOGE("NAT_ERROR : %s", lastError.c_str());
        return true;
    }
    return false;
}

GLuint Model::createDefaultTexture() {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    unsigned char pixel[] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

bool Model::LoadFromFile(AAssetManager* asset_manager ,const std::string& path) {
    lastError.clear();

    /* Open the file using standard I/O */
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        LOGE("NAT_ERROR : File from Model::LoadFromFile could not be opened : %s", path.c_str());
        return false;
    }

    /* Read file into buffer */
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if(!file.read(buffer.data(), size)){
        LOGE("NAT_ERROR : Failed to read file from Model::LoadFromFile : %s", path.c_str());
        return false;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(
            buffer.data(), size,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_OptimizeMeshes, "glb2"
            );

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        lastError = "Assimp Error : " + std::string(importer.GetErrorString());
        LOGE("NAT_ERROR : Model loading failed in LoadFromFile : %s", lastError.c_str());
        return false;
    }

    /* Extract directory path from file path for texture loading */
    size_t pos = path.find_last_of("/\\");
    directory = (pos == std::string::npos) ? "" : path.substr(0, pos);

    processNode(scene->mRootNode, scene);
    return true;
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    /* Process all meshes in the node */
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    /* Process children recursively */
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Process vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        // Position
        vertex.Position[0] = mesh->mVertices[i].x;
        vertex.Position[1] = mesh->mVertices[i].y;
        vertex.Position[2] = mesh->mVertices[i].z;

        // Normals
        if(mesh->HasNormals()) {
            vertex.Normal[0] = mesh->mNormals[i].x;
            vertex.Normal[1] = mesh->mNormals[i].y;
            vertex.Normal[2] = mesh->mNormals[i].z;
        } else {
            vertex.Normal[0] = vertex.Normal[1] = vertex.Normal[2] = 0.0f;
        }

        // Texture coordinates
        if(mesh->mTextureCoords[0]) {
            vertex.TexCoords[0] = mesh->mTextureCoords[0][i].x;
            vertex.TexCoords[1] = mesh->mTextureCoords[0][i].y;
        } else {
            vertex.TexCoords[0] = vertex.TexCoords[1] = 0.0f;
        }

        // Tangents/bitangents
        if(mesh->HasTangentsAndBitangents()) {
            vertex.Tangent[0] = mesh->mTangents[i].x;
            vertex.Tangent[1] = mesh->mTangents[i].y;
            vertex.Tangent[2] = mesh->mTangents[i].z;

            vertex.Bitangent[0] = mesh->mBitangents[i].x;
            vertex.Bitangent[1] = mesh->mBitangents[i].y;
            vertex.Bitangent[2] = mesh->mBitangents[i].z;
        } else {
            vertex.Tangent[0] = vertex.Tangent[1] = vertex.Tangent[2] = 0.0f;
            vertex.Bitangent[0] = vertex.Bitangent[1] = vertex.Bitangent[2] = 0.0f;
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Process material
    if(mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Diffuse maps
        std::vector<Texture> diffuseMaps = loadMaterialTextures(
                material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // Specular maps
        std::vector<Texture> specularMaps = loadMaterialTextures(
                material, aiTextureType_SPECULAR, "texture_specular", scene);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // Normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(
                material, aiTextureType_NORMALS, "texture_normal", scene);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // Height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(
                material, aiTextureType_HEIGHT, "texture_height", scene);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(const aiMaterial* mat,
                                                 aiTextureType type,
                                                 const std::string& typeName,
                                                 const aiScene* scene) {
    std::vector<Texture> textures;

    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);

        // Check if texture was loaded before
        bool skip = false;
        for(const auto& loaded : textures_loaded) {
            if(std::strcmp(loaded.path.data(), str.C_Str()) == 0) {
                textures.push_back(loaded);
                skip = true;
                break;
            }
        }

        if(!skip) { // Not loaded yet
            Texture texture;

            if(str.data[0] == '*') { // Embedded texture
                int index = atoi(str.C_Str() + 1);
                if(index >= 0 && index < (int)scene->mNumTextures) {
                    texture.id = TextureFromFileOrEmbedded(scene->mTextures[index]);
                    texture.type = typeName;
                    texture.path = str.C_Str();
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                }
            } else { // External texture (not supported in this implementation)
                texture.id = createDefaultTexture();
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
                LOGI("External texture not supported: %s", str.C_Str());
            }
        }
    }
    return textures;
}

GLuint Model::TextureFromFileOrEmbedded(const aiTexture* embedTexture) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int width, height, channels = 0;
    unsigned char* data = nullptr;

    if(embedTexture->mHeight == 0) {
        // Compressed texture
        data = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(embedTexture->pcData),
                embedTexture->mWidth,
                &width, &height, &channels, 0);
    } else {
        // Uncompressed ARGB8888 -> RGBA8888
        width = embedTexture->mWidth;
        height = embedTexture->mHeight;
        data = new unsigned char[width * height * 4];

        for(int i = 0; i < width * height; ++i) {
            data[i*4+0] = embedTexture->pcData[i].r;
            data[i*4+1] = embedTexture->pcData[i].g;
            data[i*4+2] = embedTexture->pcData[i].b;
            data[i*4+3] = embedTexture->pcData[i].a;
        }
        channels = 4;
    }

    if(data) {
        GLenum format = GL_RGBA;
        if(channels == 1) format = GL_RED;
        else if(channels == 3) format = GL_RGB;
        else if(channels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                     format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if(embedTexture->mHeight == 0) {
            stbi_image_free(data);
        } else {
            delete[] data;
        }
    } else {
        LOGE("Failed to load embedded texture");
        return createDefaultTexture();
    }

    return textureID;
}

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<unsigned int>& indices,
           const std::vector<Texture>& textures)
        : vertices(vertices), indices(indices), textures(textures)
{
    setupMesh();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Load data into vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Position));

    // Vertex Normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Normal));

    // Vertex Texture Coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, TexCoords));

    // Vertex Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Tangent));

    // Vertex Bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Bitangent));

    glBindVertexArray(0);
}

void Mesh::Draw(GLuint shaderProgram) {
    // Bind appropriate textures
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;

    for(unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i); // Active proper texture unit

        // Retrieve texture number (the N in diffuse_textureN)
        std::string number;
        std::string name = textures[i].type;
        if(name == "texture_diffuse")
            number = std::to_string(diffuseNr++);
        else if(name == "texture_specular")
            number = std::to_string(specularNr++);
        else if(name == "texture_normal")
            number = std::to_string(normalNr++);
        else if(name == "texture_height")
            number = std::to_string(heightNr++);

        // Set the sampler to the correct texture unit
        glUniform1i(glGetUniformLocation(shaderProgram, (name + number).c_str()), i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Set everything back to defaults
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::Cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // Note: Textures are managed by Model class
}

void Model::Render(const glm::mat4& mvp) {
    if(shaderProgram == 0) return;

    glEnable(GL_DEPTH_TEST);
    /* Accept fragment if it closer to the camera than the former one */
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mvp"), 1, GL_FALSE,
                       glm::value_ptr(mvp));

    for(auto& mesh : meshes) {
        mesh.Draw(shaderProgram);
    }

    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Model::Cleanup() {
    for(auto& mesh : meshes) {
        mesh.Cleanup();
    }

    // Delete loaded textures
    for(auto& texture : textures_loaded) {
        glDeleteTextures(1, &texture.id);
    }
    textures_loaded.clear();
}


bool Model::SetShaderProgram(GLuint program) {
    if(program == 0) {
        lastError = "Invalid shader program (0)";
        return false;
    }
    shaderProgram = program;
    return true;
}

Model::~Model() {
    Cleanup();
}