#version 430

#pragma (optimize)

layout( location = 0 ) out vec4 color;

//#define _PACKY

#define _ENABLE_DEBUG
#define _SKY
//#define _FOG
//#define _ENABLE_FIXED_CAMERA

#define MAXCELL     121

/*uniforms-------------------------------------------------------------------*/
layout( std140 ) uniform camera_block {
    vec4 pos;
    vec4 dir;
    vec4 right;
    vec4 up;
} _camera;

uniform int         _debug;
uniform float       _time;
uniform vec3        _resolution;

uniform sampler2D   _tex1;
uniform sampler2D   _tex2;
uniform sampler2D   _tex3;

uniform vec3        _packy_pos;
uniform vec3        _packy_angles;
uniform vec3        _gogu_pos;
uniform vec3        _gogu_angles;
uniform int         _bonbon_count;
uniform vec4        _bonbons[MAXCELL];
/*---------------------------------------------------------------------------*/
const float FOV         = 2.5;
const float GAMMA       = 2.2;

/*tracing*/
const float EPSILON     = 1e-3;
const float NEPSILON    = 1e-3;
const float VIEW_DIST   = 128.0;
const int   MAX_STEPS   = 256;

const float VIS_START   = EPSILON * 5;
const float VIS_SS      = 32.0;
const int   VIS_STEPS   = 64;
const int   OCC_STEPS   = 5;
const float OCC_STEPD   = 0.08;

const float PI          = 3.14159265359;

/*material idx*/
const float MAT_SKY         = - 1.0;
const float MAT_RED         =   1.0;
const float MAT_BLUE        =   2.0;
const float MAT_GREEN       =   3.0;
const float MAT_OBSIDIAN    =   4.0;
const float MAT_PEARL       =   4.5;
const float MAT_EMERALD		=   4.6;
const float MAT_ALUMINIUM   =   4.7;
const float MAT_FLESH       =   5.0;
const float MAT_GOLD        =   5.5;
const float MAT_TEX2_2D     =   7.0;
const float MAT_TEX1_3D     =   6.5;
const float MAT_TEX2_3D     =   8.5;
const float MAT_OCEAN       =   9.0;
const float MAT_MOSS        =  10.0;
const float MAT_MOSS_TEX    =  10.5;
const float MAT_FLOOR_TEX   =  11.0;

float halftime          = _time / 2.0;
float quartertime       = _time / 4.0;
float time8             = _time / 8.0;
float sinhalftime       = sin( halftime );
float coshalftime       = cos( halftime );
float sinpacky          = sin( _time * 8.0 ) * 0.5 + 0.5;

vec3  SUN               = normalize( vec3( 0.55, 0.5, -0.1 ) );
const vec3  SUN_COL     = vec3( 1.00, 1.00, 1.00 );

const vec3  SKY_COL     = vec3( 0.02, 0.22, 0.52 );
const vec3  FOG_COL     = vec3( 0.32, 0.32, 0.32 );
const vec3  HOR_COL     = vec3( 0.32, 0.32, 0.32 );

struct de_t
{
    float   dist;
    float   mat;
};

struct mat_t
{
    vec3    albedo;
    float   fr0;
    float   gloss;
};

struct point_t
{
    vec3    pos;
    vec3    nor;
    vec3    rd;
    float   mat;
    float   dist;
#ifdef _ENABLE_DEBUG    
    float   iter;
#endif    
};

de_t                bbb[MAXCELL];

/*---------------------------------------------------------------------------*/
vec2 
uv_setup( vec2 frag )
{
    vec2 uv = frag / _resolution.xy;
    float aspect = _resolution.x / _resolution.y;
 
    uv  = uv * 2.0 - 1.0;
    uv.x *= aspect;
    
    return uv; 
}

vec3 
rotate_x( vec3 v, float angle )
{
    vec3 vo = v; 
    float cosa = cos( angle ); 
    float sina = sin( angle );
    v.y = cosa*vo.y - sina*vo.z;
    v.z = sina*vo.y + cosa*vo.z;
    return v;
}

vec3 
rotate_z( vec3 v, float angle )
{
    vec3 vo = v; 
    float cosa = cos( angle ); 
    float sina = sin( angle );
    v.x = cosa*vo.x - sina*vo.y;
    v.y = sina*vo.x + cosa*vo.y;
    return v;
}

vec3 
rotate_y( vec3 v, float angle )
{
    vec3 vo = v; 
    float cosa = cos( angle ); 
    float sina = sin( angle );
    v.x = cosa*vo.x - sina*vo.z;
    v.z = sina*vo.x + cosa*vo.z;
    return v;
}

vec3
de_twist( vec3 p, float angle )
{
	float cosa = cos( angle*p.y );
	float sina = sin( angle*p.y );
	mat2 m = mat2( cosa, -sina, sina, cosa );
	return vec3( m * p.xz, p.y );
}

vec3
de_bend( vec3 p, float angle )
{
	float cosa = cos( angle*p.y );
	float sina = sin( angle*p.y );
	mat2 m = mat2( cosa, -sina, sina, cosa );
	return vec3( m * p.xy, p.z );
}

float 
length16( vec3 p )
{
	p = p*p; p = p*p; p = p*p; p = p*p;
	return pow( p.x + p.y + p.z, 1.0 / 16.0 );
}

float
length16_2( vec2 p )
{
	p = p*p; p = p*p; p = p*p; p = p*p;
	return pow( p.x + p.y, 1.0 / 16.0 );
}

