cmake_minimum_required(VERSION 2.6)

SET(TARGET main_dll)
	
PROJECT(${TARGET})

SET(SRCS
	${TARGET}.cpp
	${NSCP_INCLUDEDIR}/ServiceCmd.cpp

)

ADD_DEFINITIONS(${NSCP_GLOBAL_DEFINES})

IF(WIN32)
	SET(SRCS ${SRCS}
		"${TARGET}.def"
		installer_helper.hpp
		${NSCP_INCLUDEDIR}/ServiceCmd.h
	)
ENDIF(WIN32)

if(NOT CMAKE_CL_64)
INCLUDE_DIRECTORIES(${PSDK_2003_INCLUDE_DIRS})
LINK_DIRECTORIES(${PSDK_2003_LIBRARY_DIRS})
ENDIF(NOT CMAKE_CL_64)
ADD_LIBRARY(${TARGET} MODULE ${SRCS})

TARGET_LINK_LIBRARIES(${TARGET}
	${Boost_FILESYSTEM_LIBRARY}
	${Boost_SYSTEM_LIBRARY}
	Msi.lib
	bufferoverflowu.lib
	settings_manager
)

SET_TARGET_PROPERTIES(${TARGET} PROPERTIES FOLDER "installer-dll")
