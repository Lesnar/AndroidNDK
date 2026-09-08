package com.series.ndk

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import android.widget.Toast
import com.series.ndk.data.User

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

        val array = intArrayOf(1, 2, 3, 4, 5)
        binding.sumArrayText.text = "sumArray(${array.joinToString()}) = ${sumArray(array)}"

        val created = createArray(6)
        binding.createArrayText.text = "createArray(6) = ${created.joinToString()}"

        val input = intArrayOf(1, 2, 3)
        val doubled = multiplyArray(input)
        binding.multiplyArrayText.text =
            "multiplyArray(${input.joinToString()}) = ${doubled.joinToString()}"

        val nativeUser = createUser()

        binding.userInfoText.text =
            "getUserInfo: ${getUserInfo(User("Martin", 30))}\ncreateUser() = $nativeUser"
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

//    ======== Day02 ==========

    external fun sumArray(values: IntArray): Int

    external fun createArray(size: Int): IntArray

    external fun multiplyArray(values: IntArray): IntArray

    external fun getUserInfo(user: User): String

    external fun createUser(): User

    fun showMessage(message: String){
        Toast.makeText(this, "This toast from C++：$message", Toast.LENGTH_SHORT).show()
    }

    companion object {
        // Used to load the 'ndk' library on application startup.
        init {
            System.loadLibrary("ndk")
        }
    }
}