float 
de_sphere( const in vec3 p, const in vec3 o, const in float r ) 
{ 
    return length( o - p ) - r; 
}

float 
de_plane( const in vec3 p, const in vec3 n, const in float h )
{
    return dot( p, n ) + h;
}

float 
de_segment( const in vec3 p, const in vec3 a, const in vec3 b, const in float r )
{
    vec3 pa = p - a, ba = b - a;
    float h = clamp(  dot( pa, ba ) / dot( ba, ba ), 0.0, 1.0  );
    return length(  pa - ba * h  ) - r;
}

float 
de_prism( const in vec3 p, const in vec2 h, const in vec3 a )
{
    vec3 q = abs( p );
    return max( q.z - h.y, max( q.x * a.x + p.y * a.y, -p.y * a.z ) - h.x );
}

float 
de_box( const in vec3 p, const in vec3 o, const in vec3 dim )
{
    vec3 d = abs( o-p ) - dim;
    return min( max( d.x,max( d.y,d.z ) ),0.0 ) + length( max( d, 0.0 ) );
}

float 
de_cross( vec3 p, float w) 
{
    p = abs(p);
    vec3 d = vec3(max(p.x, p.y),
                  max(p.y, p.z),
                  max(p.z, p.x));
    return min(d.x, min(d.y, d.z)) - w;
}

float 
de_rbox( const in vec3 p, const in vec3 o, const in vec3 dim, const in float bev )
{
    vec3 d = abs( o - p ) - dim;
    return length( max( d, 0.0 ) ) - bev;
}

float 
de_rbox2( const in vec3 p, const in vec3 o, const in vec3 dim )
{
    vec3 d = abs( o - p ) - dim;
    return length( max( d, 0.0 ) ) - 0.15;
}

float 
de_torus( const in vec3 p, const in vec3 o, const in vec2 radii ) 
{ 
    vec3 q = o - p;
    vec2 l = vec2( length( q.xz ) - radii.x, q.y );
    return length( l ) - radii.y;
}

float
de_torus16( const in vec3 p, const in vec3 o, const in vec2 radii )
{
	vec3 q = o - p;
	vec2 l = vec2( length( q.xz ) - radii.x, q.y );
	return length16_2( l ) - radii.y;
}

float 
de_cylinder( const in vec3 p, const in vec2 h )
{
    vec2 d = abs( vec2( length( p.xz ), p.y ) ) - h;
    return min( max( d.x, d.y), 0.0 ) + length( max( d, 0.0 ) );
}

float 
de_cone( const in vec3 p, const in vec3 c )
{
    vec2 q = vec2( length( p.xz ), p.y );
    return max( max( dot( q, c.xy ), p.y ), -p.y-c.z );
}

