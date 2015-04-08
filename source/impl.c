#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "core.h"
#include "programs.h"
#include "impl_local.h"

/*default camera*/
static const vec3_t def_pos		= {  5.0f,  15.0f,  5.0f };
static const vec3_t def_lookat	= {  0.0f,  0.0f,  0.0f };

static const vec3_t def_dir		= {  0.0f,  0.0f,  1.0f };
static const vec3_t def_right	= { -1.0f,  0.0f,  0.0f };
static const vec3_t def_up		= {  0.0f,  1.0f,  0.0f };

static const vec3_t def_angles	= {  -35.0f,  45.0f,  0.0f };

/*camera move speed*/
static const float move_step	= 5.0f;
static const float pan_mod		= 0.5f;
static const float rot_mod		= 0.1f;
static const float zoom_mod		= 8.0f;

static key_t	keydata[256];

static frame_t	frame			= { 0 };
static frame_t	prevframe		= { 0 };

static int		debugmode		= 0;

/*opengl objects*/
static program	progs = { 0 };
/*vertex array objects*/
static GLuint	vao;
/*vertex array buffers*/
static GLuint	vertices, indices;
/*uniform locations*/
static GLint	tex1_l, tex2_l, tex3_l;
static GLint	vp_l;
static GLint	resolution_l, time_l, debug_l;
static GLint	packy_pos_l, packy_angles_l, gogu_pos_l, gogu_angles_l;
static GLint	bonbons_l, bonbon_count_l;

/*uniform blocks*/
static ubo_t    ubo_cam;

/*****************************************************************************/
/*locals*/
static void 
load_texture( const char *path, const GLuint tex_unit_enum, const GLuint tex_unit_id, const GLint tex_l )
{
	int width, height;
	static unsigned char *img = NULL;
	GLuint tex;

	img = SOIL_load_image( path, &width, &height, 0, SOIL_LOAD_RGB );
	if ( img )
	{
		glActiveTexture( tex_unit_enum );
		glGenTextures( 1, &tex );
		glBindTexture( GL_TEXTURE_2D, tex );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

		glUniform1i( tex_l, tex_unit_id );

		SOIL_free_image_data( img );
	}
}

static void 
load_textures()
{
	load_texture( "../textures/Brick_Design_UV_H_CM_1.png", GL_TEXTURE0, 0, tex1_l );
	load_texture( "../textures/Ground10_1.png", GL_TEXTURE1, 1, tex2_l );
	load_texture( "../textures/Moss_01_UV_H_CM_1.png", GL_TEXTURE2, 2, tex3_l );
}

static void 
load_shaders()
{
	program_create( &progs );
	program_link( &progs );

	glUseProgram( progs.prog );

	/*update uniforms locations*/
	vp_l = glGetAttribLocation( progs.prog, "_vp" );
	tex1_l = glGetUniformLocation( progs.prog, "_tex1" );
	tex2_l = glGetUniformLocation( progs.prog, "_tex2" );
	tex3_l = glGetUniformLocation( progs.prog, "_tex3" );
	debug_l = glGetUniformLocation( progs.prog, "_debug" );
	resolution_l = glGetUniformLocation( progs.prog, "_resolution" );
	time_l = glGetUniformLocation( progs.prog, "_time" );
	packy_pos_l = glGetUniformLocation( progs.prog, "_packy_pos" );
	packy_angles_l = glGetUniformLocation( progs.prog, "_packy_angles" );
	gogu_pos_l = glGetUniformLocation( progs.prog, "_gogu_pos" );
	gogu_angles_l = glGetUniformLocation( progs.prog, "_gogu_angles" );
	bonbons_l = glGetUniformLocation( progs.prog, "_bonbons" );
	bonbon_count_l = glGetUniformLocation( progs.prog, "_bonbon_count" );

	ubo_cam.loc = glGetUniformBlockIndex( progs.prog, "camera_block" );
}

static void 
view_init()
{
	vec3_mov( frame.view.pos, def_pos );
	vec3_mov( frame.view.lookat, def_lookat );

	vec3_sub( frame.view.dir, def_lookat, def_pos );
	vec3_normalize( frame.view.dir );

	vec3_cross( frame.view.right, frame.view.dir, def_up );
	vec3_normalize( frame.view.right );

	vec3_cross( frame.view.up, frame.view.right, frame.view.dir );
	vec3_normalize( frame.view.up );

	vec3_mov( frame.view.angles, def_angles );
}

