cmake_minimum_required(VERSION 3.16)
project(VideoPlayerQt LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

add_executable(VideoPlayerQt
    src/main.cpp
    src/mainwindow.cpp
    src/playbackcanvas.cpp
)

target_include_directories(VideoPlayerQt PRIVATE src)

target_link_libraries(VideoPlayerQt PRIVATE
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::MultimediaWidgets
)
