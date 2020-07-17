
cmake_minimum_required(VERSION 3.4.1)
file(GLOB SOURCE src/main/cpp/*.cpp )
add_library(
        frizzleplayer
        SHARED
        ${SOURCE}
        src/main/cpp/FrizzleFFmpeg.cpp
)
set(my_lib_path ${CMAKE_SOURCE_DIR}/src/main/cpp/libs/${ANDROID_ABI})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${my_lib_path} ")


include_directories(src/main/cpp/include/)
target_link_libraries(
        frizzleplayer
        avfilter avformat    avcodec  avutil  swresample swscale
        z
        OpenSLES
        android
        log )