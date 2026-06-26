cmake_minimum_required(VERSION 3.16)
project(TheWeepingDawn VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── SFML (ищется в системе) ──
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)


# ── OpenGL ──
find_package(OpenGL REQUIRED)

# ── Исходные файлы ──
file(GLOB_RECURSE ENGINE_SRC
    "source/engine/src/*.cpp"
    "source/engine/instances/*.cpp"
)
list(APPEND ENGINE_SRC "source/include/glad.c")
list(APPEND ENGINE_SRC "main.cpp")
add_executable(${PROJECT_NAME} ${ENGINE_SRC})


# ── Заголовки ──
target_include_directories(${PROJECT_NAME} PRIVATE
    source/engine/include
    source/engine/instances
    source/include
    ${SFML_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)
# ── Линковка ──
target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    OpenGL::GL
)

# ── Платформенно‑специфичные настройки ──
if(WIN32)
    # Windows: дополнительно линкуем comctl32 и копируем ресурсы
    target_link_libraries(${PROJECT_NAME} PRIVATE comctl32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    )
elseif(APPLE)
    # macOS: копируем ресурсы внутрь .app бандла (если нужно)
    # Для простоты можно тоже скопировать assets в папку сборки
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    )
else()
    # Linux: линкуем X11 и pthread, копируем assets
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES} pthread)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    )
endif()