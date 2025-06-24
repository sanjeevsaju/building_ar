# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("buildingar")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        native_renderer.cpp arcore_manager.cpp utility.cpp stb_image.cpp glb_renderer.cpp model.cpp)

# --------------------- Added Starts ---------------------------- #

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})

add_library(assimp SHARED IMPORTED)
set_target_properties(assimp PROPERTIES IMPORTED_LOCATION ${LIB_DIR}/libassimp.so)

add_library(arcore SHARED IMPORTED)
set_target_properties(arcore PROPERTIES IMPORTED_LOCATION ${LIB_DIR}/libarcore_sdk_c.so)

find_library(OPENGLES3_LIB GLESv3)
find_library(EGL_LIB EGL)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#add_library(opengl_es INTERFACE IMPORTED)
#set_target_properties(opengl_es PROPERTIES INTERFACE_LINK_LIBRARIES "-lEGL;-lGLESv3")

#include_directories(/home/sanjeev/AndroidStudioProjects/My_Native_App/app/src/main/cpp/)

# --------------------- Added Ends ---------------------------- #
# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        assimp
        arcore
        ${OPENGLES3_LIB} # Directly linking OpenGL ES
        ${EGL_LIB}      # Directly linking EGL
        android
        log)