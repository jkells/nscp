CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(TARGET nsclient_installer)
	
PROJECT(${TARGET})

SET(SRCS
	Product.wxs
	WixUI.wixlib
)
SET(DEPENDENCIES
	nscp
)
GET_FILENAME_COMPONENT(_tmp_FILE WixUI_en-us.wxl ABSOLUTE)

SET(LOCALIZATIONS
	${_tmp_FILE}
)

#MESSAGE(STATUS "Wix source: ${SRCS}")
#WIX_COMPILE(SRCS WIX_OBJS WixUI_en-us.wxl)
#MESSAGE(STATUS "Wix obj: ${WIX_OBJS}")

#SET (WIX_FULLOBJLIST ${WIXOBJ_LIST} )
#SET (WIX_DEST ${TARGET}.msi)

SET(ARCH Win32)
IF(CMAKE_CL_64)
	SET(ARCH x64)
ENDIF(CMAKE_CL_64)


SET(WIX_CANDLE_FLAGS -dboost=true -darch=${ARCH} -dVersion.Major=1 -dVersion.Minor=1 -dVersion.Revision=1 -dVersion.Build=1 
	-dSource=${NSCP_PROJECT_BINARY_DIR} -dHelpers=${NSCP_PROJECT_BINARY_DIR}/installer-dlls
	)
SOURCE_GROUP(Generated FILES ${SRCS})
#set_source_files_properties(${WIXOBJ_LIST} ${WIX_DEST} PROPERTIES GENERATED 1)
	
SOURCE_GROUP(Sources FILES ${SRCS})
ADD_WIX_INSTALLER(${TARGET} "${SRCS}" "${DEPENDENCIES}" "${LOCALIZATIONS}")

ADD_DEPENDENCIES(${TARGET} ${MAIN_NSCP_TARGET} ${ALL_MODULE_NAMES} ${ALL_TOOL_NAMES} ${ALL_INSTALLERS_DLLS_NAMES})

SET_TARGET_PROPERTIES(${TARGET} PROPERTIES FOLDER "installers")