cmake_minimum_required(VERSION 3.28)
project(TextTracer LANGUAGES CXX)

add_executable(
    Main

    main.cpp

    Sources/TextTracer.cpp

    Sources/Debugging/DebugBox.cpp

    Sources/Framebuffer/ConsoleFramebuffer.cpp
    Sources/Framebuffer/GLFWFramebuffer.cpp

    Sources/Geometry/AABB.cpp
    Sources/Geometry/AABBGeometry.cpp
    Sources/Geometry/ConvexPolyGeometry.cpp
    Sources/Geometry/DiscGeometry.cpp
    Sources/Geometry/Geometry.cpp
    Sources/Geometry/PlaneGeometry.cpp
    Sources/Geometry/SphereGeometry.cpp

    Sources/Objects/AABBObject.cpp
    Sources/Objects/Camera.cpp
    Sources/Objects/ConvexPolyObject.cpp
    Sources/Objects/DiscObject.cpp
    Sources/Objects/PlaneObject.cpp
    Sources/Objects/SphereObject.cpp
    Sources/Objects/WorldObject.cpp

    Sources/Raytracer/Raytracer.cpp

    Sources/Scenes/KdTree.cpp
    Sources/Scenes/Scene.cpp
    Sources/Scenes/TestScene.cpp
)

target_include_directories(
    Main

    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Headers"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Headers/Debugging"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Headers/Framebuffer"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Headers/Geometry"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Headers/Objects"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Headers/Raytracer"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Headers/Scenes"
)

find_package(PkgConfig REQUIRED)

pkg_search_module(GL REQUIRED opengl)
pkg_search_module(GLFW REQUIRED glfw3)

include_directories(${GLFW_INCLUDE_DIRS})
target_link_libraries(Main ${GL_LIBRARIES})
target_link_libraries(Main ${GL_STATIC_LIBRARIES})
target_link_libraries(Main ${GLFW_LIBRARIES})
target_link_libraries(Main ${GLFW_STATIC_LIBRARIES})
