cmake_minimum_required(VERSION 3.16)
project(UnknownTxd LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/External/imgui)
add_library(ImGui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(ImGui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(ImGui PUBLIC glfw OpenGL::GL)

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
    Source/Application/Application.cpp
    Source/Application/MainWindow.cpp
    Source/Application/GlTexture.cpp
    Source/Application/FileDialog.cpp
)
target_include_directories(UnknownTxd PRIVATE ${CMAKE_SOURCE_DIR}/Source ${CMAKE_SOURCE_DIR}/External/stb)
target_link_libraries(UnknownTxd PRIVATE ImGui OpenGL::GL Threads::Threads)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(UnknownTxd PRIVATE -Wall -Wextra)
endif()
