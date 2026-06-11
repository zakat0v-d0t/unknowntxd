cmake_minimum_required(VERSION 3.16)
project(UnknownTxd LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Threads REQUIRED)

add_executable(UnknownTxd
    Source/Main.cpp
    Source/Core/BinaryReader.cpp
    Source/Core/BinaryWriter.cpp
    Source/RenderWare/RwStream.cpp
    Source/RenderWare/Image.cpp
    Source/RenderWare/PixelConverter.cpp
    Source/RenderWare/TextureDictionary.cpp
    Source/RenderWare/TextureEncoder.cpp
    Source/RenderWare/ImageWriter.cpp
    Source/RenderWare/ImageReader.cpp
    Source/RenderWare/ImageOps.cpp
    Source/RenderWare/StbImpl.cpp
    Source/Application/MainWindow.cpp
)
target_include_directories(UnknownTxd PRIVATE ${CMAKE_SOURCE_DIR}/Source ${CMAKE_SOURCE_DIR}/External/stb)
target_link_libraries(UnknownTxd PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Threads::Threads)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(UnknownTxd PRIVATE -Wall -Wextra)
endif()
