#include <arcore_manager.h>
#include <thread>

#define LOG_TAG "ARCore Managerl"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define GL_CHECK(x) \
    x; \
    { \
        GLenum glError = glGetError(); \
        if(glError != GL_NO_ERROR) { \
            LOGE("OpenGL error %d at %s:%d - %s", glError, __FILE__, __LINE__, #x); \
            return; \
        } \
    }

/* Runs on the main thread */
bool ARCoreManager::Initialize(void *env, jobject context, AAssetManager* mgr) {
    LOG_TID("SANJU : Thread of Initialize");
    asset_manager = mgr;
    if(!mgr) {
        LOGE("NAT_ERROR : Asset manager is null");
        return false;
    }

    if(ar_session) return true;

    ArStatus status = ArSession_create((JNIEnv*)env, context, &ar_session);
    if(status != AR_SUCCESS) {
        LOGE("NAT_ERROR : ArFrame_create failed with status: %d", status);
        ArSession_destroy(ar_session);
        ar_session = nullptr;
        return false;
    } else {
        LOGI("SANJU : ArSession_create success");
    }

    ArFrame_create(ar_session, &ar_frame);
    return true;
}

void ARCoreManager::Resume() {
    if(ar_session) ArSession_resume(ar_session);
    LOGI("SANJU : ArSession_resume");
}

void ARCoreManager::Pause() {
    if(ar_session) ArSession_pause(ar_session);
    LOGI("SANJU : ArSession_pause");
}

/* Runs on the GL thread */
void ARCoreManager::OnSurfaceCreated() {

    std::string planeVertexShaderCode = LoadShaderFromAsset("shaders/plane/plane.vert");
    const char* planeVertexShaderSource = planeVertexShaderCode.c_str();

    std::string planeFragmentCode = LoadShaderFromAsset("shaders/plane/plane.frag");
    const char* planeFragmentShaderSource = planeFragmentCode.c_str();

    /* Compile and link object rendering shaders */
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &planeVertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &planeFragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    plane_shader_program = glCreateProgram();
    glAttachShader(plane_shader_program, vertexShader);
    glAttachShader(plane_shader_program, fragmentShader);
    glLinkProgram(plane_shader_program);

    /****************** Camera Config Program Starts *************/

    std::string cameraVertexShaderCode = LoadShaderFromAsset("shaders/camera/camera.vert");
    const char* cameraVertexShaderSource = cameraVertexShaderCode.c_str();

    std::string cameraFragmentShaderCode = LoadShaderFromAsset("shaders/camera/camera.frag");
    const char* cameraFragmentShaderSource = cameraFragmentShaderCode.c_str();

    /* Compile and link object rendering shaders */
    GLuint cameraVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(cameraVertexShader, 1, &cameraVertexShaderSource, nullptr);
    glCompileShader(cameraVertexShader);

    GLuint cameraFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(cameraFragmentShader, 1, &cameraFragmentShaderSource, nullptr);
    glCompileShader(cameraFragmentShader);

    camera_shader_program = glCreateProgram();
    glAttachShader(camera_shader_program, cameraVertexShader);
    glAttachShader(camera_shader_program, cameraFragmentShader);
    glLinkProgram(camera_shader_program);

    glGenTextures(1, &cameraTextureId);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, cameraTextureId);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* Set texture for ARCore */
    ArSession_setCameraTextureName(ar_session, cameraTextureId);

    float quad_vertices[] = {
            // positions                //tex coords
            -1.0f, -1.0f,   0.0f, 1.0f,
            1.0f, -1.0f,   1.0f, 1.0f,
            -1.0f,  1.0f,   0.0f, 0.0f,
            1.0f,  1.0f,   1.0f, 0.0f
    };

    glGenVertexArrays(1, &camera_vao);
    glBindVertexArray(camera_vao);

    glGenBuffers(1, &camera_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, camera_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);   // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);   // Texture
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    /****************** Camera Config Program Ends *************/


    /****************** Model Config Starts *****************/

    std::string modelVertexShaderCode = LoadShaderFromAsset("shaders/model/model.vert");
    const char* modelVertexShaderSource = modelVertexShaderCode.c_str();

    std::string modelFragmentShaderCode = LoadShaderFromAsset("shaders/model/model.frag");
    const char* modelFragmentShaderSource = modelFragmentShaderCode.c_str();

    // Compile and link shaders
    GLuint modelVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(modelVertexShader, 1, &modelVertexShaderSource, nullptr);
    glCompileShader(modelVertexShader);

    /* Checking for error */
    GLint status = 0;
    glGetShaderiv(modelVertexShader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        GLint logLen = 0;;
        glGetShaderiv(modelVertexShader, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetShaderInfoLog(modelVertexShader, logLen, nullptr, log.data());
        LOGI("SANJU : Failed to compile modelvertexshader : %s", log.data());
    }

    GLuint modelFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modelFragmentShader, 1, &modelFragmentShaderSource, nullptr);
    glCompileShader(modelFragmentShader);

    /* Checking for error */
    glGetShaderiv(modelFragmentShader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(modelFragmentShader, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetShaderInfoLog(modelFragmentShader, logLen, nullptr, log.data());
        LOGI("SANJU : Failed to compile modelfragmentshader : %s", log.data());
    }

    model_shader_program = glCreateProgram();
    glAttachShader(model_shader_program, modelVertexShader);
    glAttachShader(model_shader_program, modelFragmentShader);
    glLinkProgram(model_shader_program);

    /* Checking for error */
    glGetProgramiv(model_shader_program, GL_LINK_STATUS, &status);
    if(status == GL_FALSE) {
        GLint logLen = 0;;
        glGetProgramiv(model_shader_program, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetProgramInfoLog(model_shader_program, logLen, nullptr, log.data());
        LOGI("SANJU : Failed to link model_shader_program : %s", log.data());
    }


    LOG_TID("SANJU : Thread of OnSurfaceCreated");

    /* Let the model loads and does the opengl config concurrently using thread as it wont affect the camera texture rendering and plane rendering */
//    std::thread t1([&](){
//        LOG_TID("SANJU : Thread in OnSurfaceCreated");
//        /* Now reading the model data and setting model vao */
//        if(!obj_model.LoadWithAssimp(asset_manager, "objBuilding.obj")) {
//            LOGE("NAT_ERROR : Model loading failed : %s", obj_model.GetLastError().c_str());
//            return;
//        }
//
//        if(!obj_model.SetShaderProgram(model_shader_program)) {
//            LOGE("NAT_ERROR : Shader loading failed : %s", obj_model.GetLastError().c_str());
//            return;
//        }
//    });
//
//    t1.detach();

    /* Now reading the model data and setting the model vao */
//    if(!obj_model.LoadFromFile(asset_manager, model_path_)) {
//        LOGE("NAT_ERROR : Model loading failed : %s", obj_model.GetLastError().c_str());
//        return;
//    }
//
//    if(!obj_model.SetShaderProgram(model_shader_program)) {
//        LOGE("NAT_ERROR : Shader loading failed : %s", obj_model.GetLastError().c_str());
//        return;
//    }

    glb_model.setProgram(model_shader_program);
    glb_model.load(asset_manager, "models/building.glb");
//    glb_model.load(asset_manager, model_path_);
    /****************** Model Config Ends *****************/

    glGenBuffers(1, &plane_vbo);
}

/* Runs on the GL thread */
void ARCoreManager::OnDrawFrame(int width, int height, int displayRotation) {
    if(!ar_session) return;

    screen_width = width;
    screen_height = height;

    ArSession_setDisplayGeometry(ar_session, displayRotation, width, height);
    ArStatus status = ArSession_update(ar_session, ar_frame);

    ArCamera* camera;
    ArFrame_acquireCamera(ar_session, ar_frame, &camera);
    ArCamera_getViewMatrix(ar_session, camera, glm::value_ptr(view));
    ArCamera_getProjectionMatrix(ar_session, camera, /*near*/ 0.1f, /*far*/ 100.0f, glm::value_ptr(proj));
    ArCamera_release(camera);
    mvp = proj * view;

    /* Render camera frame image texture using OpenGL */
    glUseProgram(camera_shader_program);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, cameraTextureId);
    glUniform1i(glGetUniformLocation(camera_shader_program, "u_Texture"), 0);

    GLuint rotationLocation = glGetUniformLocation(camera_shader_program, "u_Rotation");
    glUniform1i(rotationLocation, displayRotation);

    glBindVertexArray(camera_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);

    /* Plane detection logic */
    ArTrackableList* planes;
    ArTrackableList_create(ar_session, &planes);
    ArSession_getAllTrackables(ar_session, AR_TRACKABLE_PLANE, planes);

    int count = 0;
    ArTrackableList_getSize(ar_session, planes, &count);

    /* Check if rest of the code can be optimized using SIMD */
    for(int i = 0; i < count; i++) {
        ArTrackable *trackable;
        ArTrackableList_acquireItem(ar_session, planes, i, &trackable);
        ArTrackingState state;
        ArTrackable_getTrackingState(ar_session, trackable, &state);

        if(state == AR_TRACKING_STATE_TRACKING) {
            /* Process plane data */
            ArPlane* plane = reinterpret_cast<ArPlane*>(trackable);
            ArPose* center_pose = nullptr;

            ArPose_create(ar_session, nullptr, &center_pose);
            ArPlane_getCenterPose(ar_session, plane, center_pose);

            float model_matrix[16];
            ArPose_getMatrix(ar_session, center_pose, model_matrix);

            float extent_x = 0.0f;
            float extent_z = 0.0f;
            ArPlane_getExtentX(ar_session, plane, &extent_x);
            ArPlane_getExtentZ(ar_session, plane, &extent_z);

            int32_t polygon_size = 0;
            ArPlane_getPolygonSize(ar_session, plane, &polygon_size);
            std::vector<float> polygon(polygon_size);
            ArPlane_getPolygon(ar_session, plane, polygon.data());

            /* Transforming the planes' polygon vertices from local to world coordinates */
            std::vector<float> world_vertices;
            for(int i = 0; i < polygon_size; i += 2) {
                float local_point[3] = { polygon[i], 0.0f, polygon[i + 1]};
                float world_point[3];
                TransformPoint(model_matrix, local_point, world_point);
                world_vertices.push_back(world_point[0]);
                world_vertices.push_back(world_point[1]);
                world_vertices.push_back(world_point[2]);
            }

            /* Draw the polygon (plane) */
            glUseProgram(plane_shader_program);
            /* Enable depth test */
            glEnable(GL_DEPTH_TEST);
            /* Accept fragment if it closer to the camera than the former one */
            glDepthFunc(GL_LESS);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            GLuint mvpLocation = glGetUniformLocation(plane_shader_program, "mvp");
            glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));

            glBindBuffer(GL_ARRAY_BUFFER, plane_vbo);
            glBufferData(GL_ARRAY_BUFFER, world_vertices.size() * sizeof(float) , world_vertices.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

            glDrawArrays(GL_TRIANGLE_FAN, 0, world_vertices.size() / 3);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glUseProgram(0);
        }
        ArTrackable_release(trackable);
    }
    ArTrackableList_destroy(planes);

    /* Render the object here */
    if(model_place) {
        glm::mat4 model_pose_matrix = hit_pose_matrix;
        glm::mat4 model_scale = glm::scale(glm::mat4(1.0f), glm::vec3(scaling_factor));
        glm::mat4 model_rotation = glm::rotate(glm::mat4(1.0f), cube_rotation_angle, cube_rotation_axis);
        glm::mat4 model_flip_axis = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 model_translation = glm::translate(glm::mat4(1.0f), cube_translation_vector);
        glm::mat4 model_mvp = proj * view * model_translation * model_pose_matrix * model_rotation * model_scale;

        glb_model.draw(glm::value_ptr(model_mvp));
    }
}