static void 
in_update()
{
	vec3_t dir, right, up, tmp;
	vec3_t offset = { 0 };

	float deltatime = frame.time - prevframe.time;
	float deltawheel = frame.mouse.sy - prevframe.mouse.sy;

	float dx = frame.mouse.x - prevframe.mouse.x;
	float dy = frame.mouse.y - prevframe.mouse.y;

	float yaw = 0.f;
	float pitch = 0.f;

	if ( keydata[RDFKEY_MOUSE_LEFT].pressed ) {
		yaw = -dx * rot_mod;
		pitch = -dy * rot_mod;
	}

	if ( keydata[RDFKEY_F1].pressed ) {
		load_shaders();
		load_textures();

		/*only once*/
		keydata[RDFKEY_F1].pressed = FALSE;
	}

	if ( keydata[RDFKEY_F5].pressed ) {
		debugmode += 1;
		if ( debugmode > 3 ) debugmode = 0;
		/*only once*/
		keydata[RDFKEY_F5].pressed = FALSE;
	}

	vec3_mov( dir, frame.view.dir );
	vec3_mov( right, frame.view.right );
	vec3_mov( up, def_up );

	if ( keydata[RDFKEY_MOUSE_MIDDLE].pressed ) {
		/*panning*/
		vec3_mov( tmp, up );
		vec3_scale( tmp, -dy * pan_mod * deltatime );
		vec3_subeq( offset, tmp );

		vec3_mov( tmp, right );
		vec3_scale( tmp, dx * pan_mod * deltatime );
		vec3_subeq( offset, tmp );

		if ( keydata[RDFKEY_LEFT_SHIFT].pressed ) {
			vec3_mov( frame.view.lookat, def_lookat );
		}
	}

	if ( keydata[RDFKEY_W].pressed ) {
		vec3_mov( tmp, dir );
		vec3_scale( tmp, move_step * deltatime );
		vec3_addeq( offset, tmp );
	}

	if ( keydata[RDFKEY_S].pressed ) {
		vec3_mov( tmp, dir );
		vec3_scale( tmp, -move_step * deltatime );
		vec3_addeq( offset, tmp );
	}

	if ( keydata[RDFKEY_A].pressed ) {
		vec3_mov( tmp, right );
		vec3_scale( tmp, -move_step * deltatime );
		vec3_addeq( offset, tmp );
	}

	if ( keydata[RDFKEY_D].pressed ) {
		vec3_mov( tmp, right );
		vec3_scale( tmp, move_step * deltatime );
		vec3_addeq( offset, tmp );
	}

	if ( keydata[RDFKEY_C].pressed ) {
		vec3_mov( tmp, up );
		vec3_scale( tmp, -move_step * deltatime );
		vec3_addeq( offset, tmp );
	}

	if ( keydata[RDFKEY_SPACE].pressed ) {
		vec3_mov( tmp, up );
		vec3_scale( tmp, move_step * deltatime );
		vec3_addeq( offset, tmp );
	}

	/*scroll offset*/
	if ( deltawheel != 0.f ) {
		vec3_mov( tmp, dir );
		vec3_scale( tmp, deltawheel * move_step * zoom_mod * deltatime );
		vec3_addeq( offset, tmp );
	}

	vec3_addeq( frame.view.pos, offset );

	float cpitch = frame.view.angles[_pitch_];
	float cyaw = frame.view.angles[_yaw_];

	if ( fabs( cpitch + pitch ) <= 89.f ) {
		cpitch += pitch;
	}
	else {
		pitch = 0.f;
	}

	cyaw += yaw;
	while ( cyaw <= -180.0f )
	{
		cyaw += 360.0f;
	}
	while ( cyaw > +180.0f )
	{
		cyaw -= 360.0f;
	}

	frame.view.angles[_pitch_] = cpitch;
	frame.view.angles[_yaw_] = cyaw;

	mat3_t rotx, roty;
	rot_make( roty, def_up, cyaw );

	rot_apply( frame.view.dir, roty, def_dir );
	rot_apply( frame.view.right, roty, def_right );
	vec3_normalize( frame.view.dir );

	rot_make( rotx, frame.view.right, cpitch );
	rot_apply( frame.view.dir, rotx, frame.view.dir );
	rot_apply( frame.view.up, rotx, def_up );

	vec3_add( frame.view.lookat, frame.view.pos, frame.view.dir );

	/*process game inputs*/
/*	game_input( keydata[RDFKEY_LEFT].pressed,
				keydata[RDFKEY_RIGHT].pressed,
				keydata[RDFKEY_UP].pressed,
				keydata[RDFKEY_DOWN].pressed,
				deltatime );
				*/
}

