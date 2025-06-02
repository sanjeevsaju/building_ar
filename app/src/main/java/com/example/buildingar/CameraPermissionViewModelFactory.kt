package com.example.buildingar

import android.app.Application
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider

class CameraPermissionViewModelFactory(private val app : Application) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if(modelClass.isAssignableFrom(CameraPermissionViewModel::class.java)){
            @Suppress("UNCHECKED_CAST")
            return CameraPermissionViewModel(app) as T
        }
        throw IllegalArgumentException("Unknown ViewModel Class")
    }
}