cmake_minimum_required(VERSION 2.6)

SET(TARGET CheckEventLog)
	
PROJECT(${TARGET})

SET(SRCS
	stdafx.cpp
	"${TARGET}.cpp"
	filter.cpp
	${NSCP_DEF_PLUGIN_CPP}
	${NSCP_FILTER_CPP}
	eventlog_wrapper.cpp
)

ADD_DEFINITIONS(${NSCP_GLOBAL_DEFINES})

IF(WIN32)
	SET(SRCS ${SRCS}
		stdafx.h
		"${TARGET}.h"
		"${TARGET}.def"

		eventlog_filter.hpp
		eventlog_record.hpp
		eventlog_wrapper.hpp
		simple_registry.hpp
		filter.hpp

		${NSCP_DEF_PLUGIN_HPP}
		${NSCP_FILTER_HPP}
	)
ENDIF(WIN32)

add_library(${TARGET} MODULE ${SRCS})

target_link_libraries(${TARGET}
	${Boost_FILESYSTEM_LIBRARY}
	${NSCP_DEF_PLUGIN_LIB}
	${Boost_PROGRAM_OPTIONS_LIBRARY}
	${Boost_THREAD_LIBRARY}
	${NSCP_FILTER_LIB}
	version.lib
)
INCLUDE(${BUILD_CMAKE_FOLDER}/module.cmake)