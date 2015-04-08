#include <stdio.h>
#include <stdlib.h>

#include "programs.h"

#define BLOCKSIZE	64

static void 
textdata_destroy( textdata *text )
{
	if ( text != NULL ) {
		if ( text->size > 0 && text->data != NULL ) {
			free( text->data );
			text->size = 0;
		}
	}
}

static int 
file_load( const char *path, textdata *text )
{
	FILE*	file;
	size_t	read, remaining, block, pos;

	if ( !path || !text )
	{
		return -1;
	}

	text->data = 0;
	text->data = NULL;

	fopen_s( &file, path, "rb" );

	if ( file ) {
		fseek( file, 0, SEEK_END );
		text->size = ftell( file );
		fseek( file, 0, SEEK_SET );

		text->data = (char *)calloc( text->size + 1, sizeof(char) );
		if ( text->data ) {
			remaining = text->size;
			pos = 0;

			while ( remaining ) {
				block = BLOCKSIZE;
				if ( remaining <= BLOCKSIZE ) {
					block = remaining;
				}

				read = fread( text->data + pos, sizeof(char), block, file );
				if ( read < 0 ) {
					fclose( file );
					textdata_destroy( text );
					return -1;
				}
				remaining -= read;
				pos += read;
			}
		}

		fclose( file );

		return 0;
	}
	else {
		fprintf( stderr, "File \"%s\" not found\n", path );
		return -1;
	}
}

static int 
shader_status( GLuint handle, GLenum type )
{
	GLuint	status;
	size_t	logsize, bytes_written;
	char	*log;

	glGetShaderiv( handle, type, &status );
	if ( !status )
	{
		glGetShaderiv( handle, GL_INFO_LOG_LENGTH, &logsize );

		log = (char*)malloc( logsize );

		if ( !log )
		{
			return -1;
		}

		glGetShaderInfoLog( handle, logsize, &bytes_written, log );

		fprintf( stderr, "=============================================================================\n" );
		fprintf( stderr, "%s", log );

		free( log );

		return -1;
	}

	return 0;
}

static int 
program_status( GLuint handle )
{
	GLuint	status;
	size_t	logsize, bytes_written;
	char	*log;

	glGetProgramiv( handle, GL_LINK_STATUS, &status );
	if ( !status )
	{
		glGetProgramiv( handle, GL_INFO_LOG_LENGTH, &logsize );

		log = (char*)malloc( logsize );

		if ( !log )
		{
			return -1;
		}

		glGetProgramInfoLog( handle, logsize, &bytes_written, log );

		fprintf( stderr, "Failed to link shader program!\n" );
		fprintf( stderr, "%s", log );

		free( log );

		return -1;
	}

	return 0;
}

static int 
program_compile( program *prg, textdata *src, int type )
{
	GLuint		prg_handle;
	int			err = 0;

	prg_handle = glCreateShader( type );
	glShaderSource( prg_handle, 1, &(src->data), NULL );
	glCompileShader( prg_handle );
	err = shader_status( prg_handle, GL_COMPILE_STATUS );

	switch ( type ) {
	case GL_VERTEX_SHADER:
		prg->vert = (err == 0) ? prg_handle : 0;
		break;
	case GL_FRAGMENT_SHADER:
		prg->frag = (err == 0) ? prg_handle : 0;
		break;
	default:
		break;
	}

	textdata_destroy( src );

	return err;
}

int 
program_create( program *prg )
{
	textdata	src;
	int			err = 0;

	if ( prg->frag_path != NULL ) {
		err = file_load( prg->frag_path, &src );
		if ( err != 0 ) {
			fprintf( stderr, "frag.glsl could not be loaded!\n" );
			return err;
		}

		err = program_compile( prg, &src, GL_FRAGMENT_SHADER );
	}


	if ( prg->vert_path != NULL ) {
		err = file_load( prg->vert_path, &src );
		if ( err != 0 ) {
			fprintf( stderr, "vert.glsl could not be loaded!\n" );
			return err;
		}

		err = program_compile( prg, &src, GL_VERTEX_SHADER );
	}

	if ( err == 0 ) {
		prg->prog = glCreateProgram();

		glAttachShader( prg->prog, prg->vert );
		glAttachShader( prg->prog, prg->frag );
	}

	return err;
}

int 
program_link( program *prg )
{
	int	err = 0;

	glLinkProgram( prg->prog );
	err = program_status( prg->prog );

	return err;
}

void 
program_destroy( program *prg )
{
	if ( prg->prog ) {
		glDeleteProgram( prg->prog );
		prg->prog = 0;
	}

	if ( prg->frag ) {
		glDeleteShader( prg->frag );
		prg->frag = 0;
	}

	if ( prg->vert ) {
		glDeleteShader( prg->vert );
		prg->vert = 0;
	}
}

void 
program_set( program *prg, char *path, int type )
{
	switch ( type )
	{
	case GL_VERTEX_SHADER:
		prg->vert_path = path;
		break;
	case GL_FRAGMENT_SHADER:
		prg->frag_path = path;
		break;
	default:
		break;
	}
}