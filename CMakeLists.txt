cmake_minimum_required(VERSION 3.16)
project(TheWeepingDawn VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── GLAD ──
set(GLAD_SRC "${CMAKE_SOURCE_DIR}/source/include/glad.cpp")
if(NOT EXISTS "${GLAD_SRC}")
    message(FATAL_ERROR "glad.cpp not found at ${GLAD_SRC}")
endif()

# ── SFML ──
if(WIN32)
    set(SFML_STATIC_LIBRARIES TRUE)
endif()
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
find_package(OpenGL REQUIRED)

# ── Source ──
file(GLOB_RECURSE ENGINE_SRC
    "source/engine/src/*.cpp"
    "source/engine/instances/*.cpp"
)
list(APPEND ENGINE_SRC "main.cpp")
list(APPEND ENGINE_SRC "${GLAD_SRC}")
add_executable(${PROJECT_NAME} ${ENGINE_SRC})

# ── Headers ──
target_include_directories(${PROJECT_NAME} PRIVATE
    source/engine/include
    source/engine/instances
    source/include
    ${SFML_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)

# ── Link ──
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_STATIC)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        sfml-graphics-s
        sfml-window-s
        sfml-system-s
        sfml-audio-s
        sfml-network-s

        opengl32.lib
        freetype.lib
        winmm.lib
        gdi32.lib
        flac.lib
        comctl32.lib
        vorbisenc.lib
        vorbisfile.lib
        vorbis.lib
        ogg.lib
        ws2_32.lib
        openal32.lib
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        OpenGL::GL
    )
    if(UNIX AND NOT APPLE)
        find_package(X11 REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES} pthread)
    endif()
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)