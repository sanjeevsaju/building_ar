package com.example.buildingar

import android.Manifest
import android.app.Application
import android.content.pm.PackageManager
import android.widget.Toast
import androidx.core.content.ContextCompat
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

class CameraPermissionViewModel(private val app : Application) : ViewModel() {
    private val _isCameraPermissionGranted = MutableStateFlow(false)
    val isCameraPermissionGranted : StateFlow<Boolean> = _isCameraPermissionGranted

    fun checkCameraPermission() {
        viewModelScope.launch {
            val permissionGranted = ContextCompat.checkSelfPermission(
                app,
                Manifest.permission.CAMERA
            ) == PackageManager.PERMISSION_GRANTED
            _isCameraPermissionGranted.value = permissionGranted
        }
    }

    fun  requestCameraPermission(launcher : (String) -> Unit) {
        if(!_isCameraPermissionGranted.value) {
            viewModelScope.launch {
                launcher(Manifest.permission.CAMERA)
            }
        }
    }

    fun onPermissionResult(isGranted : Boolean) {
        if(isGranted) {
            _isCameraPermissionGranted.value = true
        } else {
            Toast.makeText(
                app,
                "Go to Settings and enable this permission to use camera",
                Toast.LENGTH_SHORT
            ).show()
        }
    }
}