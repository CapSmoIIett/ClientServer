#pragma once

#if defined(_WIN32) || defined(_WIN64)
	#define OS_WINDOWS
#elif defined(__unix__) || defined(__unix)
	#define OS_POSIX
#else
	#error unsupported platform
#endif

#define KB 1024
#define MB 1048576



#ifdef OS_POSIX
	#define SOCKADDR_IN -1
	#define SD_BOTH 0

#endif