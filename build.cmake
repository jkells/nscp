IF(WIN32)
	set(Boost_USE_STATIC_LIBS		ON)
	set(Boost_USE_STATIC_RUNTIME	ON)
	set(BOOST_USE_MULTITHREADED		ON)
	SET(NSCP_GLOBAL_DEFINES ${NSCP_GLOBAL_DEFINES} -DBOOST_ALL_NO_LIB)	# THis is used to disable "automatic linking on windows which seems to break since I dont know how to set link dir
	SET(INC_NSCP_INCLUDEDIR D:/source/include)
	SET(INC_PSDK_61 "C:/Program Files/Microsoft SDKs/Windows/v6.1/")
	SET(INC_PSDK_2003 "C:/Program Files/Microsoft Platform SDK/")

	if(CMAKE_CL_64)
		MESSAGE(STATUS "Detected x64")
		SET(INC_NSCP_LIBRARYDIR D:/source/lib/x64)	
		SET(PYTHON_ROOT c:/python/27x64)	
	ELSE()
		MESSAGE(STATUS "Detected w32")
		SET(INC_NSCP_LIBRARYDIR D:/source/lib/x86)
		SET(PYTHON_ROOT c:/python/27)	
	ENDIF()

	SET(INC_GOOGLE_BREAKPAD_DIR "D:/source/libraries/google-breakpad-svn")
		
	SET(INC_BOOST_INCLUDEDIR "D:/source/include/boost-1_47")
	SET(INC_BOOST_LIBRARYDIR "${INC_NSCP_LIBRARYDIR}")
	SET(INC_PROTOBUF_LIBRARYDIR "${INC_NSCP_LIBRARYDIR}")

	SET(INC_OPENSSL_INCLUDEDIR "${INC_NSCP_INCLUDEDIR}")
	
	SET(ZEROMQ_INCLUDE_DIR "D:/source/libraries/zeromq-2.1.9/include")

	SET(INC_PROTOBUF_DIR "D:/source/libraries/protobuf-2.4.0a")

	SET(INC_CRYPTOPP_DIR "D:/source/libraries/crypto++-5.6.1")

	SET(INC_LUA_DIR "D:/source/libraries/lua-5.1.4")
	
	SET(ARCHIVE_FOLDER "D:/archive")
	
	SET(TARGET_SITE "nscp@nsclient.org:/var/nsclient/www/files/nightly/;op5=mickem@home.medin.name:/home/nscp-dist/op5/nscp/;opsera=mickem@home.medin.name:/home/nscp-dist/opsera/nscp/")

ELSE(WIN32)

	SET(INC_OPENSSL_INCLUDEDIR "/usr/include/")
	SET(PROTOBUF_INCLUDE_DIR "/usr/include/")

ENDIF(WIN32)

