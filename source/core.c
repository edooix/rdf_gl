#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

#define BUFSIZE			64

static GLFWwindow*		wnd = NULL;
static unsigned int		mouse_buttons = 0;

static RDFInitF         rdf_init = NULL;
static RDFFinishF		rdf_finish = NULL;
static RDFKeysF			rdf_keys = NULL;
static RDFMouseF		rdf_mouse = NULL;
static RDFResizeF		rdf_resize = NULL;
static RDFMouseScrollF	rdf_mouse_scroll = NULL;
static RDFUpdateF		rdf_update = NULL;
static RDFTimeF			rdf_time = NULL;

/* parse GL_VERSION for supported version (hacky edition) */
static int 
check_opengl_version( int major, int minor )
{
	char *version_string;
	char *tok, *tok_ver = NULL, *next_tok;
	int drv_major, drv_minor;

	/* OpenGL<space>ES<space><version number><space><vendor-specific information>. */
	version_string = (char *)glGetString( GL_VERSION );

	fprintf( stderr, "GL_VERSION: %s\n", version_string );

	tok = strtok_s( version_string, " ", &next_tok );
	while ( tok != NULL ) {
		if ( (strcmp( tok, "OpenGL" ) == 0) ||
			 (strcmp( tok, "ES" ) == 0) ||
			 (strcmp( tok, "GLSL" ) == 0) ) {
			tok = strtok_s( NULL, " ", &next_tok );
		}
		else {
			break;
		}
	}

	tok_ver = strtok_s( tok, ".", &next_tok );
	drv_major = atoi( tok_ver );
	tok_ver = strtok_s( NULL, ".", &next_tok );
	drv_minor = atoi( tok_ver );

	if ( drv_major < major ) {
		return ERR;
	}
	else if ( drv_major == major ){
		if ( drv_minor < minor ) {
			return ERR;
		}
	}

	return OK;
}

static void 
wnd_destroy()
{
	if ( wnd != NULL )
	{
		glfwMakeContextCurrent( NULL );
		glfwDestroyWindow( wnd );
		wnd = NULL;
	}

	glfwTerminate();
}

/* wnd callbacks */
static void 
wnd_close( GLFWwindow *wnd )
{
	glfwSetWindowShouldClose( wnd, GL_TRUE );
}

static void 
wnd_mouse_scroll( GLFWwindow *wnd, double x, double y )
{
	static float wheel = 0.f;

	if ( rdf_mouse_scroll != NULL ) {
		wheel += (float)y;

		rdf_mouse_scroll( wheel );
	}
}

static void 
wnd_mouse_click( GLFWwindow *wnd, int button, int action, int flags )
{
	int key;

	switch ( button )
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		key = RDFKEY_MOUSE_LEFT;
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		key = RDFKEY_MOUSE_RIGHT;
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		key = RDFKEY_MOUSE_MIDDLE;
		break;
	default:
		key = RDFKEY_UNUSED;
		break;
	}

	if ( rdf_keys != NULL ) {
		rdf_keys( key, 0, action, flags );
	}
}

static void 
wnd_mouse_move( GLFWwindow *wnd, double x, double y )
{
	if ( rdf_mouse ) {
		rdf_mouse( (float)x, (float)y );
	}
}

static void 
wnd_keys( GLFWwindow *wnd, int key, int scancode, int action, int flags )
{
	int rdfkey;

	if ( action == GLFW_RELEASE )	{
		if ( key == GLFW_KEY_ESCAPE ) {
			glfwSetWindowShouldClose( wnd, GL_TRUE );
			return;
		}
	}

	switch ( key )
	{
	case GLFW_KEY_F1:
		rdfkey = RDFKEY_F1;
		break;
	case GLFW_KEY_F5:
		rdfkey = RDFKEY_F5;
		break;
	case GLFW_KEY_W:
		rdfkey = RDFKEY_W;
		break;
	case GLFW_KEY_S:
		rdfkey = RDFKEY_S;
		break;
	case GLFW_KEY_A:
		rdfkey = RDFKEY_A;
		break;
	case GLFW_KEY_D:
		rdfkey = RDFKEY_D;
		break;
	case GLFW_KEY_C:
		rdfkey = RDFKEY_C;
		break;
	case GLFW_KEY_LEFT_SHIFT:
		rdfkey = RDFKEY_LEFT_SHIFT;
		break;
	case GLFW_KEY_SPACE:
		rdfkey = RDFKEY_SPACE;
		break;
	case GLFW_KEY_LEFT:
		rdfkey = RDFKEY_LEFT;
		break;
	case GLFW_KEY_RIGHT:
		rdfkey = RDFKEY_RIGHT;
		break;
	case GLFW_KEY_DOWN:
		rdfkey = RDFKEY_DOWN;
		break;
	case GLFW_KEY_UP:
		rdfkey = RDFKEY_UP;
		break;
	default:
		rdfkey = RDFKEY_UNUSED;
		break;
	}

	if ( rdf_keys != NULL ) {
		rdf_keys( rdfkey, scancode, action, flags );
	}
}

