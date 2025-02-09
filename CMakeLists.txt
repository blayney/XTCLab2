cmake_minimum_required(VERSION 3.15)

# ------------------------------------------------------------------------------
# Project name & version
# ------------------------------------------------------------------------------
project(HelloJUCE VERSION 1.0.0)

# ------------------------------------------------------------------------------
# 1) Locate or fetch JUCE
# ------------------------------------------------------------------------------
# Option A: Local path to JUCE
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
add_subdirectory(${JUCE_PATH} JUCE)

# Option B: FetchContent (not needed if you have a local JUCE)
# include(FetchContent)
# FetchContent_Declare(
#     JUCE
#     GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
#     GIT_TAG        7.0.5
# )
# FetchContent_MakeAvailable(JUCE)

# ------------------------------------------------------------------------------
# 2) Add a JUCE-based GUI app
# ------------------------------------------------------------------------------
juce_add_gui_app(HelloJUCE
    PRODUCT_NAME "HelloJUCE"
    COMPANY_NAME "MyCompany"
)

# Your source files
target_sources(HelloJUCE PRIVATE
    Source/Main.cpp
)

# ------------------------------------------------------------------------------
# 3) Link JUCE modules
# ------------------------------------------------------------------------------
target_link_libraries(HelloJUCE 
    PRIVATE
        juce::juce_core
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
)

# ------------------------------------------------------------------------------
# 4) Find and link OpenCV
# ------------------------------------------------------------------------------
find_package(OpenCV REQUIRED)  # Looks for system-wide OpenCV installation
target_include_directories(HelloJUCE PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(HelloJUCE PRIVATE ${OpenCV_LIBS})