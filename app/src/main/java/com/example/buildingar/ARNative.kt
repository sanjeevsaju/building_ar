package com.example.buildingar

import android.app.Activity
import android.content.Context
import java.lang.ref.WeakReference

object ARNative {
    init {
        System.loadLibrary("buildingar")
        println("SANJU : ARNative::Init")
    }

    private var isSurfaceCreated : Boolean = false
    private var pendingModelPath : String? = null
    private val lock = Any()
    val lock_2 = Any()
    private var arSurfaceViewWeak : WeakReference<ARSurfaceView>? = null

    fun onSurfaceCreated() {
        println("SANJU : ARNative::onSurfaceCreated")
        synchronized(lock) {
            isSurfaceCreated = true
            pendingModelPath?.let { path ->
                runOnGLThread { nativeLoadModel(path) }
                pendingModelPath = null
            }
        }
        nativeOnSurfaceCreated()
    }

    fun loadModel(modelPath : String) {
        println("SANJU : ARNative::loadModel")
        synchronized(lock) {
            if(isSurfaceCreated) {
                runOnGLThread { nativeLoadModel(modelPath) }
            } else {
                pendingModelPath = modelPath
            }
        }
    }

    fun setARSurfaceView(view : ARSurfaceView) {
        println("SANJU : ARNative::setARSurfaceView")
        arSurfaceViewWeak = WeakReference(view)
    }

    fun runOnGLThread(action : () -> Unit) {
        println("SANJU : ARNative::runOnGLThread")
        arSurfaceViewWeak?.get()?.runOnGLThread(action)
    }

    external fun onCreate(context: Context)
    external fun onResume(activity: Activity)
    external fun onPause()
    external fun nativeOnSurfaceCreated()
    external fun onDrawFrame(width : Int, height : Int, displayRotation : Int)
    external fun onTouch(x : Float, y : Float)

    external fun onRotateCube(degrees : Float)
    external fun onScaleCube(scale : Float)
    external fun onTranslateCube(x : Float, y : Float, z : Float)

    external fun nativeConvertToGLB(inputPath : String, outputPath : String) : Boolean
    external fun setModelPath(modelPath : String)
    external fun nativeLoadModel(modelPath : String)

}