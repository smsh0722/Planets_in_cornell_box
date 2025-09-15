#ifdef GL_ES
	#ifndef GL_FRAGMENT_PRECISION_HIGH	// highp may not be defined
		#define highp mediump
	#endif
	precision highp float; // default precision needs to be defined
#endif

struct ray_t {
	vec3 pos;
	vec3 dir;
};

// inputs from vertex shader
in vec4 epos;
in vec3 norm;
in vec2 tc;	// used for texture coordinate visualization
in vec4 wpos;

// output of the fragment shader
out vec4 fragColor;

// --------- uniform variables ----------------
uniform mat4	view_matrix;
uniform float	shininess;
uniform vec4	light_position, Ia, Id, Is;	// light
uniform vec4	Ka, Ks;					// material properties

// shader's global variables, called the uniform variables
// uniform bool b_solid_color;
// uniform vec4 solid_color;
uniform int color_option;
uniform bool is_wall=false;
uniform vec4 wall_color;
uniform	bool b_shadow=true;

// texture
uniform sampler2D	TEX0;
uniform sampler2D	TEX1;
uniform sampler2D	TEX2;
uniform sampler2D	TEX3;
uniform sampler2D	TEX4;
uniform sampler2D	TEX5;
uniform sampler2D	TEX6;
uniform sampler2D	TEX7;
uniform sampler2D	TEX8;
uniform int			tex_idx;

uniform vec4 spheres[9]; // {x, y, z, r}

// Hard Shadow
bool intersect_sphere( ray_t r, vec4 s )
{
	// NOTE: VII.2 Intersection of a ray with a sphere¡± in the Graphics Gems I book.
	vec3 center = s.xyz;
	float radius = s.w;

	vec3 eo = center - r.pos;
	vec3 V = normalize(r.dir);
	float v = dot(eo, V );
	float disc = radius*radius - ( dot(eo,eo) - v * v );
	if ( disc < 0 ) return false;
	else {
		float d = sqrt(disc);
		vec3 P = r.pos + ( v -d )*V;
		vec3 E_to_P = P - r.pos;
		if  ( dot( normalize(E_to_P), V) > 0.995 ) return true; // NOTE: float margin
		else false;
	}
}
bool is_fragment_in_shadow( vec3 fragment_pos, int cur_idx )
{
	ray_t r;
	r.pos = fragment_pos;
	r.dir = normalize(light_position.xyz - fragment_pos);

	// Find any intersect
	for ( int i = 0; i < 9 ; i++ )
	{
		if ( i == cur_idx ) continue;

		if ( intersect_sphere(r, spheres[i] ) == true )
			return true;
	}

	return false;
}

// Shading : blinn-phong 
vec4 phong( vec3 l, vec3 n, vec3 h, vec4 Kd )
{
	vec4 Ira = Ka*Ia;									// ambient reflection
	vec4 Ird = max(Kd*dot(l,n)*Id,0.0);					// diffuse reflection
	vec4 Irs = max(Ks*pow(dot(h,n),shininess)*Is,0.0);	// specular reflection
	return Ira + Ird + Irs;
}

void main()
{
	// light position in the eye space
	vec4 lpos = view_matrix*light_position;

	vec3 n = normalize(norm);	// norm interpolated via rasterizer should be normalized again here
	vec3 p = epos.xyz;			// 3D position of this fragment
	vec3 l = normalize(lpos.xyz-(lpos.a==0.0?vec3(0):p));	// lpos.a==0 means directional light
	vec3 v = normalize(-p);		// eye-epos = vec3(0)-epos
	vec3 h = normalize(l+v);	// the halfway vector


	if( is_wall == true ){
		fragColor = phong( l, n, h, wall_color);

		if ( b_shadow == true )
		{
			bool b_is_fragment_in_shadow = is_fragment_in_shadow(wpos.xyz, -1);
			if ( b_is_fragment_in_shadow  == true ){
				fragColor = vec4( fragColor.rgb * 0.5, fragColor.a );
			}
		}
	}
	else {
		// fragColor = b_solid_color ? solid_color : vec4(tc.xy,0,1);
		/*
		if ( color_option == 0 )
			fragColor = vec4(tc.xy, 0, 1);
		else if ( color_option == 1 )
			fragColor = vec4(tc.xxx, 1 );
		else if ( color_option == 2 )
			fragColor = vec4(tc.yyy, 1 );
		*/

		vec4 Kd = vec4(1.0);
		if (tex_idx == 0 ) Kd = texture( TEX0, tc);
		else if (tex_idx == 1 ) Kd = texture( TEX1, tc);
		else if (tex_idx == 2 ) Kd = texture( TEX2, tc);
		else if (tex_idx == 3 ) Kd = texture( TEX3, tc);
		else if (tex_idx == 4 ) Kd = texture( TEX4, tc);
		else if (tex_idx == 5 ) Kd = texture( TEX5, tc);
		else if (tex_idx == 6 ) Kd = texture( TEX6, tc);
		else if (tex_idx == 7 ) Kd = texture( TEX7, tc);
		else if (tex_idx == 8 ) Kd = texture( TEX8, tc);

		fragColor = phong( l, n, h, Kd );

		if ( b_shadow == true )
		{
			bool b_is_fragment_in_shadow  = is_fragment_in_shadow(wpos.xyz, tex_idx);
			if ( b_is_fragment_in_shadow  == true ){
				fragColor = vec4( fragColor.rgb * 0.5, fragColor.a );
			}
		}
	}
}