float 
smin( float a, float b, float k )
{
    float h = clamp( 0.5 + 0.5 * ( b-a ) / k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*( 1.0-h );
}

de_t 
de_union( de_t de1, de_t de2 )
{
    return ( de1.dist < de2.dist ) ? de1 : de2;
}

de_t 
de_intersect( de_t de1, de_t de2 )
{
    return ( de1.dist < de2.dist ) ? de2 : de1;
}

de_t 
de_carve( de_t de1, de_t de2 )
{
    de_t det = de1;
    det.dist = -de1.dist;
    return ( det.dist > de2.dist ) ? det : de2;
}

/*procedurals----------------------------------------------------------------*/
float 
hash( float n )
{ 
    return fract( sin( n ) * 43758.5453123 ); 
}

float 
noise2d( const in vec2 x )
{
    vec2 p = floor( x );
    vec2 f = fract( x );
    f = f * f * ( 3.0 - 2.0 * f );
    float n = p.x + p.y * 57.0;
    return mix( mix( hash( n +  0.0 ), hash( n +  1.0), f.x ),
                mix( hash( n + 57.0 ), hash( n + 58.0), f.x ), f.y);
}

float 
noise3d( const in vec3 x )
{
    vec3 p = floor( x );
    vec3 f = fract( x );
    f = f * f * ( 3.0 - 2.0 * f );
    float n = p.x + p.y * 157.0 + 113.0 * p.z;
    return mix( mix( mix( hash( n+  0.0 ),  hash( n+  1.0 ),f.x ),
                mix( hash(      n+157.0 ),  hash( n+158.0 ),f.x ),f.y ),
                mix( mix( hash( n+113.0 ),  hash( n+114.0 ),f.x ),
                mix( hash(      n+270.0 ),  hash( n+271.0 ),f.x ),f.y ),f.z );
}

float 
fract_noise2d( in vec2 xy )
{
    float w = 0.85;
    float f = 0.0;

    for ( int i = 0; i < 4; i++ ) {
        f += noise2d( xy ) * w;
        w = w * 0.2;
        xy = 8.0 * xy;
    }

    return f;
}

/*---------------------------------------------------------------------------*/
vec3 
texture3D( sampler2D tex, vec3 p, vec3 n, float scale )
{
    vec3 x = texture2D( tex, p.yz * scale ).xyz;
    vec3 y = texture2D( tex, p.zx * scale ).xyz;
    vec3 z = texture2D( tex, p.xy * scale ).xyz;

    return x*abs( n.x ) + y*abs( n.y ) + z*abs( n.z );  
}

/*---------------------------------------------------------------------------*/
de_t 
packy( const in vec3 p )
{
    de_t de;

    de.mat = MAT_GREEN;
    de.dist = length( p ) - 1.0;

    vec3 q = p - vec3( 1.25 + sinpacky, -0.55, 0.0 );
    q = rotate_z( q, 0.87 );

    de_t mouth = de_t(  de_prism( q, vec2( 0.5, 1.25 ), vec3( 0.50, 0.25, 1.0 ) ), 
                        MAT_FLESH );
    de = de_carve( mouth, de);

    de_t eye = de_t(    de_sphere( p, vec3( 0.65, 0.65, 0.35 ), 0.08 ), 
                        MAT_OBSIDIAN );
    de = de_union( eye, de);

    eye = de_t(    de_sphere( p, vec3( 0.65, 0.65,-0.35 ), 0.08 ), 
                   MAT_OBSIDIAN );
    de = de_union( eye, de);

    /* left arm */
    float arm_1 = de_segment( p, vec3( 0.0,0.15,0.95 ), vec3( 0.05,0.0,1.25 ), 0.05 );
    float arm_2 = de_segment( p, vec3( 0.05,0.0,1.25 ), vec3( 0.25,0.3,1.45 ), 0.05 );

    de_t arm = de_t(    smin( arm_1, arm_2, 0.05 ),
                        MAT_GREEN );
    de = de_union( arm, de);

    /* right arm */
    arm_1 = de_segment( p, vec3( 0.0,0.15,-0.95 ), vec3( 0.05,0.0,-1.25 ), 0.05 );
    arm_2 = de_segment( p, vec3( 0.05,0.0,-1.25 ), vec3( 0.25,-0.3,-1.45 ), 0.05 );

    arm = de_t(    smin( arm_1, arm_2, 0.05 ),
                   MAT_GREEN );
    de = de_union( arm, de);

    return de;
}

de_t 
facult( const in vec3 p )
{
    de_t de;    
    de.dist = de_box( p, vec3( 0.0, -0.75, 0.0 ), vec3( 0.05, 0.02, 0.40 ) );
    de.mat = MAT_OBSIDIAN;

    float d1 = de_box( p, vec3( 0.0, -0.71, 0.0 ), vec3( 0.05, 0.02, 0.38 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_ALUMINIUM;
    }

    d1 = de_box( p, vec3( 0.0, -0.67, 0.0 ), vec3( 0.05, 0.02, 0.36 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_OBSIDIAN;
    }

    d1 = de_box( p, vec3( 0.0, -0.63, 0.0 ), vec3( 0.05, 0.04, 0.32 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_PEARL;
    }

    d1 = de_box( p, vec3( 0.0, -0.40, 0.26 ), vec3( 0.05, 0.20, 0.05 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_ALUMINIUM;
    }

    d1 = de_box( p, vec3( 0.0, -0.40, 0.13 ), vec3( 0.05, 0.20, 0.05 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_ALUMINIUM;
    }

    d1 = de_box( p, vec3( 0.0, -0.40, 0.00 ), vec3( 0.05, 0.20, 0.05 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_ALUMINIUM;
    }

    d1 = de_box( p, vec3( 0.0, -0.40, -0.13 ), vec3( 0.05, 0.20, 0.05 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_ALUMINIUM;
    }

    d1 = de_box( p, vec3( 0.0, -0.40, -0.26 ), vec3( 0.05, 0.20, 0.05 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_ALUMINIUM;
    }

    d1 = de_box( p, vec3( 0.0, -0.16, 0.0 ), vec3( 0.05, 0.04, 0.32 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_OBSIDIAN;
    }    

    vec3 q = rotate_y( p, 1.57079633 );
    q -= vec3( 0.0, -0.10, 0.0 );
    d1 = de_prism( q, vec2( 0.05, 0.05 ), vec3( 0.14, 0.4, 2.5 ) );

    if( d1 < de.dist ) {
        de.dist = d1;
        de.mat = MAT_OBSIDIAN;
    }   

    return de;
}

float
cluster( const in vec3 p )
{
    vec3 offset = vec3( -0.15,0.0,0.0 );
    float s = length( p - offset ) - 0.15;
    float res = s;

    offset = vec3( 0.15,0.0,0.0 );
    s = length( p - offset ) - 0.15;
    res = smin( res, s, 0.05 );

    offset = vec3( 0.0,0.15,0.0 );
    s = length( p - offset ) - 0.15;
    res = smin( res, s, 0.05 );

    offset = vec3( 0.0,-0.15,0.0 );
    s = length( p - offset ) - 0.15;
    res = smin( res, s, 0.05 );

    offset = vec3( 0.0,0.0,0.15 );
    s = length( p - offset ) - 0.15;
    res = smin( res, s, 0.05 );

    offset = vec3( 0.0,0.0,-0.15 );
    s = length( p - offset ) - 0.15;
    res = smin( res, s, 0.05 );  

    return res;  
}

de_t 
gogu( const in vec3 p )
{
    de_t de;

    de.mat = MAT_FLESH;

    float sina = sinhalftime;
    float cosa = coshalftime;
      
    float bump = 0.035*sin( 8.0*_time )*sin( 2.0*p.y )*sin( 16.0*p.z );
    de.dist = length( p ) - 1.15 + bump;

    float reye = length( p-vec3( 0.95,0.35,0.25 ) ) - 0.15;

    if( reye < de.dist ) {
        de.dist = reye;
        de.mat = MAT_PEARL;
    }

    reye = length( p-vec3( 1.05,0.38,0.25 ) ) - 0.05;

    if( reye < de.dist ) {
        de.dist = reye;
        de.mat = MAT_BLUE;
    }

    float leye = length( p-vec3( 0.95,0.35,-0.25 ) ) - 0.15;

    if( leye < de.dist ) {
        de.dist = leye;
        de.mat = MAT_PEARL;
    }

    leye = length( p-vec3( 1.05,0.38,-0.25 ) ) - 0.05;

    if( leye < de.dist ) {
        de.dist = leye;
        de.mat = MAT_BLUE;
    }

    de.dist = smin( de.dist, de_sphere( p, vec3( 0.0, 0.25, 0.0 ), 1.15 ), 0.05 );

    return de;
}

float 
de_wavyfloor( const in vec3 p, const in float height )
{
    float mod = sin( ( p.x*0.3 + _time ) * 0.75 ) + sin( ( p.z*0.63 + _time * 2 ) * 0.07 ) + 0.1;
    return p.y - height + mod;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/*FRACTALS*/

float fract3d_menger( vec3 z )
{
    vec3    Offset = vec3( 10.0, 10.0, 10.0 );
    int     Iterations = 16;
    float   Scale = 3.0;

    int n = 0;
    while ( n < Iterations ) {
        z = abs( z );
        if ( z.x<z.y ){ z.xy = z.yx;}
        if ( z.x< z.z ){ z.xz = z.zx;}
        if ( z.y<z.z ){ z.yz = z.zy;}
        z = Scale*z-Offset*( Scale-1.0 );
        if(  z.z<-0.5*Offset.z*( Scale-1.0 ) )  z.z+=Offset.z*( Scale-1.0 );
        n++;
    }
    
    return abs( length( z )-0.0  ) * pow( Scale, float( -n ) );
}

float fract3d_weird( vec3 p )
{
    vec3    CSize = vec3( .808, .8, 1.137 );
    float   scale = 2.0;
    float   add = sin( _time )*.2+.1;

    for(  int i=0; i < 8;i++  )
    {
        p = 2.0*clamp( p, -CSize, CSize ) - p;
        float r2 = dot( p,p );
        float k = max( ( 1.1 )/( r2 ), 1.0 );
        p     *= k;
        scale *= k;
    }
    float l = length( p.xz );
    float rxy = l - 4.0;
    float n = l * p.y;
    rxy = max( rxy, -( n ) / ( length( p ) )-.07+sin( _time*2.0+p.x+p.y+23.5*p.z )*.02 );
    return ( rxy ) / abs( scale );    
}

float stime, ctime;
 void ry( inout vec3 p, float a ){  
    float c,s;vec3 q=p;  
    c = cos( a ); s = sin( a );  
    p.x = c * q.x + s * q.z;  
    p.z = -s * q.x + c * q.z; 
 }  

/* 

z = r*( sin( theta )cos( phi ) + i cos( theta ) + j sin( theta )sin( phi )

zn+1 = zn^8 +c

z^8 = r^8 * ( sin( 8*theta )*cos( 8*phi ) + i cos( 8*theta ) + j sin( 8*theta )*sin( 8*theta )

zn+1' = 8 * zn^7 * zn' + 1

*/

vec3 mb( vec3 p ) {
    p.xyz = p.xzy;
    vec3 z = p;
    vec3 dz=vec3( 0.0 );
    float power = 8.0;
    float r, theta, phi;
    float dr = 1.0;
    
    float t0 = 1.0;
    
    for( int i = 0; i < 7; ++i ) {
        r = length( z );
        if( r > 2.0 ) break;
        theta = atan( z.y / z.x );
        phi = asin( z.z / r );
        
        dr = pow( r, power - 1.0 ) * dr * power + 1.0;
    
        r = pow( r, power );
        theta = theta * power;
        phi = phi * power;
        
        z = r * vec3( cos( theta )*cos( phi ), sin( theta )*cos( phi ), sin( phi ) ) + p;
        
        t0 = min( t0, r );
    }
    return vec3( 0.5 * log( r ) * r / dr, t0, 0.0 );
}

float fract3d_mandelbulb( vec3 p ){ 
    stime=sin( _time*0.2 ); 
    ctime=cos( _time*0.2 );
    ry( p, stime );
    return mb( p ).x; 
} 

float fract3d_qjulia( vec3 pos ) {
    vec4 C = vec4( 0.10, 0.63, -0.03, -0.06 );
    vec4 p = vec4( pos, 0.0 );
    vec4 dp = vec4( 1.0, 0.0,0.0,0.0 );
    for ( int i = 0; i < 8; i++ ) {
        dp = 2.0* vec4( p.x*dp.x-dot( p.yzw, dp.yzw ), p.x*dp.yzw+dp.x*p.yzw+cross( p.yzw, dp.yzw ) );
        p = vec4( p.x*p.x-dot( p.yzw, p.yzw ), vec3( 2.0*p.x*p.yzw ) ) + C;
        float p2 = dot( p,p );
        if ( p2 > 100 ) break;
    }
    float r = length( p );
    return  0.5 * r * log( r ) / length( dp );
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


#ifndef _PACKY
de_t
scene( const in vec3 p )
{
    de_t de, de2, de3;

    /*de.mat  = MAT_OCEAN;
    de.dist = de_wavyfloor( p, -1.0 );*/

    de.dist = de_plane( p, vec3( 0.0, 1.0, 0.0 ), 1.0  );
    de.mat = MAT_ALUMINIUM;

    vec3 q = rotate_y( p - vec3( 15.0, 3.0, 15.0), _time ) / 5.0;
    de2 = facult( q );
    de2.dist *= 5.0;

    de = de_union( de, de2 );

    float bb = 0.0;

    bb = de_sphere( p, vec3( 10.0, 0.0, 15.0), 1.5 );
  
    if( bb < de.dist ) {
        vec3 q = rotate_y( p - vec3( 10.0, 0.0, 15.0), quartertime );
        de_t obj_packy = packy( q );
        de = de_union( obj_packy, de );
    }

    bb = de_sphere( p, vec3( 10.0, 0.0, 20.0), 1.5 );

    if( bb < de.dist ) {
        vec3 q = rotate_y( p - vec3( 10.0, 0.0, 20.0), -halftime );
        de_t obj_gogu = gogu( q );
        de = de_union( obj_gogu, de );
    }   
 
/*    de2.dist = de_box( p, vec3( 15.0, 0.5, 20.0 ), vec3( 1.5 ) );
    de2.mat = MAT_FLESH;

    de = de_union( de, de2 );

    de2.dist = de_rbox( p, vec3( 20.0, 2.0, 20.0 ), vec3( 0.75, 2.75, 0.75 ), 0.25 );
    de2.mat = MAT_BLUE;

    de = de_union( de, de2 );

    q = rotate_z( p - vec3( 20.0, 0.0, 15.0), _time );
    q = rotate_x( q, _time );
    de2.dist = de_torus( q, vec3( 0.0 ), vec2( 1.0, 0.25 ) );
    de2.mat = MAT_GREEN;

    de = de_union( de, de2 );

    de2.dist = de_cylinder( p - vec3( 15.0, 0.0, 10.0), vec2( 1.0, 1.0 ) ) + noise3d( p * 5.5 ) * 0.18;
    de2.mat = MAT_EMERALD;

    de = de_union( de, de2 );
*/
    
 	//float desert = de_plane( p, vec3( 0.0, 1.0, 0.0 ), 1.0  ) + bump_noise;
    //de.mat  = MAT_GOLD;
    //de.dist = desert;

    //float bump = 0.10*sin( 10.0*p.x )*sin( 8.0*_time )*sin( 10.0*p.z );
    //float disco = de_sphere( p, vec3( 0.0, sin( _time )*5.0, 0.0 ), 5.25 ) + bump;
    //de.mat  = MAT_GREEN;
    //de.dist = disco;

    //float bump = 0.10*sin( 10.0*p.x )*sin( 8.0*_time )*sin( 10.0*p.z );
    //vec3 q = p;
    //q.x = mod( q.x, 16.0) - 8.0;
    //q.y = mod( q.y, 16.0) - 8.0;
    //q.z = mod( q.z, 16.0) - 8.0;
    //float disco = de_sphere( q, vec3(0.0), 1.0 ) + bump;
    //de.mat  = MAT_GREEN;
    //de.dist = disco;    

    //float fractal = fract3d_menger( p );
    //de2.mat  = MAT_TEX1_3D;
    //de2.dist = fractal;    

    //float fractal = fract3d_weird( p ); 
    //de.mat  = MAT_FLESH;
    //de.dist = fractal;    

    //float fractal = fract3d_mandelbulb( p ); 
	//de2.mat = MAT_OCEAN;
    //de2.dist = fractal;    
	//de = de_union( de, de2 );

    //float fractal = fract3d_qjulia( rotate_y( p, _time/4 ) );
    //de.mat  = MAT_GOLD;
    //de.dist = fractal;  

    return de;    
}

#else

float
moss_tile( const in vec3 p, const in vec3 o, const in vec3 dim )
{
    float de;
    float z = length( o - p );
    float bump = 0.0;

    if( z < length( dim ) ) {
        vec3 n = normalize( o - p );
        bump = texture3D( _tex3, p, n, 0.005 ).r * 0.55;
    }

    de = de_box( p, o, dim ) + bump;

    return de;
}

de_t
scene( const in vec3 p )
{
    de_t de = de_t( de_box( p, vec3( 19.5, -1.5, 19.5 ), vec3( 19.5, 0.5, 19.5 ) ),
                    MAT_FLOOR_TEX );

    de_t barrier_north = de_t(      de_box( p, vec3( 19.5, -0.5, 37.5 ), vec3( 19.5, 1.05, 1.5 ) ),
                                    MAT_TEX1_3D );
    de_t barrier_south = de_t(      de_box( p, vec3( 19.5, -0.5, 1.5 ), vec3( 19.5, 1.05, 1.5 ) ),
                                    MAT_TEX1_3D );
    de_t barrier_east = de_t(       de_box( p, vec3( 1.5, -0.5, 19.5 ), vec3( 1.5, 1.05, 19.5 ) ),
                                    MAT_TEX1_3D );
    de_t barrier_west = de_t(       de_box( p, vec3( 37.5, -0.5, 19.5 ), vec3( 1.5, 1.05, 19.5 ) ),
                                    MAT_TEX1_3D ); 

    de_t elem_a = de_t(       de_rbox2( p, vec3( 33.0, -0.5, 15.0 ), vec3( 3.0, 1.05, 3.0 ) ),
                              MAT_MOSS_TEX );
    de_t elem_b = de_t(       de_rbox2( p, vec3( 31.5, -0.5, 27.0 ), vec3( 1.5, 1.05, 6.0 ) ),
                              MAT_MOSS_TEX );    
    de_t elem_c = de_t(       de_rbox2( p, vec3( 30.0, -0.5, 7.5 ), vec3( 3.0, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );   
    de_t elem_d = de_t(       de_rbox2( p, vec3( 30.0, -0.5, 31.5 ), vec3( 3.0, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );    
    de_t elem_e = de_t(       de_rbox2( p, vec3( 25.5, -0.5, 24.0 ), vec3( 1.5, 1.05, 3.0 ) ),
                              MAT_MOSS_TEX );                                                              
    de_t elem_f = de_t(       de_rbox2( p, vec3( 25.5, -0.5, 15.0 ), vec3( 1.5, 1.05, 3.0 ) ),
                              MAT_MOSS_TEX );      
    de_t elem_g = de_t(       de_rbox2( p, vec3( 19.5, -0.5, 31.5 ), vec3( 4.5, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );                                 
    de_t elem_h = de_t(       de_rbox2( p, vec3( 22.5, -0.5, 25.5 ), vec3( 1.5, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );   
    de_t elem_i = de_t(       de_rbox2( p, vec3( 22.5, -0.5, 7.5 ), vec3( 1.5, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );   
    de_t elem_j = de_t(       de_rbox2( p, vec3( 19.5, -0.5, 13.5 ), vec3( 1.5, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );       
    de_t elem_k = de_t(       de_rbox2( p, vec3( 19.5, -0.5, 19.5 ), vec3( 1.5, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );       
    de_t elem_l = de_t(       de_rbox2( p, vec3( 16.5, -0.5, 27.0 ), vec3( 1.5, 1.05, 3.0 ) ),
                              MAT_MOSS_TEX );          
    de_t elem_m = de_t(       de_rbox2( p, vec3( 16.5, -0.5, 13.5 ), vec3( 1.5, 1.05, 7.5 ) ),
                              MAT_MOSS_TEX );              
    de_t elem_n = de_t(       de_rbox2( p, vec3( 7.5, -0.5, 28.5 ), vec3( 4.5, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );   
    de_t elem_o = de_t(       de_rbox2( p, vec3( 7.5, -0.5, 13.5 ), vec3( 4.5, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );  
    de_t elem_p = de_t(       de_rbox2( p, vec3( 4.5, -0.5, 7.5 ), vec3( 1.5, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );                                                                                               
    de_t elem_r = de_t(       de_rbox2( p, vec3( 10.5, -0.5, 31.5 ), vec3( 1.5, 1.05, 1.5 ) ),
                              MAT_MOSS_TEX );
    de_t elem_s = de_t(       de_rbox2( p, vec3( 10.5, -0.5, 6.0 ), vec3( 1.5, 1.05, 3.0 ) ),
                              MAT_MOSS_TEX );

    de_t house = de_t(        de_box( p, vec3( 7.5, -0.5, 21.0 ), vec3( 4.5, 3.05, 3.0 ) ),
                              MAT_TEX1_3D );    
    de_t house_exit = de_t(        de_box( p, vec3( 7.5, 0.65, 17.5 ), vec3( 1.5, 1.5, 1.5 ) ),
                                   MAT_OBSIDIAN );        

    de = de_union( barrier_north, de );
    de = de_union( barrier_south, de );
    de = de_union( barrier_east, de );
    de = de_union( barrier_west, de );

    de = de_union( elem_a, de );
    de = de_union( elem_b, de );
    de = de_union( elem_c, de );
    de = de_union( elem_d, de );
    de = de_union( elem_e, de );
    de = de_union( elem_f, de );
    de = de_union( elem_g, de );
    de = de_union( elem_h, de );
    de = de_union( elem_i, de );
    de = de_union( elem_j, de );
    de = de_union( elem_k, de );
    de = de_union( elem_l, de );
    de = de_union( elem_m, de );
    de = de_union( elem_n, de );
    de = de_union( elem_o, de );
    de = de_union( elem_p, de );
    de = de_union( elem_r, de );
    de = de_union( elem_s, de );

    de = de_union( house, de );
    de = de_carve( house_exit, de );

    de_t moss;    

    float bb = 0.0;

    bb = de_sphere( p, _packy_pos, 1.5 );
  
    if( bb < de.dist ) {
        vec3 q = rotate_y( p - _packy_pos, _packy_angles.y );
        de_t obj_packy = packy( q );
        de = de_union( obj_packy, de );
    }

    bb = de_sphere( p, _gogu_pos, 1.5 );

    if( bb < de.dist ) {
        vec3 q = rotate_y( p - _gogu_pos, _gogu_angles.y );
        de_t obj_gogu = gogu( q );
        de = de_union( obj_gogu, de );
    }

    /*vec3 q = p;
    q.x = mod( q.x, 3.0 ) - 1.5;
    q.z = mod( q.z, 3.0 ) - 1.5;
    de_t pebble = de_t( cluster( q ),
                        MAT_GOLD );

    bb = de_box( p, vec3( 19.5, -0.5, 19.5 ), vec3( 19.5, 1.5, 19.5 ) );
    if( pebble.dist > bb) {
        de = de_union( pebble, de );
    }*/

    return de;
}

#endif

/*---------------------------------------------------------------------------*/
vec3 
normal( const in vec3 p )
{
    vec3 offset = vec3( NEPSILON, 0.0, 0.0 );
    vec3 n;

    n.x = scene( p+offset.xyy ).dist - scene( p-offset.xyy ).dist;
    n.y = scene( p+offset.yxy ).dist - scene( p-offset.yxy ).dist;
    n.z = scene( p+offset.yyx ).dist - scene( p-offset.yyx ).dist;

    return normalize( n );
}

/*---------------------------------------------------------------------------*/
point_t 
trace( const in vec3 ro, const in vec3 rd, const in float maxd )
{
    point_t px;

    int     i = 0;
    de_t    de = de_t( 0.0, MAT_SKY );

    px.mat = MAT_SKY;
    px.dist = EPSILON * 2.0;

    for( i=0; i<MAX_STEPS; i++ ) {
        px.pos = ro + rd * px.dist;
        de = scene( px.pos );
        px.mat = de.mat;
        if( ( abs( de.dist ) < EPSILON ) || ( px.dist > maxd ) || ( i > MAX_STEPS ) ) {
            break;
        }        
        px.dist += de.dist;
    }

    if( px.dist > maxd ) {
        px.mat = MAT_SKY;
        px.dist = maxd;
    }

#ifdef _ENABLE_DEBUG    
    px.iter = float( i )/float( MAX_STEPS );
#endif

    return px;
}

/*---------------------------------------------------------------------------*/
mat_t 
object( const in vec3 p, const in vec3 n, const in float matid )
{
    mat_t mat;

    if( matid == MAT_GREEN ) {
        mat.albedo = vec3( 0.196078, 0.8, 0.196078 );
        mat.gloss = 0.0;
        mat.fr0 = 0.0;
    }

    if( matid == MAT_OBSIDIAN ) {
        mat.albedo = vec3( 0.05, 0.05, 0.05 );
        mat.gloss = 10.5;
        mat.fr0 = 0.0;
    }

    if( matid == MAT_ALUMINIUM ) {
        mat.albedo = vec3( 0.329412, 0.329412, 0.329412 );
        mat.gloss = 1.5;
        mat.fr0 = 1.0;
    }    

    if( matid == MAT_PEARL ) {
        mat.albedo = vec3( 0.95, 0.95, 0.95 );
        mat.gloss = 1.5;
        mat.fr0 = 0.0;
    }

    if( matid == MAT_GOLD ) {
        mat.albedo = vec3( 0.85, 0.45, 0.0 );
        mat.gloss = 5.0;
        mat.fr0 = 0.3;
    }

    if( matid == MAT_RED ) {
        mat.albedo = vec3( 0.9, 0.2, 0.2 );
        mat.gloss = 0.0;
        mat.fr0 = 0.0;
    }    

    if( matid == MAT_BLUE ) {
        mat.albedo = vec3( 0.05, 0.05, 0.95 );
        mat.gloss = 10.5;
        mat.fr0 = 0.0;
    }    

    if( matid == MAT_FLESH ) {
        mat.albedo = vec3( 0.9, 0.1, 0.1 );
        mat.gloss = 1.0;
        mat.fr0 = 0.5;
    }    

    if( matid == MAT_TEX2_2D ){
        mat.albedo = texture2D( _tex2, p.xz * 0.10 ).xyz;
        mat.gloss = 0.0;
        mat.fr0 = 0.0;        
    }    

    if( matid == MAT_TEX1_3D ){
        mat.albedo = texture3D( _tex1, p, n, 0.45 );
        mat.gloss = 0.0;
        mat.fr0 = 0.0;        
    }

    if( matid == MAT_TEX2_3D ){
        mat.albedo = texture3D( _tex2, p, n, 0.2 );
        mat.gloss = 0.0;
        mat.fr0 = 0.0;        
    }

    if( matid == MAT_OCEAN ) {
        mat.albedo = vec3( 0.0, 0.05, 0.05 );
        mat.gloss = 1.0;
        mat.fr0 = 0.2;
    } 

	if ( matid == MAT_EMERALD ) {
		mat.albedo = vec3( 0.07568, 0.61424, 0.07568 );
		mat.gloss = 0.6;
		mat.fr0 = 0.2;
	}

    if( matid == MAT_MOSS ) {
        mat.albedo = vec3( 0.15, 0.35, 0.15 );
        mat.gloss = 0.025;
        mat.fr0 = 0.0;
    }     

    if( matid == MAT_MOSS_TEX ) {
        mat.albedo = texture3D( _tex3, p, n, 0.2 );
        mat.gloss = 0.025;
        mat.fr0 = 0.0;
    }

    if( matid == MAT_FLOOR_TEX ){
        mat.albedo = texture2D( _tex2, p.xz * 0.2 ).xyz;
        mat.gloss = 0.0;
        mat.fr0 = 0.0;        
    }    

    return mat;
}

/*---------------------------------------------------------------------------*/
vec3 
background( const in vec3 p, const in vec3 rd )
{
#ifdef _SKY
    float sun = max( dot( rd, SUN ), 0.0 );
    float v = 1.0 - max( rd.y, 0.0 );
    vec3  sky = mix( SKY_COL, HOR_COL, v );
    //sky = sky + SUN_COL * sun * sun * 0.25;
    sky = sky + SUN_COL * min( pow( sun, 512.0 ) * 1.5, 1.0 );
   
    return clamp(sky, 0.0, 1.0);

#else    
    vec3 sun = SUN_COL * pow( clamp( dot( rd, SUN ), 0.0, 1.0 ), 128.0 );
    vec3 sky = vec3( 0.5 + 0.2*rd.y );    

    return sky + sun;
#endif    
}

vec3 
fog( const in vec3 rgb, const in float dist )
{
    float density = exp( - dist * dist * 0.00015 );
    return mix( FOG_COL, rgb, density );
}

/*---------------------------------------------------------------------------*/
float 
vis( const in vec3 ro, const in vec3 rd, const in float maxd )
{
    de_t    de = de_t( 0.0, MAT_SKY );
    float   d = VIS_START;
    float   visf = 1.0f;

    for( int i=0; i<VIS_STEPS; i++ ) {
        if( d < maxd ) {
            vec3 p = ro + rd * d;
            de = scene( p );
            //if(de.dist < EPSILON) return 0.0;
            visf = min( visf, VIS_SS * de.dist/d );
            d += de.dist;
        }
    }

    return clamp(  visf, 0.0, 1.0  );
}

/*---------------------------------------------------------------------------*/
float 
occ( const in vec3 ro, const in vec3 rd )
{
    float   occf = 1.0;
    float   occt = 0.0;
    de_t    de = de_t( 0.0, MAT_SKY );
    float   d = 0.0;
    
    for( int i=0; i<OCC_STEPS; i++ ) {
        d = float( i ) * 0.05 + EPSILON;
        vec3 p = ro + rd * d;
        de = scene( p );
        occt += -( de.dist - d ) * occf;
        occf *= 0.75;
    }

    return clamp( 1.0 - OCC_STEPS*occt, 0.0, 1.0 );
}

/*---------------------------------------------------------------------------*/
float 
lambert_diffuse( const in vec3 n, const in vec3 l ) 
{    
    return max( 0.0, dot( n,l ) );
}

float 
blinnphong_spec( const in vec3 rd, const in vec3 n, const in vec3 l, const in float gloss )
{          
    vec3 h = normalize( l - rd );
    float n_dot_h = max( 0.0, dot( h, n ) );

    float spow = exp2( 4.0 + 6.0 * gloss );
    float spe = ( spow + 2.0 ) * 0.125;

    return pow( n_dot_h, spow ) * spe;
}

/*---------------------------------------------------------------------------*/
vec3 
shade( const in point_t px, const in mat_t mat )
{
    vec3 p = px.pos;
    vec3 n = px.nor;
    vec3 rd = px.rd;

    vec3 albedo = mat.albedo;
    float ks = mat.gloss;
    float kfr = mat.fr0;
    
	float ao = occ( px.pos, px.nor );
    float amb = clamp( 0.5+0.5*n.y, 0.0, 1.0 );
    float dif = lambert_diffuse( n, SUN );
	float sh = vis( px.pos, SUN, VIEW_DIST );
    
    vec3 brdf = vec3( 0.0 );
    brdf += 0.20*amb*HOR_COL*ao;
    brdf += 0.50*dif*sh*SUN_COL;
	brdf += ao*0.30;

    float pp = clamp( dot( reflect( rd,n ), SUN ), 0.0, 1.0  );
	float spe = sh*pow( pp, 64.0 );
	float fre = ao*pow( clamp( 1.0 + dot( n, rd ), 0.0, 1.0 ), 2.0 );

    return albedo*brdf + ks*albedo*spe + kfr*fre*( 0.5+0.5*albedo );
}

/*---------------------------------------------------------------------------*/
/*linear tonemapping*/
vec3 
postprocess( in vec3 rgb )
{
    float exposure = 1.0;
    rgb = clamp( exposure * rgb, 0.0, 1.0 );
    rgb = pow( rgb, vec3( 1.0 / GAMMA ) );
    return rgb;
}

/*---------------------------------------------------------------------------*/
vec3 
render( const in vec3 ro, const in vec3 rd )
{
    point_t px;
    mat_t   mat;
    vec3    rgb = vec3( 1.0 );
    
    px = trace( ro, rd, VIEW_DIST );

#ifdef _ENABLE_DEBUG
    if( _debug == 1 ) {
        return vec3( px.iter, 0.0, 0.0 );
    }

    if( _debug == 3 ) {
        float ndist = clamp( px.dist / VIEW_DIST, 0.0, 1.0 );
        return vec3( ndist );
    }
#endif

    if( px.mat < 0.0 ) {
        return background( px.pos, rd );
    }

    px.nor = normal( px.pos );
    px.rd  = rd;
    
#ifdef _ENABLE_DEBUG
    if( _debug == 2 ) {
        return px.nor;
    }
#endif

    mat = object( px.pos, px.nor, px.mat );
    rgb = shade( px, mat );

#ifdef _FOG    
    rgb = fog( rgb, px.dist );
#endif  

    return rgb;
}

/*---------------------------------------------------------------------------*/
void 
main( void )
{  
    color = vec4( 0.0 );

    vec3 rgb    = vec3( 0.0 );
    vec3 ro     = vec3( 0.0 );
    vec3 rd     = vec3( 0.0 );  
    vec2 uv     = uv_setup( gl_FragCoord.xy );

#ifdef _ENABLE_FIXED_CAMERA    
    ro = vec3( 0.0, 0.0, -5.0 );
    //ro = vec3( cos( _time/2 )*2.0, 2.0, sin( _time/2 )*2.0 );

    vec3 rt = vec3( 0.0 );
    vec3 up = vec3( 0.0, 1.0, 0.0 );

    rd = normalize( rt - ro );
    vec3 ri = normalize( cross( rd, up ) );
    up = normalize( cross( ri, rd ) );
    rd = normalize( FOV * rd + uv.x*ri + uv.y*up );
#else
    ro = _camera.pos.xyz;
    rd = normalize( FOV * _camera.dir.xyz + uv.x*_camera.right.xyz + uv.y * _camera.up.xyz );
#endif        

    SUN = vec3( sin( time8 ), 0.45, cos( time8 ) );
    SUN = normalize( SUN );
    
    rgb = render( ro, rd );
    rgb = postprocess( rgb );

    color = vec4( rgb, 1.0 );
}