static void 
view_setup()
{
	/*vec3_mov( frame.view.pos, game_state()->packy_pos );
	vec3_mov( frame.view.lookat, game_state()->packy_pos );
	frame.view.pos[_y_] = 25.0f;
	frame.view.pos[_z_] -= 15.0f;*/

	/*frame.view.pos[_x_] = 19.5f;
	frame.view.pos[_y_] = 37.5f;
	frame.view.pos[_z_] = -5.0f;

	frame.view.lookat[_x_] = 19.5f;
	frame.view.lookat[_y_] = 0.0f;
	frame.view.lookat[_z_] = 19.5f;*/
	
	vec3_sub( frame.view.dir, frame.view.lookat, frame.view.pos );
	vec3_normalize( frame.view.dir );

	vec3_cross( frame.view.right, frame.view.dir, def_up );
	vec3_normalize( frame.view.right );

	vec3_cross( frame.view.up, frame.view.right, frame.view.dir );
	vec3_normalize( frame.view.up );
}

static void 
view_update()
{
	float deltatime = frame.time - prevframe.time;

	in_update();
	view_setup();
	
//	game_think( deltatime );

	wnd_size( &frame.width, &frame.height );
	
	/*save last frame data*/
	memcpy( &prevframe, &frame, sizeof(frame_t) );
}

static void
ubo_setup( ubo_t *ubo )
{
	glGetActiveUniformBlockiv( progs.prog, ubo->loc, GL_UNIFORM_BLOCK_DATA_SIZE, &( ubo->size ) );

	glGenBuffers( 1, &( ubo->handle ) );
	glBindBuffer( GL_UNIFORM_BUFFER, ubo->handle );
	glBufferData( GL_UNIFORM_BUFFER, ubo->size, NULL, GL_DYNAMIC_DRAW );
}

static void 
init( void )
{
	GLfloat vertices_data[] = {
		-1.0f, -1.0f, 0.0f, +1.0f,
		+1.0f, -1.0f, 0.0f, +1.0f,
		-1.0f, +1.0f, 0.0f, +1.0f,
		+1.0f, +1.0f, 0.0f, +1.0f };
	GLuint indices_data[] = { 0, 1, 2, 1, 3, 2 };

	program_set( &progs, "../shaders/frag.glsl", GL_FRAGMENT_SHADER );
	program_set( &progs, "../shaders/vert.glsl", GL_VERTEX_SHADER );

	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	load_shaders();
	load_textures();

	/*initialize view controls and game state*/
	view_init();

	/*ubos*/
	ubo_setup( &ubo_cam );

	/*vertices*/
	glGenBuffers( 1, &vertices );
	glBindBuffer( GL_ARRAY_BUFFER, vertices );
	glBufferData( GL_ARRAY_BUFFER, sizeof(vertices_data), (GLfloat*)vertices_data, GL_STATIC_DRAW );

	/*indices*/
	glGenBuffers( 1, &indices );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indices );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(indices_data), (GLuint*)indices_data, GL_STATIC_DRAW );

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
}

