#pragma once
#ifndef __SPHERE_H__
#define __SPHERE_H__

static const float VELOCITY_SCALE = 8.0f; // default: 10.0f
static const float MAX_DT = 15 / 60.0f; // min fps
/*
* RATIO based earth 1
* SUN, Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune
*/
static const std::vector<float> PLANET_RATIO =
	{ 109.2f, 0.382f, 0.949f, 1.0f, 0.532f, 10.97f, 9.14f, 3.981f, 3.865f };

struct sphere_t
{
	vec3	center=vec3(0);		// 2D position for translation
	float	radius=100.0f;		// radius
	float	theta=0.0f;			// rotation angle
	vec4	color;				// RGBA color in [0,1]
	mat4	model_matrix;		// modeling transformation
	vec3	velocity = vec3(0); // velocity of spheres
	float	mass = 1.0f;		// sphere mass for elastic collision
	int		tex_idx = -1;		// texture index
	// public functions
	void	update( float t, float dt, std::vector<sphere_t>& spheres, const std::vector<wall_t>& walls);
	bool	IsCollide(const sphere_t& other) const;
	bool	collide_wall(const wall_t& w);
	void sphere_t::SimulateElasticCollision(std::vector<sphere_t>& spheres);
	void	bounce_wall( const std::vector<wall_t>& walls);
};

inline std::vector<sphere_t> create_spheres(uint count = 1 )
{
	std::vector<sphere_t> spheres;

	for (uint k = 0, kn = 1024, n = 0; k < kn && n < count; k++)
	{
		sphere_t s;

		s.radius = randf(10.0f, 80.0f);
		s.center = vec3( randf(100.0f, 400.0f)
						, randf(100.0f, 400.0f)
						, randf(-100.0f, -400.0f)
					);
		
		bool collision_exists = false;
		for (auto& target_s : spheres)
		{
			if (target_s.IsCollide(s)) {
				collision_exists = true;
				break;
			}
		}
		if (collision_exists == true) continue;

		s.color = vec4(0.5f, 1.0f, 1.0f, 1.0f);
		s.velocity = randf3(-30.0f, 30.0f);
		s.tex_idx = n;

		spheres.emplace_back(s);
		n++;
	}

	return spheres;
}

inline bool sphere_t::IsCollide(const sphere_t& other) const
{
	if (&other == this) return false;
	return length(center - other.center) <= radius + other.radius;
}

inline bool sphere_t::collide_wall(const wall_t& w)
{
	float d = dot(w.normal, this->center) - w.dist;
	return radius > d;
}

inline void sphere_t::SimulateElasticCollision(std::vector<sphere_t>& spheres)
{
	for (auto& other : spheres)
	{
		// Check if the c is itself
		if (&other == this) 
			continue;

		// Check is collide
		if ( this->IsCollide(other) == false )
			continue;

		// this : sphere1
		// other : sphere2
		vec3 u1 = this->velocity;
		vec3 u2 = other.velocity;
		vec3 nTo1 = (this->center - other.center).normalize();
		
		// 이미 튕겨났는 지 확인
		if (dot(nTo1, u1 - u2) >= 0)
			continue; // NOTE: 이미 튕겨난 경우이므로 충돌 처리 X

		// Calculate Elastic Collsition
			// uXn: 충돌면에 수직인(normal) 속도 성분 벡터, 충돌 전후 달라지므로 갱신
		vec3 u1n = dot(u1, nTo1) * nTo1;
		vec3 u2n = dot(u2, nTo1) * nTo1;
			// uXt: 충돌면에 평행한(tangential) 속도 성분 벡터, 마찰력x -> 변화X
		vec3 u1t = this->velocity - u1n;
		vec3 u2t = other.velocity - u2n;
		float m1 = this->mass;
		float m2 = other.mass;

		this->velocity = ((m1 - m2) * u1n + 2 * m2 * u2n) / (m1 + m2) + u1t;
		other.velocity = ((m2 - m1) * u2n + 2 * m1 * u1n) / (m1 + m2) + u2t;
	}
}

inline void sphere_t::bounce_wall(const std::vector<wall_t>& walls)
{
	// floor,c,b,l,r,front
	// 0,	 1,2,3,4,5
	// check l or r
	if ( this->collide_wall(walls.at(3)) && velocity.x < 0) {
		velocity.x *= -1;
	}
	else if ( this->collide_wall(walls.at(4)) && velocity.x > 0) {
		velocity.x *= -1;
	}
	// Check c or floor
	if ( this->collide_wall(walls.at(0)) && velocity.y < 0) {
		velocity.y *= -1;
	}
	else if (this->collide_wall(walls.at(1)) && velocity.y > 0) {
		velocity.y *= -1;
	}
	// front or back
	if ( this->collide_wall(walls.at(2)) && velocity.z < 0) {
		velocity.z *= -1;
	}
	else if ( this->collide_wall(walls.at(5)) && velocity.z > 0) {
		velocity.z *= -1;
	}
}

inline void sphere_t::update( float t, float dt, std::vector<sphere_t>& spheres, const std::vector<wall_t>& walls )
{
	theta	= t;
	float c	= cos(theta), s=sin(theta);

	// these transformations will be explained in later transformation lecture
	mat4 scale_matrix =
	{
		radius, 0, 0, 0,
		0, radius, 0, 0,
		0, 0, radius, 0,
		0, 0, 0, 1
	};

	mat4 rotation_matrix =
	{
		c,-s, 0, 0,
		s, c, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
	
	this->bounce_wall(walls);
	this->SimulateElasticCollision(spheres);

	// SET MAX_DT
	if (dt > MAX_DT) dt = MAX_DT;

	center += velocity * dt * VELOCITY_SCALE;

	mat4 translate_matrix =
	{
		1, 0, 0, center.x,
		0, 1, 0, center.y,
		0, 0, 1, center.z,
		0, 0, 0, 1
	};

	mat4 x_rot_matrix =
	{
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, -1, 0, 0,
		0, 0, 0, 1
	};

	mat4 z_rot_matrix =
	{
		0, -1, 0, 0,
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// 회전 적용 o
	// model_matrix = translate_matrix * rotation_matrix * scale_matrix;
	// 회전 적용 x
	model_matrix = translate_matrix * scale_matrix * x_rot_matrix * z_rot_matrix;
}

#endif
