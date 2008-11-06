#ifndef MRPC_API_PRIVATE
#define MRPC_API_PRIVATE

#include "mrpc/mrpc_api.h"

#if !defined(MRPC_API)
	#error mrpc/mrpc_api.h file must define MRPC_API
#endif

#if defined(WIN32)
	#undef MRPC_API
	#define MRPC_API __declspec(dllexport)
#endif

#endif
