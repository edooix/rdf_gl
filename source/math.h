#ifndef __math_h_
#define __math_h_

#include <math.h>

#define EPSILON		1e-6f

#define PI			3.141593f
#define TWO_PI		6.283186f
#define PI_DEG2RAD	0.017453f
#define PI_RAD2DEG	57.295773f

#define DEG2RAD( a ) ( ( a ) * PI_DEG2RAD )
#define RAD2DEG( a ) ( ( a ) * PI_RAD2DEG )

typedef float vec3_t[3];
typedef float vec4_t[4];
typedef float mat3_t[9];

#define	_x_ 0
#define	_y_ 1
#define	_z_ 2
#define	_w_ 3

#define _pitch_		0
#define _yaw_		1
#define _roll_		2


RDFINLINE float
vec3_dot( const vec3_t op1, const vec3_t op2 )
{
	return op1[_x_] * op2[_x_] + op1[_y_] * op2[_y_] + op1[_z_] * op2[_z_];
}

RDFINLINE void
vec3_mov( vec3_t dest, const vec3_t op )
{
	dest[_x_] = op[_x_];
	dest[_y_] = op[_y_];
	dest[_z_] = op[_z_];
}

RDFINLINE void
vec4_mov( vec4_t dest, const vec4_t op )
{
	dest[_x_] = op[_x_];
	dest[_y_] = op[_y_];
	dest[_z_] = op[_z_];
	dest[_w_] = op[_w_];
}

RDFINLINE void
vec3_cross( vec3_t dest, const vec3_t op1, const vec3_t op2 )
{
	dest[_x_] = op1[_y_] * op2[_z_] - op1[_z_] * op2[_y_];
	dest[_y_] = op1[_z_] * op2[_x_] - op1[_x_] * op2[_z_];
	dest[_z_] = op1[_x_] * op2[_y_] - op1[_y_] * op2[_x_];
}

RDFINLINE void
vec3_add( vec3_t dest, const vec3_t op1, const vec3_t op2 )
{
	dest[_x_] = op1[_x_] + op2[_x_];
	dest[_y_] = op1[_y_] + op2[_y_];
	dest[_z_] = op1[_z_] + op2[_z_];
}

RDFINLINE void
vec3_addeq( vec3_t op1, const vec3_t op2 )
{
	op1[_x_] += op2[_x_];
	op1[_y_] += op2[_y_];
	op1[_z_] += op2[_z_];
}

RDFINLINE void
vec3_sub( vec3_t dest, const vec3_t op1, const vec3_t op2 )
{
	dest[_x_] = op1[_x_] - op2[_x_];
	dest[_y_] = op1[_y_] - op2[_y_];
	dest[_z_] = op1[_z_] - op2[_z_];
}

RDFINLINE void
vec3_subeq( vec3_t op1, const vec3_t op2 )
{
	op1[_x_] -= op2[_x_];
	op1[_y_] -= op2[_y_];
	op1[_z_] -= op2[_z_];
}

RDFINLINE float
vec3_length( vec3_t v )
{
	float	length;

	length = v[_x_] * v[_x_] + v[_y_] * v[_y_] + v[_z_] * v[_z_];
	length = sqrtf( length );

	return length;
}

RDFINLINE float
vec3_normalize( vec3_t v )
{
	float length = vec3_length( v );
	float mag = 0.0f;

	if ( length ) {
		mag = 1 / length;
		v[_x_] *= mag;
		v[_y_] *= mag;
		v[_z_] *= mag;
	}

	return length;
}

RDFINLINE void
vec3_scale( vec3_t dest, const float scale )
{
	dest[_x_] = dest[_x_] * scale;
	dest[_y_] = dest[_y_] * scale;
	dest[_z_] = dest[_z_] * scale;
}

RDFINLINE void 
vec3_rotate_x( vec3_t v, const float angle )
{
	vec3_t tmp;
	float anglerad = DEG2RAD( angle );
	float cosa = cosf( anglerad );
	float sina = sinf( anglerad );

	vec3_mov( tmp, v );

	v[_x_] = tmp[_x_];
	v[_y_] = tmp[_y_] * cosa - tmp[_z_] * sina;
	v[_z_] = tmp[_y_] * sina + tmp[_z_] * cosa;
}

