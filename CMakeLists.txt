cmake_minimum_required(VERSION 3.16)
project(Synera_Starter VERSION 1.0)

# CXX configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QT configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# QT6 dependency
find_package(Qt6 COMPONENTS Core Widgets Gui REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)

file(GLOB_RECURSE HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx"
)

add_executable(Synera_Starter ${SOURCES} ${HEADERS})

target_include_directories(Synera_Starter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Synera_Starter PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

add_custom_command(TARGET Synera_Starter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/boards"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/core/default_board.json"
        "$<TARGET_FILE_DIR:Synera_Starter>/boards/default_board.json"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src/core/boards"
        "$<TARGET_FILE_DIR:Synera_Starter>/boards"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/events"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/world/event/events.json"
        "$<TARGET_FILE_DIR:Synera_Starter>/events/events.json"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/battle"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/combat/battle.json"
        "$<TARGET_FILE_DIR:Synera_Starter>/battle/battle.json"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/hextech"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/world/hextech/hextech.json"
        "$<TARGET_FILE_DIR:Synera_Starter>/hextech/hextech.json"
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/assets/hextech/cards"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Synera_Starter>/assets"
)

add_custom_target(copy_runtime_assets ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/boards"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/core/default_board.json"
        "$<TARGET_FILE_DIR:Synera_Starter>/boards/default_board.json"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src/core/boards"
        "$<TARGET_FILE_DIR:Synera_Starter>/boards"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/events"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/world/event/events.json"
        "$<TARGET_FILE_DIR:Synera_Starter>/events/events.json"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/battle"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/combat/battle.json"
        "$<TARGET_FILE_DIR:Synera_Starter>/battle/battle.json"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/hextech"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/world/hextech/hextech.json"
        "$<TARGET_FILE_DIR:Synera_Starter>/hextech/hextech.json"
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        "$<TARGET_FILE_DIR:Synera_Starter>/assets/hextech/cards"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Synera_Starter>/assets"
)

if(WIN32)
    set_target_properties(Synera_Starter PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(UNIX)
    target_link_libraries(Synera_Starter PRIVATE pthread)
target_link_libraries(Synera_Starter PRIVATE Qt6::Widgets)
endif()

if(APPLE)
    set_target_properties(Synera_Starter PROPERTIES MACOSX_BUNDLE TRUE)
endif()
