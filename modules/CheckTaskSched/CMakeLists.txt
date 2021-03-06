cmake_minimum_required(VERSION 2.6)

SET(TARGET CheckTaskSched)
	
PROJECT(${TARGET})

SET(SRCS
	stdafx.cpp
	"${TARGET}.cpp"
	TaskSched.cpp
	filter.cpp

	${NSCP_DEF_PLUGIN_CPP}
	${NSCP_FILTER_CPP}
)

ADD_DEFINITIONS(${NSCP_GLOBAL_DEFINES})

SET(SRCS ${SRCS}
	stdafx.h
	"${TARGET}.h"
	"${TARGET}.def"
	TaskSched.h
	filter.hpp	

	${NSCP_DEF_PLUGIN_HPP}
	${NSCP_FILTER_HPP}
)

add_library(${TARGET} MODULE ${SRCS})

target_link_libraries(${TARGET}
	${Boost_FILESYSTEM_LIBRARY}
	${NSCP_DEF_PLUGIN_LIB}
	${NSCP_FILTER_LIB}
	Mstask.lib
)

INCLUDE(${BUILD_CMAKE_FOLDER}/module.cmake)