static void 
update( void )
{
	view_update();

	glViewport( 0, 0, frame.width, frame.height );

	glUniform1f( time_l, frame.time );
	glUniform1i( debug_l, debugmode );
	glUniform3f( resolution_l, (GLfloat)frame.width, (GLfloat)frame.height, 0.0 );

	glBindBufferBase( GL_UNIFORM_BUFFER, ubo_cam.loc, ubo_cam.handle );
	vec4_t *ptr = (vec4_t*)glMapBufferRange( GL_UNIFORM_BUFFER, 0, ubo_cam.size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT );
	vec4_mov( ptr[0], frame.view.pos );
	vec4_mov( ptr[1], frame.view.dir );
	vec4_mov( ptr[2], frame.view.right );
	vec4_mov( ptr[3], frame.view.up );
	glUnmapBuffer( GL_UNIFORM_BUFFER );

	glClear( GL_COLOR_BUFFER_BIT );

	glEnableVertexAttribArray( vp_l );
	glVertexAttribPointer( vp_l, 4, GL_FLOAT, GL_FALSE, 0, NULL );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indices );
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

	glDisableVertexAttribArray( vp_l );
}

static void 
finish( void )
{
	glUseProgram( 0 );

	program_destroy( &progs );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_UNIFORM_BUFFER, 0 );

	if ( vao ) {
		glDeleteVertexArrays( 1, &vao );
	}

	if ( vertices ) {
		glDeleteBuffers( 1, &vertices );
	}

	if ( indices )
	{
		glDeleteBuffers( 1, &indices );
	}

	if ( ubo_cam.handle )
	{
		glDeleteBuffers( 1, &ubo_cam.handle );
	}
}

static void 
mouse_scroll( const float y )
{
	frame.mouse.sy = y;
}

static void 
mouse( const float x, const float y )
{
	frame.mouse.x = x;
	frame.mouse.y = y;
}

static void 
keys( const int key, const int scancode, const int action, const int flags )
{
	if ( action == GLFW_RELEASE )	{
		keydata[key].pressed = FALSE;
	}
	else {
		keydata[key].pressed = TRUE;
	}
}

static void 
itime( const float ctime )
{
	frame.time = ctime;
}

static void 
setup_keys()
{
	keydata[RDFKEY_ESC] = (key_t){ FALSE, "ESC", "exit" };
	keydata[RDFKEY_F1] = (key_t){ FALSE, "F1", "reload shaders" };
	keydata[RDFKEY_A] = (key_t){ FALSE, "A", "strafe left" };
	keydata[RDFKEY_W] = (key_t){ FALSE, "W", "move forward" };
	keydata[RDFKEY_S] = (key_t){ FALSE, "S", "move backwards" },
	keydata[RDFKEY_D] = (key_t){ FALSE, "D", "strafe right" };
	keydata[RDFKEY_MOUSE_LEFT] = (key_t){ FALSE, "MB_LEFT", "rotate camera" };
	keydata[RDFKEY_MOUSE_RIGHT] = (key_t){ FALSE, "MB_RGHT", NULL };
	keydata[RDFKEY_MOUSE_MIDDLE] = (key_t){ FALSE, "MB_MID", "pan camera" };
	keydata[RDFKEY_LEFT_SHIFT] = (key_t){ FALSE, "LSHIFT", NULL };
	keydata[RDFKEY_C] = (key_t){ FALSE, "C", "move down" };
	keydata[RDFKEY_SPACE] = (key_t){ FALSE, "SPACE", "move up" };
	keydata[RDFKEY_F5] = (key_t){ FALSE, "F5", "debug mode" };
	keydata[RDFKEY_UNUSED] = (key_t){ FALSE, NULL, NULL };
	keydata[RDFKEY_LEFT] = (key_t){ FALSE, "LEFT", "move Packy left" };
	keydata[RDFKEY_UP] = (key_t){ FALSE, "UP", "move Packy up" };
	keydata[RDFKEY_DOWN] = (key_t){ FALSE, "DOWN", "move Packy down" };
	keydata[RDFKEY_RIGHT] = (key_t){ FALSE, "RIGHT", "move Packy right" };
}

/*****************************************************************************/
/*exports*/
void 
impl_printkeys( void )
{
	key_t	*pkey;
	for ( pkey = keydata; pkey->name; pkey++ ) {
		if ( pkey->description ) {
			fprintf( stdout, "%s\t- %s\n", pkey->name, pkey->description );
		}
	}
}

void 
impl_setup( void )
{
	set_init( init );
	set_update( update );
	set_finish( finish );
	set_keys( keys );
	set_mouse( mouse );
	set_mouse_scroll( mouse_scroll );
	set_time( itime );

	setup_keys();
}