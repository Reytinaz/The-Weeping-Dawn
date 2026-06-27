cmake_minimum_required(VERSION 3.16)
project(TheWeepingDawn VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GLAD_SRC "${CMAKE_SOURCE_DIR}/source/include/glad.cpp")
if(NOT EXISTS "${GLAD_SRC}")
    message(FATAL_ERROR "glad.cpp not found at ${GLAD_SRC}.")
endif()

if(WIN32)
    set(SFML_STATIC_LIBRARIES TRUE)
endif()
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
find_package(OpenGL REQUIRED)

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
    ${SFML_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SFML_STATIC)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    OpenGL::GL
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        opengl32
        freetype
        winmm
        gdi32
        flac
        comctl32
        vorbisenc
        vorbisfile
        vorbis
        ogg
        ws2_32
        openal32
        user32
        advapi32
        shell32
    )
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework OpenGL"
        "-framework IOKit"
        "-framework Cocoa"
    )
else()
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES} pthread)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)