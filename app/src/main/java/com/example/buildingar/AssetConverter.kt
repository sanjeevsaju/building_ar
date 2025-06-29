package com.example.buildingar

import android.content.Context
import java.io.File

class AssetConverter(private val context: Context) {

    fun convertModel(assetPath : String) : String {
        val outputFile = File(context.filesDir, "converted/${assetPath.substringAfterLast('/')}.glb")

        /* If the file already exists */
        if(outputFile.exists()) {
            return outputFile.absolutePath
        }

        /* If the file does not already exists, create the file */
        outputFile.parentFile?.mkdirs()

        val success = ARNative.nativeConvertToGLB(
            assetPath,
            outputFile.absolutePath
        )

        return if(success) outputFile.absolutePath else ""
    }
}