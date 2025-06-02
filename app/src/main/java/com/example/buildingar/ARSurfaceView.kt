package com.example.buildingar

import android.content.Context
import android.hardware.display.DisplayManager
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.view.Display
import android.view.Surface
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class ARSurfaceView(context: Context) : GLSurfaceView(context), GLSurfaceView.Renderer {

    private var displayRotation : Int = 0
    private val displayManager  = context.getSystemService(Context.DISPLAY_SERVICE) as DisplayManager
    private val display : Display? = displayManager.getDisplay(Display.DEFAULT_DISPLAY)

    init {
        /* Called on the main thread */
        preserveEGLContextOnPause = true
        setEGLContextClientVersion(3)
        setEGLConfigChooser(8, 8, 8, 8, 16, 0)
        setRenderer(this)

        /* Needed if you want to show UI elements behind the OpenGL content */
//        holder.setFormat(android.graphics.PixelFormat.TRANSPARENT)
        /* Force the surface to be drawn on top of the window */
//        setZOrderOnTop(true)

        renderMode = RENDERMODE_CONTINUOUSLY
        println("SANJU : ARSurfaceView init")
    }

    /* Called on the GL thread */
    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        ARNative.onSurfaceCreated()
        println("SANJU : ARSurfaceView onSurfaceCreated : ${Thread.currentThread().name}")
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        println("SANJU : ARSurfaceView onSurfaceChanged")
        updateDisplayRotation()
        GLES20.glViewport(0, 0, width, height)
    }

    /* Called on the GL thread */
    override fun onDrawFrame(gl: GL10?) {
        updateDisplayRotation()
        ARNative.onDrawFrame(width, height, displayRotation)
    }

    /* Called on the main thread */
    override fun onPause() {
        super.onPause()
        println("SANJU : ARSurfaceView onPause")
    }

    /* Called on the main thread */
    override fun onResume() {
        super.onResume()
        println("SANJU : ARSurfaceView onResume")
    }

    private fun updateDisplayRotation() {
        /* Get current display orientation */
        display?.let {
            displayRotation = when(it.rotation) {
                Surface.ROTATION_0 -> 0
                Surface.ROTATION_90 -> 1
                Surface.ROTATION_180 -> 2
                Surface.ROTATION_270 -> 3
                else -> 0
            }
        }
    }
}