static void 
wnd_init( void )
{
	unsigned int width, height;

	if ( rdf_init != NULL ) {
		rdf_init();

		glfwGetWindowSize( wnd, &width, &height );
	}
}

static void 
wnd_finish( void )
{
	if ( rdf_finish != NULL ) {
		rdf_finish();
	}

	wnd_destroy( wnd );
}

static void 
wnd_loop( void )
{
	bool	loop = TRUE;
	float	time, delta, prevtime = .0f;
	float	frame_time = .0f;
	int		fps = 0;

	char	buf[BUFSIZE];
	
	buf[BUFSIZE - 1] = '\0';

	while ( loop ) {
		time = (float)glfwGetTime();

		delta = time - prevtime;
		frame_time += delta;
		prevtime = time;
		fps++;

		if ( frame_time > 1.0f ) {
			_snprintf_s( buf, BUFSIZE, BUFSIZE, "RDF - %d fps", fps );
			glfwSetWindowTitle( wnd, buf );

			fps = 0;
			frame_time = 0;
		}

		if ( rdf_time != NULL ) {
			rdf_time( time );
		}

		if ( rdf_update != NULL && !glfwGetWindowAttrib( wnd, GLFW_ICONIFIED ) ) {
			rdf_update();
		}

		glfwSwapBuffers( wnd );
		glfwPollEvents();

		if ( glfwWindowShouldClose( wnd ) ) {
			loop = FALSE;
		}
	}
}

int 
run( void )
{
	if ( wnd == NULL ) {
		return ERR;
	}

	wnd_init();
	wnd_loop();
	wnd_finish();

	return OK;
}

static int 
wnd_create( const char *title, const unsigned int width, const unsigned int height, const bool fullscreen )
{
	const GLFWvidmode	*system;

	GLFWmonitor			*display = NULL;
	unsigned int		wnd_width = width;
	unsigned int		wnd_height = height;
	int					status = 0;

	if ( wnd != NULL ) {
		fprintf( stderr, "wnd already created\n" );
		return ERR;
	}

	status = glfwInit();
	if ( status != GL_TRUE ) {
		fprintf( stderr, "GLFW: init failed\n" );
		return ERR;
	}

	/*get dekstop size for fullscreen mode*/
	if ( fullscreen )  {
		display = glfwGetPrimaryMonitor();
		system = glfwGetVideoMode( display );

		wnd_width = system->width;
		wnd_height = system->height;
	}

	/*opengl context hints*/
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, OGL_VER_MAJOR );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, OGL_VER_MINOR );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
	glfwWindowHint( GLFW_SAMPLES, 16 );
	glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );

	wnd = glfwCreateWindow( wnd_width, wnd_height, title, display, NULL );
	if ( wnd == NULL ) {
		fprintf( stderr, "GLFW: CreateWindow failed\n" );
		wnd_destroy();
		return ERR;
	}

	/* attach opengl context to wnd */
	glfwMakeContextCurrent( wnd );

	/* probe for opengl extensions */
	glewExperimental = GL_TRUE;
	status = glewInit();
	if ( status != GLEW_OK ) {
		fprintf( stderr, "GLEW: Init failed\n" );
	}

	status = check_opengl_version( OGL_VER_MAJOR, OGL_VER_MINOR );

	if ( status != OK ) {
		fprintf( stderr, "OpenGL v%d.%d not supported by your hardware\n", OGL_VER_MAJOR, OGL_VER_MINOR );
		wnd_destroy( wnd );
		return ERR;
	}

	glfwSetWindowCloseCallback( wnd, wnd_close );
	glfwSetKeyCallback( wnd, wnd_keys );
	glfwSetMouseButtonCallback( wnd, wnd_mouse_click );
	glfwSetCursorPosCallback( wnd, wnd_mouse_move );
	glfwSetScrollCallback( wnd, wnd_mouse_scroll );

	/* vsync */
	glfwSwapInterval( 0 );

	return OK;
}

void 
wnd_size( int *width, int *height )
{
	glfwGetWindowSize( wnd, width, height );

	if ( *width < 1 ) {
		*width = 1;
	}

	if ( *height < 1 ) {
		*height = 1;
	}
}

/*TODO: add cmdline params*/
int 
setup_opengl( void )
{
	return wnd_create( "RDF_GL", 800, 600, FALSE );
}

void 
set_init( RDFInitF callback )
{
	rdf_init = callback;
}

void 
set_finish( RDFFinishF callback )
{
	rdf_finish = callback;
}

void 
set_update( RDFUpdateF callback )
{
	rdf_update = callback;
}

void 
set_keys( RDFKeysF callback )
{
	rdf_keys = callback;
}

void 
set_mouse( RDFMouseF callback )
{
	rdf_mouse = callback;
}

void 
set_mouse_scroll( RDFMouseScrollF callback )
{
	rdf_mouse_scroll = callback;
}

void 
set_time( RDFTimeF callback )
{
	rdf_time = callback;
}