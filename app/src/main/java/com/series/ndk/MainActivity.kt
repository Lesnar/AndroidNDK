package com.series.ndk

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import com.series.ndk.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        binding.sampleText.text = stringFromJNI()

        binding.addResultText.text = "add(3, 4) = ${add(3, 4)}"

        binding.multiplyResultText.text = "multiply(2.5, 4.0) = ${multiply(2.5, 4.0)}"

        binding.sayHelloText.text = sayHello("AndroidNDK")

        binding.isPositiveText.text = "isPositive(11) = ${isPositive(11)}"
    }

    /**
     * A native method that is implemented by the 'ndk' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    external fun add(a: Int, b: Int): Int

    external fun multiply(a: Double, b: Double): Double

    external fun sayHello(name: String): String

    external fun isPositive(value: Int): Boolean

    companion object {
        // Used to load the 'ndk' library on application startup.
        init {
            System.loadLibrary("ndk")
        }
    }
}