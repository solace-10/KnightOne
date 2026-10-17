project(game)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src FILES ${SOURCE_FILES})

find_package(pandora REQUIRED)
add_executable(game ${SOURCE_FILES})

target_include_directories(
    game PRIVATE 
    src/
    ${PANDORA_INCLUDE_DIRS}
)

target_link_directories(game PRIVATE ${PANDORA_LIBRARY_DIRS})
target_link_libraries(game PRIVATE pandora)

if(TARGET_PLATFORM_NATIVE)
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:game> ${CMAKE_CURRENT_LIST_DIR}/bin
    )
elseif(TARGET_PLATFORM_WEB)
    target_link_options(game PRIVATE
        "--bind" # Support for embind so we can interact with the frontend.
    )

    # The page itself is the SvelteKit app in webapp/, which loads game.js from its static/ folder.
    set(WEBAPP_STATIC_DIR "${CMAKE_CURRENT_LIST_DIR}/../webapp/static")
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${WEBAPP_STATIC_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:game>/$<TARGET_FILE_BASE_NAME:game>.js ${WEBAPP_STATIC_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:game>/$<TARGET_FILE_BASE_NAME:game>.wasm ${WEBAPP_STATIC_DIR}
    )
endif()
