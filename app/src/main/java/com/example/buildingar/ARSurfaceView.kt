package com.example.buildingar

import android.content.Context
import android.hardware.display.DisplayManager
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.view.Display
import android.view.Surface
import android.view.SurfaceHolder
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class ARSurfaceView(context: Context) : GLSurfaceView(context), GLSurfaceView.Renderer {

    enum class SurfaceState { INITIAL, CREATED, CHANGED, PAUSED, DESTROYED }
    private val _surfaceState = MutableStateFlow<SurfaceState>(SurfaceState.INITIAL)
    val surfaceState : StateFlow<SurfaceState> = _surfaceState

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
        println("SANJU : ARSurfaceView::Init")
    }

    /* Called on the GL thread */
    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        ARNative.onSurfaceCreated()
        println("SANJU : ARSurfaceView::onSurfaceCreated : ${Thread.currentThread().name}")
        _surfaceState.value = SurfaceState.CREATED
    }

    /* Called on the GL thread */
    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        println("SANJU : ARSurfaceView::onSurfaceChanged : ${Thread.currentThread().name}")
        updateDisplayRotation()
        GLES20.glViewport(0, 0, width, height)
        _surfaceState.value = SurfaceState.CHANGED
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        super.surfaceDestroyed(holder)
        _surfaceState.value = SurfaceState.DESTROYED
        println("SANJU : ARSurfaceView::surfaceDestroyed()")
    }

    /* Called on the GL thread */
    override fun onDrawFrame(gl: GL10?) {
        updateDisplayRotation()
        ARNative.onDrawFrame(width, height, displayRotation)
    }

    /* Called on the main thread */
    override fun onPause() {
        super.onPause()
        println("SANJU : ARSurfaceView::onPause")
        _surfaceState.value = SurfaceState.PAUSED
    }

    /* Called on the main thread */
    override fun onResume() {
        super.onResume()
        println("SANJU : ARSurfaceView::onResume")
    }

    fun runOnGLThread(action : () -> Unit) {
        /* Queue a runnable to be run on the GL thread */
        println("SANJU : ARSurfaceView::runOnGLThread(queueEvent)")
        queueEvent(action)
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