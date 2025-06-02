#include "arcore_manager.h"

void ARCoreManager::TransformPoint(const float model_matrix[16], const float local_point[3], float world_point[3]) {
    /* Convert the local point to homogeneous coordinates */
    float local_point_homogeneous[4] = {local_point[0], local_point[1], local_point[2], 1.0f};
    float result[4] = {0};

    /* Perform matrix multiplication : result = model_matrix * local_point_homogeneous */
    /* The model_matrix is in column major order */
    /* I think this can be optimized using SIMD */
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            result[i] += model_matrix[j * 4 + i] * local_point_homogeneous[j];
        }
    }

    world_point[0] = result[0];
    world_point[1] = result[1];
    world_point[2] = result[2];
}

bool ARCoreManager::IsDepthSupported() {
    if(!ar_session) return false;

    ArConfig* ar_config = nullptr;
    ArConfig_create(ar_session, &ar_config);
    bool isSupported = false;
    ArSession_isDepthModeSupported(ar_session, AR_DEPTH_MODE_AUTOMATIC,
                                   reinterpret_cast<int32_t *>(&isSupported));
    return isSupported;
}

void ARCoreManager::RotateCube(float degrees) {
    cube_rotation_angle += glm::radians(degrees);
}

void ARCoreManager::ScaleCube(float scale) {
    scaling_factor += scale;
}

void ARCoreManager::DrawVector(glm::vec3 start, glm::vec3 end) {
    test_x_s = start[0];
    test_y_s = start[1];
    test_z_s = start[2];

    test_x_e = end[0];
    test_y_e = end[1];
    test_z_e = end[2];
}

void ARCoreManager::OnTouch(float x, float y) {
//    LOG_TID("SANJU : ARCoreManager::OnTouch - ");
    if(ar_session == nullptr || ar_frame == nullptr) return;

    /* Reset the translation vector */
    cube_translation_vector = glm::vec3(0.0f);

    ArHitResultList* hit_result_list = nullptr;
    ArHitResultList_create(ar_session, &hit_result_list);
    ArFrame_hitTest(ar_session, ar_frame, x, y, hit_result_list);

    int32_t hit_result_list_size = 0;
    ArHitResultList_getSize(ar_session, hit_result_list, &hit_result_list_size);

    if(hit_result_list_size > 0) {
        ArHitResult* hit_result = nullptr;
        ArHitResult_create(ar_session, &hit_result);
        ArHitResultList_getItem(ar_session, hit_result_list, 0, hit_result);

        ArTrackable* trackable = nullptr;
        ArHitResult_acquireTrackable(ar_session, hit_result, &trackable);
        ArTrackableType trackableType;
        ArTrackable_getType(ar_session, trackable, &trackableType);

        if(trackableType == AR_TRACKABLE_PLANE) {
            ArPose* pose = nullptr;
            ArPose_create(ar_session, nullptr, &pose);
            ArHitResult_getHitPose(ar_session, hit_result, pose);

            float pose_matrix[16];
            ArPose_getMatrix(ar_session, pose, pose_matrix);

            plane_normal = glm::vec3(pose_matrix[4], pose_matrix[5], pose_matrix[6]);
            plane_normal = glm::normalize(plane_normal);

            hit_pose_matrix = glm::make_mat4(pose_matrix);

//            hit_pose_matrix = glm::translate(hit_pose_matrix, glm::vec3(0.0f, 0.5 * scaling_factor, 0.0f));

            model_place = true;
        }
    }
}

void ARCoreManager::TranslateCube(float x, float y, float z) {
    /* Homogenous coordinate(w) set to 0 -
     * because we don't want the translation part of the view matrix to affect the inverse calculation */
    glm::vec4 raw_direction = glm::vec4(x, y, z, 0);
//    LOGI("SANJU : Raw direction : %f %f %f", raw_direction[0], raw_direction[1], raw_direction[2]);
    glm::mat4 inv_view = glm::inverse(view);
    glm::vec3 direction_in_camera_space = inv_view * raw_direction;
//    LOGI("SANJU : After inverse direction : %f %f %f", direction_in_camera_space[0], direction_in_camera_space[1], direction_in_camera_space[2]);

    /* Now extracting the orientation of the hit point */
    glm::vec3 direction_on_plane = direction_in_camera_space - glm::dot(direction_in_camera_space, plane_normal) * plane_normal;
//    LOGI("SANJU : After dot product : %f %f %f", direction_on_plane[0], direction_on_plane[1], direction_on_plane[2]);
    cube_translation_vector += direction_on_plane;
}

void ARCoreManager::LoadTextureFromFile(const char *path, GLuint& textureID) {

    if(!asset_manager) {
        LOGI("SANJU : asset_manager is null");
        return;
    }

    AAsset* asset = AAssetManager_open(asset_manager, path, AASSET_MODE_BUFFER);
    if(!asset) {
        LOGI("SANJU : Failed to load asset : %s", path);
        return;
    }

    /* Reading asset data into memory */
    const void* asset_data = AAsset_getBuffer(asset);
    off_t asset_size = AAsset_getLength(asset);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load_from_memory(static_cast<const unsigned char*>(asset_data), asset_size, &width, &height, &channels, STBI_rgb_alpha);
    AAsset_close(asset);

    if(!data) {
        LOGI("SANJU : Failed to load texture : %s", path);
        return;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    /* Texture Params */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
}

std::string ARCoreManager::LoadShaderFromAsset(const char *shaderPath) {
    if(!asset_manager) {
        LOGI("SANJU : asset_manager is null");
        return "!";
    }

    AAsset* asset = AAssetManager_open(asset_manager, shaderPath, AASSET_MODE_BUFFER);
    if(!asset) {
        LOGI("SANJU : Failed to load asset : %s", shaderPath);
        return "Wow!";
    }

    size_t length = AAsset_getLength(asset);
    char* buffer = new char[length + 1];
    AAsset_read(asset, buffer, length);
    buffer[length] = '\0';
    std::string shaderCode(buffer);
    AAsset_close(asset);
    delete[] buffer;
    return shaderCode;
}