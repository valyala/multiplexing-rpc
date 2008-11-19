#include "common.h"
#include "parser.h"
#include "c_server_generator.h"
#include "c_client_generator.h"

#include "types.h"

/*
 * interface_compiler.exe {c_client|c_server} filename
 */

void help(char *app_name)
{
	printf("\nUsage: %s filename output_type\n\n", app_name);
	printf(
		"The multiplexing-rpc interface compiler generates source files,\n"
		"which is required for either accessing mrpc service (client source files)\n"
		"either implementing mrpc service (server source files).\n"
		"The interface compiler writes generated files into current directory.\n"
		"Be careful: it owerwrites existing files with the same name.\n"
		"The interface compiler doesn't support unicode.\n"
		"\n"
		"- filename must contain path to the interface definition file.\n"
		"  It is preferred to invoke the interface compiler from the directory,\n"
		"  which contains the given interface definition file.\n"
		"\n"
		"- output_type can have the following values:\n"
		"  - c_client - generates client source files for the C mrpc library:\n"
		"    - client_interface_{interface_name}.h contains single declaration of\n"
		"      the client mrpc_interface constructor for the given interface definition.\n"
		"\n"
		"    - client_interface_{interface_name}.c contains definition of\n"
		"      the client mrpc_interface constructor. Do not modify this file!\n"
		"\n"
		"    - client_service_{interface_name}.h contains declarations of rpc\n"
		"      wrapper functions defined in the interface definition file.\n"
		"      These functions must be called in order to perform rpc.\n"
		"\n"
		"    - client_service_{interface_name}.c contains definition of\n"
		"      the rpc wrapper functions. Do not modify this file!\n"
		"\n"
		"  - c_server - generates server source files for the C mrpc library:\n"
		"    - server_interface_{interface_name}.h contains single declaration of\n"
		"      the server mrpc_interface constructor for the given interface definition.\n"
		"\n"
		"    - server_interface_{interface_name}.c contains definition of\n"
		"      the server mrpc_interface constructor. Do not modify this file!\n"
		"\n"
		"    - server_service_{interface_name}.h contains declaration of the service,\n"
		"      which must implement rpc interface defined in the interface definition\n"
		"      file on the server.\n"
		"\n"
		"    - server_service_{interface_name}.c contains stubs for the service,\n"
		"      which implement rpc interface defined in the interface definition file.\n"
		"      These stubs must be substituted by real implementation of the given\n"
		"      rpc interface.\n"
	);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	const struct interface *interface;
	const char *filename;
	const char *output_type;

	if (argc != 3)
	{
		help(argv[0]);
	}

	filename = argv[1];
	interface = parse_interface(filename);

	output_type = argv[2];
	if (strcmp(output_type, "c_client") == 0)
	{
		c_client_generator_generate(interface);
	}
	else if (strcmp(output_type, "c_server") == 0)
	{
		c_server_generator_generate(interface);
	}
	else
	{
		fprintf(stderr, "error: unexpected output_type [%s]. Expected: c_client, c_server\n", output_type);
		help(argv[0]);
	}

	interface_delete(interface);

	return 0;
}
