package com.example.buildingar

import android.Manifest
import android.app.ComponentCaller
import android.content.Context
import android.content.Intent
import android.content.res.AssetManager
import android.content.res.Configuration
import android.net.Uri
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.provider.OpenableColumns
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.annotation.RequiresApi
import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.lifecycleScope
import com.google.android.gms.ads.AdRequest
import com.google.android.gms.ads.AdSize
import com.google.android.gms.ads.AdView
import com.google.android.gms.ads.MobileAds
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import java.io.File
import kotlin.jvm.java

class MainActivity : ComponentActivity() {

    enum class IntentRoute {
        DEFAULT, ONCREATE, ONNEWINTENT
    }
    private lateinit var intentRoute: IntentRoute

    val context : Context = this
    val cameraPermissionViewModel : CameraPermissionViewModel by viewModels {
        CameraPermissionViewModelFactory(application)
    }

    private val cameraPermissionRequestLauncher : ActivityResultLauncher<String> =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) { isGranted : Boolean ->
            cameraPermissionViewModel.onPermissionResult(isGranted)
        }

    private lateinit var arSurfaceView: ARSurfaceView
    private lateinit var surfaceStateJob : Job

    override fun getAssets(): AssetManager {
        return super.getAssets()
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        println("SANJU : MainActivity::onCreate()")

        intentRoute = IntentRoute.ONCREATE

        MobileAds.initialize(this) {}

        /* When the app is launches for the first time, the GL thread is created along
        with the opengl context creation */
        arSurfaceView = ARSurfaceView(this)
        ARNative.setARSurfaceView(arSurfaceView)

        cameraPermissionViewModel.checkCameraPermission()
        ARNative.onCreate(this)

        enableEdgeToEdge()
        setContent {
            val permissionGranted by cameraPermissionViewModel.isCameraPermissionGranted.collectAsState()
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                if(permissionGranted) {
                    ArViewWithTouch()
                } else {
                    Button(onClick = {
                        cameraPermissionViewModel.requestCameraPermission {
                            cameraPermissionRequestLauncher.launch(Manifest.permission.CAMERA)
                        }
                    }) {
                        Text("Request Camera Permission")
                    }
                }
            }
        }

        surfaceStateJob = lifecycleScope.launch {
            arSurfaceView.surfaceState.collect { state->
                if(state == ARSurfaceView.SurfaceState.CHANGED && intentRoute == IntentRoute.ONCREATE) {
                    println("SANJU : MainActivity::onCreate::setContent::lifecycleScope")
                    processIntent(context, intent)
                }
            }
        }
    }

    /* Handle external intents here */
    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        println("SANJU : MainActivity::onNewIntent")

        intentRoute = IntentRoute.ONNEWINTENT

        /* When the app is re-opened after minimization, the emitter emit
        the last known state again when it sees the new collector */
        surfaceStateJob = lifecycleScope.launch {
            arSurfaceView.surfaceState.collect { state->
                if(state == ARSurfaceView.SurfaceState.CHANGED && intentRoute == IntentRoute.ONNEWINTENT) {
                    println("SANJU : MainActivity::onNewIntent::lifecycleScope")
                    processIntent(context, intent)
                }
            }
        }
    }

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    fun processIntent(context : Context, intent : Intent) {
        val uri = intent.data
        var model_name : String? = null
        var file : File? = null
        if(uri?.scheme == "content") {
            val cursor = context.contentResolver.query(uri, null, null, null, null)
            cursor?.use {
                if(it.moveToFirst()) {
                    val nameIndex = it.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    if(nameIndex != -1) {
                        model_name = it.getString(nameIndex)
                    }
                }
            }
        }
        // Now create a File(), write the contents to the file and then invoke the ARNative function with the filepath as parameter
        if(model_name != null) {
            file = File(context.filesDir, "models/${model_name}")
            if(!file.exists()) {
                file.parentFile?.mkdirs()
                if(uri != null) {
                    context.contentResolver.openInputStream(uri)?.use { input ->
                        file.outputStream().use { output ->
                            input.copyTo(output)
                        }
                    }
                }
            }
            println("SANJU : MainActivity::processIntent : ${file.absolutePath}")
            ARNative.loadModel(file.absolutePath)
        }
    }

    override fun onPause() {
        super.onPause()
        println("SANJU : MainActivity::onPause() ${Thread.currentThread().name}")
        ARNative.onPause()
        if(::arSurfaceView.isInitialized) {
            arSurfaceView.onPause()
        }
    }

    override fun onStop() {
        super.onStop()
        println("SANJU : MainActivity::onStop()")
        intentRoute = IntentRoute.DEFAULT
    }

    override fun onDestroy() {
        super.onDestroy()
        println("SANJU : MainActivity::onDestroy()")
    }

    override fun onResume() {
        super.onResume()
        ARNative.onResume(this)
        if(::arSurfaceView.isInitialized) {
            arSurfaceView.onResume()
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        println("SANJU : MainActivity::onConfigurationChanged")
    }

//    override fun onTouchEvent(event: MotionEvent?): Boolean {
//        if(event?.action == MotionEvent.ACTION_DOWN) {
//            val x_val = event.x
//            val y_val = event.y
//            // Send the x and y to the c++ side
//            ARNative.onTouch(x_val, y_val)
//        }
//        return true
//    }

    @Composable
    fun ArViewWithTouch() {
        val configuration = LocalConfiguration.current
        val isLandscape = (configuration.orientation == Configuration.ORIENTATION_LANDSCAPE)
        val context = LocalContext.current

        Box(
            modifier = Modifier
                .fillMaxSize()
                .pointerInput(Unit) {
                    detectTapGestures { offset : Offset ->
                        ARNative.onTouch(offset.x, offset.y)
                    }
                }
        ) {

            // Rendering OpenGL
            AndroidView(
                factory = {
//                    ARSurfaceView(context).also {
//                        arSurfaceView = it
//                        ARNative.setARSurfaceView(it)
//                    }
                    arSurfaceView
                },
                modifier = Modifier.fillMaxSize()
            )

            /* The column contains two boxes - one for the translation buttons and
            * the other for the rotation buttons */
            Column(
                modifier = Modifier
                    .then(
                        if (isLandscape) Modifier
                            .align(Alignment.CenterEnd)
                            .padding(16.dp)
                        else Modifier
                            .align(Alignment.BottomCenter)
                            .padding(32.dp)
                    ),
            ) {
                /* This box contains the translation buttons */
                Box(
                    modifier = Modifier,
                    contentAlignment = Alignment.Center
                ) {
                    Column(
                        verticalArrangement = Arrangement.spacedBy(10.dp),
                        horizontalAlignment = Alignment.CenterHorizontally
                    ) {
                        NavigationButton(
                            imageRes = R.drawable.button_up,
                            onClick = { ARNative.onTranslateCube(0.0f, 0.0f, -0.01f) }
                        )
                        NavigationButton(
                            imageRes = R.drawable.button_down,
                            onClick = { ARNative.onTranslateCube(0.0f, 0.0f, 0.01f) }
                        )
                    }

                    Row(
                        horizontalArrangement = Arrangement.spacedBy(10.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        NavigationButton(
                            imageRes = R.drawable.button_left,
                            onClick = { ARNative.onTranslateCube(-0.01f, 0.0f, 0.0f) }
                        )
                        NavigationButton(
                            imageRes = R.drawable.button_right,
                            onClick = { ARNative.onTranslateCube(0.01f, 0.0f, 0.0f) }
                        )
                    }
                }

                Spacer(modifier = Modifier.height(20.dp))

                /* This box contains the translation buttons */
                Box(
                    modifier = Modifier,
                    contentAlignment = Alignment.Center
                ) {
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(20.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        NavigationButton(
                            imageRes = R.drawable.rot_left,
                            onClick = { ARNative.onRotateCube(1.0f) }
                        )

                        NavigationButton(
                            imageRes = R.drawable.rot_right,
                            onClick = { ARNative.onRotateCube(-1.0f) }
                        )
                    }
                }
            }

            /* Google AdMob Ad */
            Column(
                modifier = Modifier.align(Alignment.TopCenter).padding(16.dp),
                verticalArrangement = Arrangement.Center,
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                BannerAd()
            }
        }
    }

    @Composable
    fun BannerAd(modifier: Modifier = Modifier) {
        val context = LocalContext.current

        AndroidView(
            modifier = modifier,
            factory = {
                val adView = AdView(context)
                adView.setAdSize(AdSize.BANNER)
                adView.adUnitId = "ca-app-pub-3940256099942544/6300978111"
                adView.loadAd(AdRequest.Builder().build())
                adView
            }
        )
    }

    @Composable
    fun NavigationButton(imageRes : Int, onClick : () -> Unit) {
        Image(
            painter = painterResource(id = imageRes),
            contentDescription = "Navigation Button",
            contentScale = ContentScale.Fit,
            modifier = Modifier
                .size(70.dp)
                .clickable { onClick.invoke() }
        )
    }

    @Composable
    fun AxisFlipButton(imageRes : Int, onClick: () -> Unit) {
        Image(
            painter = painterResource(id = imageRes),
            contentDescription = "Axis flip button",
            contentScale = ContentScale.Fit,
            modifier = Modifier
                .size(70.dp)
                .clickable { onClick.invoke() }
        )
    }
}

/* Note for Adb */
// To view the local file system of the running app in the adb, use the following commands
// 1. adb shell
// 2. run-as com.example.buildingar (for non-rooted devices)
// 3. cd files/
// The folder 'converted' will be available from this location