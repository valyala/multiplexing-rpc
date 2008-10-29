#ifndef MRPC_API_PUBLIC
#define MRPC_API_PUBLIC

#if defined(MRPC_BUILD_DLL)
	#define MRPC_API __declspec(dllexport)
#elif defined(MRPC_USE_DLL)
	#define MRPC_API __declspec(dllimport)
#else
	#define MRPC_API extern
#endif

#endif
