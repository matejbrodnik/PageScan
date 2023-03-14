# PageScan

This app is based on the existing project [PageScan] with major improvements to its performance, several fixes and with added features.
It demonstrates the usage of OpenCV in an Android application using the C++ OpenCV library. The library is imported through the use of Gradle and CMake scripts `pagescan/src/build.gradle` and `pagescan/src/main/cpp/CMakeLists.txt`.

## Setup
The project is compiled using Android Studio. Android NDK must be installed. The path to Android SDK must be specified in the `local.properties` file:
```
sdk.dir=<path_to_sdk>
```
## Structure
The project is organized in a standard Gradle Android app structure. The native C++ code is available in pagescan/src/main/cpp, Java code is available in pagescan/src/main/java. The main class of the application is si.vicos.pagescan.MainActivity.


   [PageScan]: <https://github.com/lukacu/pagescan>
