#ifndef __core_h_
#define __core_h_

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#define WIN32_LEAN_AND_MEAN
	#define RDFINLINE	__forceinline
#endif/*_WIN32*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

#define OGL_VER_MAJOR	4
#define OGL_VER_MINOR	3

typedef unsigned char	bool;

#define TRUE	1
#define FALSE	0

#define ERR		EXIT_FAILURE
#define OK		EXIT_SUCCESS

#define MOUSE_NONE		0
#define MOUSE_LEFT		1
#define MOUSE_RIGHT		2
#define MOUSE_MIDDLE	4

#define RDFKEY_ESC				0
#define RDFKEY_F1				1
#define RDFKEY_F5				2
#define RDFKEY_W				3
#define RDFKEY_S				4
#define RDFKEY_A				5
#define RDFKEY_D				6
#define RDFKEY_SPACE			7
#define RDFKEY_C				8
#define RDFKEY_MOUSE_LEFT		9
#define RDFKEY_MOUSE_RIGHT		10
#define RDFKEY_MOUSE_MIDDLE		11
#define RDFKEY_LEFT_SHIFT		12
#define RDFKEY_LEFT				13
#define RDFKEY_RIGHT			14
#define RDFKEY_DOWN				15
#define RDFKEY_UP				16
#define RDFKEY_UNUSED			17

#define GLFW_KEY_MOUSECLICK_LEFT	GLFW_KEY_LAST + 1
#define GLFW_KEY_MOUSECLICK_RIGHT	GLFW_KEY_LAST + 2
#define GLFW_KEY_MOUSECLICK_MIDDLE	GLFW_KEY_LAST + 3

typedef void(*RDFInitF)(void);
typedef void(*RDFTimeF)(const float);
typedef void(*RDFFinishF)(void);
typedef void(*RDFResizeF)(const unsigned int, const unsigned int);
typedef void(*RDFUpdateF)(void);
typedef void(*RDFKeysF)(const int, const int, const int, const int);
typedef void(*RDFMouseF)(const float, const float);
typedef void(*RDFMouseScrollF)(const float);

void	set_init(RDFInitF callback);
void	set_finish(RDFFinishF callback);
void	set_time(RDFTimeF callback);
void	set_update(RDFUpdateF callback);
void	set_keys(RDFKeysF callback);
void	set_mouse(RDFMouseF callback);
void	set_mouse_scroll(RDFMouseScrollF callback);

void	wnd_size(int*, int*);
int		setup_opengl(void);
int		run(void);

#endif/*__core_h_*/
