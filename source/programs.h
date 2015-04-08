#ifndef __programs_h_
#define __programs_h_

#include "core.h"

typedef struct
{
	GLuint	prog;
	GLuint	vert;
	GLuint	frag;
	char	*vert_path;
	char	*frag_path;
} program;

typedef struct
{
	size_t	size;
	char	*data;
} textdata;

int	 program_create( program *prg );
int	 program_link( program *prg );
void program_set( program *prg, char *path, int type );
void program_destroy( program *prg );

#endif/*__programs_h_*/