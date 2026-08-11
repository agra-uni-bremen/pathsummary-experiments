file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH PARENT_DIR)
cmake_path(GET PARENT_DIR PARENT_PATH PARENT_DIR)

add_library(peripherals_sysc
        ${SOURCES}
        ${HEADERS}
)
target_include_directories(peripherals_sysc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(peripherals_sysc PRIVATE ${SystemC_INCLUDE_DIR})
target_link_libraries(peripherals_sysc PUBLIC ${SystemC_LIBRARIES})
