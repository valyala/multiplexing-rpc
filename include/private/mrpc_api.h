#ifndef MRPC_API_PRIVATE
#define MRPC_API_PRIVATE

#include "mrpc/mrpc_api.h"

#if !defined(MRPC_API)
	#error mrpc/mrpc_api.h file must define MRPC_API
#endif

/* undefine public definition of the MRPC_API */
#undef MRPC_API

#if defined(WIN32)
	#define MRPC_API __declspec(dllexport)
#else
	#define MRPC_API __attribute__((visibility("default")))
#endif

#endif
