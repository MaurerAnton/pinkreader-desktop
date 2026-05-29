cmake_minimum_required(VERSION 3.16)
project(pinkreader-desktop VERSION 0.1.0
    DESCRIPTION "PinkReader Desktop — wxWidgets GUI for PinkReader Reddit client"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(wxWidgets REQUIRED COMPONENTS core base stc media)
find_package(CURL REQUIRED)

include(${wxWidgets_USE_FILE})

set(SOURCES
    src/main.cpp
    src/PinkReaderApp.cpp
    src/MainFrame.cpp
    src/PostListPanel.cpp
    src/ImageViewPanel.cpp
    src/SearchPanel.cpp
    src/RedditClient.cpp
)

set(HEADERS
    src/PinkReaderApp.h
    src/MainFrame.h
    src/PostListPanel.h
    src/ImageViewPanel.h
    src/SearchPanel.h
    src/RedditClient.h
)

add_executable(pinkreader-desktop ${SOURCES} ${HEADERS})
target_link_libraries(pinkreader-desktop PRIVATE ${wxWidgets_LIBRARIES} CURL::libcurl)
target_include_directories(pinkreader-desktop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS pinkreader-desktop RUNTIME DESTINATION bin)
