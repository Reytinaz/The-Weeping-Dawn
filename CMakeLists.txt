cmake_minimum_required(VERSION 3.16)
project(TheWeepingDawn VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GLAD_SRC "${CMAKE_SOURCE_DIR}/source/include/glad.cpp")
if(NOT EXISTS "${GLAD_SRC}")
    message(FATAL_ERROR "glad.cpp not found at ${GLAD_SRC}.")
endif()

file(GLOB_RECURSE ENGINE_SRC
    "source/engine/src/*.cpp"
    "source/engine/instances/*.cpp"
)
list(APPEND ENGINE_SRC "main.cpp")
list(APPEND ENGINE_SRC "${GLAD_SRC}")
add_executable(${PROJECT_NAME} ${ENGINE_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
    source/engine/include
    source/engine/instances
    source/include
)

if(WIN32)
    set(SFML_ROOT "${CMAKE_SOURCE_DIR}/source/include/SFML-3.1.0")
    set(LIBS_DIR "${CMAKE_SOURCE_DIR}/source/libraries")
    target_include_directories(${PROJECT_NAME} PRIVATE "${SFML_ROOT}/include")
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_STATIC)
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${SFML_ROOT}/extlibs"
        "${SFML_ROOT}/lib"
        "${LIBS_DIR}"
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        sfml-graphics-s
        sfml-window-s
        sfml-system-s
        sfml-audio-s
        sfml-network-s
        opengl32
        freetype
        winmm
        gdi32
        flac
        harfbuzz
        comctl32
        vorbisenc
        vorbisfile
        vorbis
        ogg
        ws2_32
        openal32
    )
elseif(APPLE)
    find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
    find_package(OpenGL REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SFML_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        SFML::Graphics SFML::Window SFML::System OpenGL::GL
        "-framework OpenGL" "-framework IOKit" "-framework Cocoa"
    )
else()
    find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SFML_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        SFML::Graphics SFML::Window SFML::System OpenGL::GL
        ${X11_LIBRARIES} pthread
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)