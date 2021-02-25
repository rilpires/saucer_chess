cmake_minimum_required( VERSION 3.5 )

project(saucer_chess)

add_library(saucer_chess OBJECT chess.cpp)

set(BOOST_ROOT "" CACHE PATH "Path to boost root folder" )
set(BOOST_INCLUDEDIR "" CACHE PATH "Path to boost include folder" )
set(BOOST_LIBRARYDIR "" CACHE PATH "Path to boost libs directories" )

find_package(Boost REQUIRED)

target_include_directories( saucer_chess PUBLIC ${BOOST_ROOT} ${BOOST_INCLUDEDIR} $<TARGET_PROPERTY:saucer_core,INTERFACE_INCLUDE_DIRECTORIES> )
target_link_directories( saucer_chess PUBLIC ${BOOST_LIBRARYDIR} )
