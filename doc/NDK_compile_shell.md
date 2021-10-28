# 前言
Android NDK以前默认使用Android.mk与Application.mk进行构建，但是在Android Studio2.2之后推荐使用CMake进行编译。</br>
CMake是跨平台编译工具，全称为cross platform make，内建c、c++、java自动相依性分析功能。NDK通过工具链支持CMake，</br>
工具链文件是用于自定义交叉编译工具链的CMake文件。用于NDK的工具链位于<NDK>/build/cmake/android.toolchain.cmake，</br>
关于CMake更多详情请参考官网：[cmake官网](https://cmake.org/documentation/)。下面对比下Android.mk与CMakeLists.txt的语法。</br>
更详细脚本分析可查看博客：[NDK编译脚本分析](https://blog.csdn.net/u011686167/article/details/106458899)<br>

# 一、Android.mk语法
以动态库编译hello模块为例，完整脚本如下：
```
WORKING_DIR := $(call my-dir)
LOCAL_PATH := $(WORKING_DIR)

include $(CLEAR_VARS)
LOCAL_ARM_MODE  := arm
LOCAL_MODULE    := libffmpeg
LOCAL_SRC_FILES := $(LOCAL_PATH)/ffmpeg/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/ffmpeg/include
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE  := arm
#模块名称
LOCAL_MODULE    := hello
#源文件
LOCAL_SRC_FILES := hello.c
#头文件路径
LOCAL_C_INCLUDES := $(LOCAL_PATH)
#系统库依赖
LOCAL_LDLIBS    := -llog -lz -lm -landroid
#第三方动态库
LOCAL_SHARED_LIBRARIES := libffmpeg
#以动态库形式编译
include $(BUILD_SHARED_LIBRARY)
```
## 1、LOCAL_MODULE
声明模块名称，例如这里编译出来的动态库名称为libhello.so。

## 2、LOCAL_SRC_FILES
声明源文件列表，文件之间用空格分开，需要换行时使用'\'换行符。

## 3、LOCAL_C_INCLUDES
声明头文件路径，例如$(LOCAL_PATH)/xxx

## 4、LOCAL_CPP_EXTENSION
指定C++源文件除.cpp以外的文件扩展名，例如这样LOCAL_CPP_EXTENSION := .cpp .cxx .cc

## 5、LOCAL_CPP_FEATURES
指定依赖c++的某些功能，例如RTTI(运行时类型信息)：
LOCAL_CPP_FEATURES := rtti
使用c++异常检测：
LOCAL_CPP_FEATURES := exceptions

## 6、LOCAL_CFLAGS
在编译c和c++源文件时编译系统要传递的编译器标记，即指定额外的宏定义或编译选项。
LOCAL_CFLAGS += -I<path>

## 7、LOCAL_STATIC_LIBRARIES
共享静态库，作为第三方库被引用
LOCAL_STATIC_LIBRARIES := libavcodec libavutil libavformat libavfilter

## 8、LOCAL_SHARED_LIBRARIES
共享动态库，与共享静态库一样作为第三方库被引用
LOCAL_SHARED_LIBRARIES := libffmpeg

## 9、LOCAL_LDLIBS
额外链接器，一般为系统库，使用-l来引用
LOCAL_LDLIBS := -lz -lm

## 10、 LOCAL_ARM_MODE
ndk默认使用thumb模式来生成目标二进制文件，每条指令为16位宽。也可以指定为ARM模式，来生成32位ARM的目标文件：</br>
LOCAL_ARM_MODE := arm

## 11、LOCAL_ARM_NEON
用于开启NEON指令加速，仅对armeabi-v7a平台有效。为模块开启NEON：
LOCAL_ARM_NEON := true
为单独源文件开启NEON：
LOCAL_SRC_FILES := hello.c.neon

## 12、TARGET_ARCH
用于指向CPU架构，包括x86、x86_64、armeabi-v7a、 arm64-v8a

## 13、TARGET_PLATFORM
目标平台，对应Android API级别号，例如Android5.0系统镜像对应Android API级别21：android-21

## 14、打印信息
可用warning、debug、info、error级别来打印信息，如果是打印error信息，会终止编译。以warning为例：
$(warning 'This is a test')

## 15、if条件判断
采用ifeq关键字，然后左右变量放在括号体内，用逗号分隔：
```
ifeq($(TARGET_ABI), arm64-v8a)
$(debug 'This is arm64-v8a')
endif
```
# 二、Application.mk语法
Android.mk依赖Application.mk文件进行编译，一般Application.mk脚本如下所示：</br>
```
APP_STL      := c++_static
APP_DEBUG    := false
APP_OPTIM    := release
APP_CPPFLAGS := -frtti
APP_PLATFORM := android-16
APP_ABI      := armeabi-v7a arm64-v8a
```
## 1、APP_ABI
与Android.mk的TARGET_ABI对应，包括CPU架构有：x86、x86_64、armeabi-v7a、arm64-v8a，支持所有平台这样表示：</br>
APP_ABI := all

## 2、APP_BUILD_SCRIPT
指向编译脚本的路径，一般Android.mk和Application.mk都位于jni目录，默认指向jni/Android.mk路径，如果是其他路径，</br>
需要使用此变量来指定绝对路径：
APP_BUILD_SCRIPT := /xx/xx/Android.mk

## 3、APP_OPTIM
编译优化选项，调试模式为debug，发布模式为release。在调试模式下，会保留symbol符号表；在发布模式下，会开启优化，去掉symbol符号表。

## 4、APP_PLATFORM
指定编译平台，面向于Android API级别，对应gradle声明的minSdkVersion。如果不声明，默认为ndk支持的最低API版本

## 5、APP_STL
声明使用c++的标准库，默认为system STL。其他选项包括c++_static、c++_shared和none

# 三、CMakeLists.txt语法

以编译hello模块以及依赖ffmpeg模块为例：
```
cmake_minimum_required(VERSION 3.4.1)
#添加动态库，包含源文件路径
add_library( hello
             SHARED
             src/main/jni/hello.c)
#添加第三方动态库
add_library( ffmpeg
             SHARED
             IMPORTED )
#指定第三方库路径
set_target_properties( ffmpeg
                       PROPERTIES IMPORTED_LOCATION
                       ../../../../libs/${CMAKE_ANDROID_ARCH_ABI}/libffmpeg.so )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
#指定头文件路径
include_directories(src/main/cpp)
if(${CMAKE_ANDROID_ARCH_ABI} MATCHES "armeabi-v7a")
    include_directories(src/main/cpp/include/armeabi-v7a)
    message("This is armeabi-v7a")
elseif(${CMAKE_ANDROID_ARCH_ABI} MATCHES "arm64-v8a")
    include_directories(src/main/cpp/include/arm64-v8a)
    message("This is arm64-v8a")
endif()
#查找系统库
find_library( # Sets the name of the path variable.
              log-lib
              log )
#链接目标库
target_link_libraries( hello
                       ffmpeg
                       ${log-lib} )
```
## 1、add_library
传递三个参数，第一个参数是模块名称，第二个参数是SHARED或者STATIC。如果是源文件模块，第三个参数是源文件列表；</br>
如果是第三方库，第三个参数是IMPORTED。

## 2、set_target_properties
用于指定第三方库路径，IMPORT_LOCATION一般是指向src/main/cpp目录

## 3、include_directories
用于指定头文件路径，头文件路径可以有多个

## 4、find_library
用于查找系统库，比如Android系统的log日志库

## 5、target_link_libraries
链接目标库，把依赖库都链接到目标库中

## 6、if条件判断
与Android.mk稍有差异，CMake采用if...MATCHES形式，例如：
if(${CMAKE_ANDROID_ARCH_ABI} MATCHES "armeabi-v7a")
......
endif()

## 7、打印日志
与Android.mk不同的是，CMake采用message函数来打印日志，括号体传入msg内容
message("hello, cmake")

## 8、命令行参数
命令行参数前面统一加上-D，常用的参数：

* -DANRDOID_ABI :android的ABI架构平台

* -DANDROID_NDK :ndk路径

* -DANDROID_ARM_MODE :arm模式/thumb模式

* -DANDROID_ARM_NEON :是否开启arm neon加速，针对armeabi-v7a平台

* -DANDROID_TOOLCHAIN :编译工具链

* -DANDROID_NATIVE_API_LEVEL :与ANDROID_PLATFORM相同，对应minSdkVersion

* -DCMAKE_BUILD_TYPE :编译类型，debug或release

* -DCMAKE_MAKE_PROGRAM :编译程序

* -DCMAKE_TOOLCHAIN_FILE :编译文件

## 9、命令行编译
以cmake作为关键字，后面带着指定参数，示例如下：
```
    cmake \
    -DANDROID_ABI=armeabi-v7a \
    -DANDROID_NDK=${HOME}/Android/Sdk/ndk-bundle \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_MAKE_PROGRAM=${HOME}/Android/Sdk/cmake/3.6.3155560/bin/ninja \
    -DCMAKE_TOOLCHAIN_FILE=${HOME}/Android/Sdk/ndk-bundle/build/cmake/android.toolchain.cmake \
    -DANDROID_NATIVE_API_LEVEL=23 \
    -DANDROID_TOOLCHAIN=clang
```
# 四、ndk编译配置
## 1、Android.mk方式配置
在gradle的defaultConfig配置ndk：
```
defaultConfig {
    ......
    ndk {
        moduleName "hello"
        abiFilters "armeabi-v7a", "arm64-v8a"
    }
}
```
然后配置jni源文件路径：
```
sourceSets {
    main {
        jniLibs.srcDir 'src/main/libs' // Enable to use libs
        jni.srcDirs 'src/main/jni' // Enable the automatic ndk-build
    }
}
```
另外配置Android.mk文件绝对路径：
```
externalNativeBuild {
    ndkBuild {
        "src/main/jni/Android.mk"
    }
}
```
## 2、CMake方式配置
前两步与Android.mk方式一样，配置脚本路径稍有差异：
```
externalNativeBuild {
    cmake {
        path "CMakeLists.txt"
    }
}
```
另外在defaultConfig设置cppFlags:
```
externalNativeBuild {
    cmake {
        cppFlags ""
    }
}
```
# 五、ndk编译过程
## 1、ndk-build编译
在命令行输入ndk-build后，会根据声明所支持的平台依次编译。首先是armeabi-v7a平台架构，把hello.c源文件编译成hello</br>
目标文件，然后链接成libhello.so动态库，最终安装到libs/armeabi-v7a目录下。

## 2、cmake在Gradle中编译
编译arm64-v8a平台架构的hello模块。首先把hello.c源文件编译成hello.c.o目标文件，然后链接成libhello.so动态库。</br>
生成的debug模式动态库在/build/intermediates/cmake/debug/obj/arm64-v8a目录下。