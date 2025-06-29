#include <jni.h>
#include <arcore_manager.h>

#define LOG_TAG "Native Renderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/* Now only the pointer variable is in the .data(initialized global static) process memory segment */
ARCoreManager *manager = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_onCreate(JNIEnv *env, jobject thiz, jobject context) {
    jclass context_class = env->GetObjectClass(context);
    jmethodID get_assets_method = env->GetMethodID(context_class, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject asset_manager_jobj = env->CallObjectMethod(context, get_assets_method);

    AAssetManager* asset_manager = AAssetManager_fromJava(env, asset_manager_jobj);

    manager = new ARCoreManager;
    manager->Initialize(env, context, asset_manager);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_onResume(JNIEnv *env, jobject thiz, jobject activity) {
    manager->Resume();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_onPause(JNIEnv *env, jobject thiz) {
    manager->Pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_nativeOnSurfaceCreated(JNIEnv *env, jobject thiz) {
    manager->OnSurfaceCreated();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_onDrawFrame(JNIEnv *env, jobject thiz, jint width, jint height,
                                                 jint display_rotation) {
    manager->OnDrawFrame(width, height, display_rotation);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_onTouch(JNIEnv *env, jobject thiz, jfloat x, jfloat y) {
    manager->OnTouch(x, y);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_onRotateCube(JNIEnv *env, jobject thiz, jfloat degrees) {
    manager->RotateCube(degrees);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_onScaleCube(JNIEnv *env, jobject thiz, jfloat scale) {
    manager->ScaleCube(scale);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_onTranslateCube(JNIEnv *env, jobject thiz, jfloat x, jfloat y,
                                                     jfloat z) {
    manager->TranslateCube(x, y, z);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_buildingar_ARNative_nativeConvertToGLB(JNIEnv *env, jobject thiz,
                                                        jstring input_path, jstring output_path) {
    const char* inputPath = env->GetStringUTFChars(input_path, nullptr);
    LOGI("SANJU : inputPath = %s", inputPath);
    const char* outputPath = env->GetStringUTFChars(output_path, nullptr);
    LOGI("SANJU : outputPath = %s", outputPath);

    bool success = manager->ConvertToGLB(inputPath, outputPath);

    env->ReleaseStringUTFChars(input_path, inputPath);
    env->ReleaseStringUTFChars(output_path, outputPath);

    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_setModelPath(JNIEnv *env, jobject thiz, jstring model_path) {
    if(manager) {
        const char* path = env->GetStringUTFChars(model_path, nullptr);
        manager->SetModelPath(path);
        env->ReleaseStringUTFChars(model_path, path);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_buildingar_ARNative_nativeLoadModel(JNIEnv *env, jobject thiz, jstring model_path) {
    if(manager) {
        const char* path = env->GetStringUTFChars(model_path, nullptr);
        manager->loadModelFromStorage(path);
        env->ReleaseStringUTFChars(model_path, path);
    }
}