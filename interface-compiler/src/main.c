#include "common.h"
#include "parser.h"
#include "c_server_generator.h"
#include "c_client_generator.h"

#include "types.h"

int main()
{
	struct interface *interface;

	interface = parse_interface("test.txt");

	c_server_generator_generate(interface);
	c_client_generator_generate(interface);

	interface_delete(interface);

	return 0;
}
