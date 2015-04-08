#include "core.h"
#include "impl.h"

int 
main( int argc, char* argv[] )
{
	int status = 0;

	setup_opengl();

	impl_setup();
	impl_printkeys();

	status = run();

	return status;
}