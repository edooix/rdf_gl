#ifndef __impl_local_h__
#define __impl_local_h__

#include "math.h"

typedef struct
{
	float x;
	float y;
	float sy;
} mouse_t;

typedef struct
{
	vec3_t	pos;
	vec3_t	lookat;

	vec3_t	dir;
	vec3_t	up;
	vec3_t	right;

	vec3_t	angles;
} view_t;

typedef struct
{
	view_t			view;
	mouse_t			mouse;

	float			time;
	unsigned int	width;
	unsigned int	height;
} frame_t;

typedef struct
{
	bool	pressed;
	char	*name;
	char	*description;
} key_t;

typedef struct
{
	GLuint	handle;
	GLuint	loc;
	GLint	size;
} ubo_t;

#endif/*__impl_local_h__*/