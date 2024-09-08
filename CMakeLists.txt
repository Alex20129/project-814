CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(project-814)

SET(CMAKE_CXX_FLAGS "-Wall -O0 -no-pie")
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt5 COMPONENTS Core Network REQUIRED)

LINK_LIBRARIES(Qt5::Core Qt5::Network)
IF(WIN32)
	LINK_LIBRARIES(-static-libgcc -static-libstdc++)
ENDIF(WIN32)

ADD_EXECUTABLE("asicconf"
		main.cpp
		asicdevice.cpp
		globals.cpp
		logger.cpp
		scanner.cpp
		asicdevice.h
		globals.h
		logger.h
		scanner.h)
