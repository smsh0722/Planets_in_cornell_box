#pragma once
#ifndef __TRACKBALL_H__
#define __TRACKBALL_H__

static const vec3	DEFAULT_EYE = vec3(273, 273, 800);
static const vec3	DEFAULT_AT = vec3(273, 273, -273);
static const vec3	DEFAULT_UP = vec3(0, 1, 0);

struct camera
{
	vec3	eye = vec3(273, 273, 800);
	vec3	at = vec3(273, 273, -273);
	vec3	up = vec3(0, 1, 0);
	mat4	view_matrix;

	float	fovy = PI / 4.0f; // must be in radian
	float	aspect = 16.0f / 9.0f;
	float	dnear = 1.0f;
	float	dfar = 5000.0f;
	mat4	projection_matrix;

	camera() { view_matrix = mat4::look_at(eye, at, up); }
	camera(vec3 Eye, vec3 At = vec3(0), vec3 Up = vec3(0, 0, 1)) :eye(Eye), at(At), up(Up) { view_matrix = mat4::look_at(eye, at, up); }
};

struct trackball
{
	bool	b_tracking = false;
	bool	b_zomming = false;
	bool	b_panning = false;

	float	scale = 1.0f;	// controls how much rotation is applied
	float	zoom_scale = 1000.0f;
	float	panning_scale = 300.0f; 
	camera	cam0;			// initial camera when an action begins
	vec2	m0;				// the last mouse position

	trackball(float rot_scale = 1.0f) : scale(rot_scale) {}
	
	bool is_tracking() const { return b_tracking; }
	bool is_zomming() const { return b_zomming; }
	bool is_panning() const { return b_panning; }

	void begin_tracking(const camera& cam, vec2 m);
	void end_tracking() { b_tracking = false; }

	void begin_zooming(const camera& cam, vec2 m);
	void end_zooming() { b_zomming = false; }

	void begin_panning(const camera& cam, vec2 m);
	void end_panning() { b_panning = false; }

	camera update(vec2 m) const;
	camera reset(const camera& cam) const;
};

inline void trackball::begin_tracking(const camera& cam, vec2 m)
{
	b_tracking = true;	// enable trackball tracking
	b_zomming = false;
	b_panning = false;
	m0 = m;				// save current mouse position
	cam0 = cam;			// save current camera
}

inline void trackball::begin_zooming(const camera& cam, vec2 m)
{
	b_tracking = false;
	b_zomming = true;
	b_panning = false;
	m0 = m;
	cam0 = cam;
}

inline void trackball::begin_panning(const camera& cam, vec2 m)
{
	b_tracking = false;
	b_zomming = false;
	b_panning = true;
	m0 = m;
	cam0 = cam;
}

inline camera trackball::update(vec2 m) const
{
	camera c = cam0;

	if (b_tracking == true)
	{
		// project a 2D mouse position to a unit sphere
		static const vec3 p0 = vec3(0, 0, 1.0f); // reference position on sphere
		vec3 p1 = vec3(m - m0, 0); // displacement
		if (length(p1) < 0.0001f) return c; // ignore subtle movement
		p1 *= scale; // apply rotation scale
		p1.z = sqrtf(std::max(0.0f, 1.0f - length2(p1))); // back-project z=0 onto the unit sphere
		p1 = p1.normalize();

		// find rotation axis and angle in world space
		// - trackball self-rotation should be done at first in the world space
		// - mat3(cam0.view_matrix): rotation-only view matrix
		// - mat3(cam0.view_matrix).transpose(): inverse view-to-world matrix
		vec3 v = mat3(cam0.view_matrix).transpose() * p0.cross(p1);
		float theta = asin(std::min(v.length(), 1.0f));

		// build matrices to transform eye and up
		mat4 T = mat4::translate(cam0.at);
		mat4 R = mat4::rotate(v.normalize(), -theta); // use -theta, since we're rotating camera directly

		c.eye = T * R * (c.eye - c.at);	// rotate view vector and translate by cam0.at
		c.up = R * c.up;			// rotate up vector
		c.view_matrix = mat4::look_at(c.eye, c.at, c.up);
	}
	else if (b_zomming == true)
	{
		vec3 p1 = vec3(m - m0, 0);
		
		vec3 n = (cam0.eye - cam0.at).normalize();

		c.eye += n * p1.y * zoom_scale;                 
		c.view_matrix = mat4::look_at(c.eye, c.at, c.up);
	}
	else if (b_panning == true)
	{
		vec3 p1 = vec3(m0 - m, 0);
		
		// refer gl-06-camera.pdf
		vec3 n = (cam0.eye - cam0.at).normalize();
		vec3 u = cam0.up.cross(n).normalize();
		vec3 v = n.cross(u).normalize();
		
		vec3 panning = (u * p1.x + v * p1.y) * panning_scale;
		c.eye += panning;
		c.at += panning;
		c.view_matrix = mat4::look_at(c.eye, c.at, c.up);
	}

	return c;
}

inline camera trackball::reset(const camera& cam) const
{
	camera c = cam;
	c.eye = DEFAULT_EYE;
	c.at = DEFAULT_AT;
	c.up = DEFAULT_UP;
	c.view_matrix = mat4::look_at(c.eye, c.at, c.up);

	return c;
}

// utility function
inline vec2 cursor_to_ndc(dvec2 cursor, ivec2 window_size)
{
	// normalize window pos to [0,1]^2
	vec2 npos = vec2(float(cursor.x) / float(window_size.x - 1),
		float(cursor.y) / float(window_size.y - 1));

	// normalize window pos to [-1,1]^2 with vertical flipping
	// vertical flipping: window coordinate system defines y from
	// top to bottom, while the trackball from bottom to top
	return vec2(npos.x * 2.0f - 1.0f, 1.0f - npos.y * 2.0f);
}

#endif // __TRACKBALL_H__
