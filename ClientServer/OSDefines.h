#pragma once

#if defined(_WIN32) || defined(_WIN64)
	#define OS_WINDOWS
#elif defined(__unix__) || defined(__unix)
	#define OS_POSIX
#else
	#error unsupported platform
#endif


#ifdef OS_WINDOWS	 
#define WIN(exp) exp
#define NIX(exp)

#else
#define WIN(exp)
#define NIX(exp) exp
#endif


#define KB 1024
#define MB 1048576