RDFINLINE void 
vec3_rotate_y( vec3_t v, const float angle )
{
	vec3_t tmp;
	float anglerad = DEG2RAD( angle );
	float cosa = cosf( anglerad );
	float sina = sinf( anglerad );

	vec3_mov( tmp, v );

	v[_x_] = tmp[_x_] * cosa + tmp[_z_] * sina;
	v[_y_] = tmp[_y_];
	v[_z_] = tmp[_x_] * -sina + tmp[_z_] * cosa;
}

RDFINLINE void 
vec3_rotate_z( vec3_t v, const float angle )
{
	vec3_t tmp;
	float anglerad = DEG2RAD( angle );
	float cosa = cosf( anglerad );
	float sina = sinf( anglerad );

	vec3_mov( tmp, v );

	v[_x_] = tmp[_x_] * cosa - tmp[_y_] * sina;
	v[_y_] = tmp[_x_] * sina + tmp[_y_] * cosa;
	v[_z_] = tmp[_z_];
}

RDFINLINE void 
angles2axes( const vec3_t angles, vec3_t left, vec3_t up, vec3_t dir )
{
	float pitch = DEG2RAD( angles[_pitch_] );
	float yaw = DEG2RAD( angles[_yaw_] );
	float roll = DEG2RAD( angles[_roll_] );

	float sx = sinf( pitch );
	float cx = cosf( pitch );
	float sy = sinf( yaw );
	float cy = cosf( yaw );
	float sz = sinf( roll );
	float cz = cosf( roll );

	left[_x_] = cy * cz;
	left[_y_] = sx * sy * cz + cx * sz;
	left[_z_] = -cx * sy * cz + sx * sz;

	up[_x_] = -cy * sz;
	up[_y_] = -sx * sy * sz + cx * cz;
	up[_z_] = cx * sy * sz + sx * cz;

	dir[_x_] = sy;
	dir[_y_] = -sx * cy;
	dir[_z_] = cx * cy;
}

/*axis are normalized*/
RDFINLINE void 
rot_make( mat3_t dest, const vec3_t axis, const float angle )
{
	float x = axis[0];
	float y = axis[1];
	float z = axis[2];

	float cosa = cosf( DEG2RAD( angle ) );
	float sina = sinf( DEG2RAD( angle ) );

	float one_cosa = 1.f - cosa;

	float xx = x * x;
	float yy = y * y;
	float zz = z * z;

	float xy = x * y;
	float xz = x * z;
	float yz = y * z;

	float sx = sina * x;
	float sy = sina * y;
	float sz = sina * z;

	dest[0] = xx * one_cosa + cosa;
	dest[1] = xy * one_cosa + sz;
	dest[2] = xz * one_cosa + sy;

	dest[3] = xy * one_cosa + sz;
	dest[4] = yy * one_cosa + cosa;
	dest[5] = yz * one_cosa - sx;

	dest[6] = xz * one_cosa - sy;
	dest[7] = yz * one_cosa + sx;
	dest[8] = zz * one_cosa + cosa;
}

RDFINLINE void 
rot_apply( vec3_t dest, const mat3_t rot, const vec3_t v )
{
	vec3_t tmp;
	vec3_mov( tmp, v );

	dest[0] = tmp[0] * rot[0] + tmp[1] * rot[1] + tmp[2] * rot[2];
	dest[1] = tmp[0] * rot[3] + tmp[1] * rot[4] + tmp[2] * rot[5];
	dest[2] = tmp[0] * rot[6] + tmp[1] * rot[7] + tmp[2] * rot[8];
}

RDFINLINE void 
mat3_mul( mat3_t dest, mat3_t op1, mat3_t op2 )
{
	dest[0] = op1[0] * op2[0] + op1[1] * op2[3] + op1[2] * op2[6];
	dest[1] = op1[0] * op2[1] + op1[1] * op2[4] + op1[2] * op2[7];
	dest[2] = op1[0] * op2[2] + op1[1] * op2[5] + op1[2] * op2[8];

	dest[3] = op1[3] * op2[0] + op1[4] * op2[3] + op1[5] * op2[6];
	dest[4] = op1[3] * op2[1] + op1[4] * op2[4] + op1[5] * op2[7];
	dest[5] = op1[3] * op2[2] + op1[4] * op2[5] + op1[5] * op2[8];

	dest[6] = op1[6] * op2[0] + op1[7] * op2[4] + op1[8] * op2[6];
	dest[7] = op1[6] * op2[1] + op1[7] * op2[5] + op1[8] * op2[7];
	dest[8] = op1[6] * op2[2] + op1[7] * op2[6] + op1[8] * op2[8];
}

#endif/*__math_h_*/