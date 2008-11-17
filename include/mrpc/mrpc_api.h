#ifndef MRPC_API_PUBLIC_H
#define MRPC_API_PUBLIC_H

#if defined(WIN32)
	#define MRPC_API __declspec(dllimport)
#else
	#define MRPC_API __attribute__((externally_visible))
#endif

#endif
