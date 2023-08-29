cmake_minimum_required(VERSION 3.5)

project(btcpp_sample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(btcpp_sample main.cpp)

find_package(ament_cmake QUIET)

if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo
MinSizeRel." FORCE )
endif()

if( CMAKE_BUILD_TYPE MATCHES Release)
    set(behaviortree_cpp_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib_release/cmake/behaviortree_cpp")
else()
    set(behaviortree_cpp_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib_debug/cmake/behaviortree_cpp")
endif()

if(ament_cmake_FOUND)

    find_package(behaviortree_cpp REQUIRED)
    ament_target_dependencies(btcpp_sample behaviortree_cpp)

elseif( CATKIN_DEVEL_PREFIX OR CATKIN_BUILD_BINARY_PACKAGE)

    find_package(catkin REQUIRED COMPONENTS behaviortree_cpp)
    catkin_package(CATKIN_DEPENDS behaviortree_cpp)
    target_include_directories(btcpp_sample PRIVATE ${catkin_INCLUDE_DIRS})
    target_link_libraries(btcpp_sample ${catkin_LIBRARIES})

else()

    find_package(behaviortree_cpp REQUIRED)
    target_link_libraries(btcpp_sample BT::behaviortree_cpp)

endif()

