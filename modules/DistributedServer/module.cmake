IF(ZEROMQ_FOUND)
	SET (BUILD_MODULE 1)
ELSE(ZEROMQ_FOUND)
	MESSAGE(STATUS "Disabling DistributedServer since zeromq was not found")
ENDIF(ZEROMQ_FOUND)



