#pragma once
#ifndef __WALL_H__
#define __WALL_H__

const vec4 GRAY = { 0.725f, 0.710f, 0.680f, 1.0f };
const vec4 RED = { 0.630f, 0.065f, 0.050f, 1.0f };
const vec4 GREEN = { 0.137f, 0.447f, 0.090f, 1.0f };

struct wall_t
{
	// geometry information
	std::vector<vertex> vertices;
	std::vector<uint> indices;
	// gpu
	GLuint				vertex_array = 0;
	GLuint				vertex_buffer = 0;
	GLuint				index_buffer = 0;

	vec4				color;
	mat4				model_matrix =
						{
							1, 0, 0, 0,
							0, 1, 0, 0,
							0, 0, 1, 0,
							0, 0, 0, 1
						};

	vec3				normal; // n vector of plane
	float				dist;   // distance from O to plane
};

inline void set_wall_vao(wall_t& wall)
{
	// clear and create new buffers
	if (wall.vertex_buffer) { glDeleteBuffers(1, &wall.vertex_buffer);	wall.vertex_buffer = 0; }
	if (wall.index_buffer) { glDeleteBuffers(1, &wall.index_buffer);	wall.index_buffer = 0; }

	// check exceptions
	if (wall.vertices.empty()) { printf("[error] vertices is empty.\n"); return; }

	// generation of vertex buffer: use vertices as it is
	glGenBuffers(1, &wall.vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, wall.vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * wall.vertices.size(), &wall.vertices[0], GL_STATIC_DRAW);

	// geneation of index buffer
	glGenBuffers(1, &wall.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wall.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * wall.indices.size(), &wall.indices[0], GL_STATIC_DRAW);

	// generate vertex array object, which is mandatory for OpenGL 3.3 and higher
	if (wall.vertex_array) glDeleteVertexArrays(1, &wall.vertex_array);
	wall.vertex_array = cg_create_vertex_array(wall.vertex_buffer, wall.index_buffer);
	if (!wall.vertex_array) { printf("%s(): failed to create vertex aray\n", __func__); return; }
}

inline std::vector<wall_t> create_cornellbox()
{
	std::vector<wall_t> walls;

	{	// floor
		/*
			23
			10
		*/
		wall_t wall;
		wall.color = GRAY;
		wall.vertices =
		{
			{ vec3(552.8f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f) },
			{ vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f) },
			{ vec3(0.0f, 0.0f, -559.2f), vec3(0.0f, 1.0f, 0.0f) },
			{ vec3(549.6f, 0.0f, -559.2f), vec3(0.0f, 1.0f, 0.0f) }
		};
		wall.indices = { 1,0,3,3,2,1 };
		wall.normal = vec3(0.0f, 1.0f, 0.0f);
		wall.dist = 0.0f;
		
		set_wall_vao(wall);

		walls.push_back(wall);
	}

	{	// ceil
		/* NOTE: 카메라가 바라볼 때 시계 반대 방향
			30
			21
		*/
		wall_t wall;
		wall.color = GRAY;
		wall.vertices =
		{
			{vec3(556.0f, 548.8f, 0.0f), vec3(0.0f, -1.0f, 0.0f)},
			{vec3(556.0f, 548.8f, -559.2f), vec3(0.0f, -1.0f, 0.0f)},
			{vec3(0.0f, 548.8f, -559.2f), vec3(0.0f, -1.0f, 0.0f)},
			{vec3(0.0f, 548.8f, 0.0f), vec3(0.0f, -1.0f, 0.0f)}
		};
		wall.indices = { 3,2,1,1,0,3 };
		wall.normal = vec3(0.0f, -1.0f, 0.0f);
		wall.dist = -548.8f;

		set_wall_vao(wall);

		walls.push_back(wall);
	}

	{ // back
		/*
			23
			10
		*/
		wall_t wall;
		wall.color = GRAY;
		wall.vertices =
		{
			{vec3(549.6f, 0.0f, -559.2f), vec3(0.0f, 0.0f, 1.0f)},
			{vec3(0.0f, 0.0f, -559.2f), vec3(0.0f, 0.0f, 1.0f)},
			{vec3(0.0f, 548.8f, -559.2f), vec3(0.0f, 0.0f, 1.0f)},
			{vec3(556.0f, 548.8f, -559.2f), vec3(0.0f, 0.0f, 1.0f)}
		};
		wall.indices = { 1,0,3,3,2,1 };
		wall.normal = vec3(0.0f, 0.0f, 1.0f);
		wall.dist = -559.2f;

		set_wall_vao(wall);

		walls.push_back(wall);
	}

	{ // left
		/*
			23
			10
		*/
		wall_t wall;
		wall.color = RED;
		wall.vertices =
		{
			{vec3(0.0f, 0.0f, -559.2f), vec3(1.0f, 0.0f, 0.0f)},
			{vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f)},
			{vec3(0.0f, 548.8f, 0.0f), vec3(1.0f, 0.0f, 0.0f)},
			{vec3(0.0f, 548.8f, -559.2f), vec3(1.0f, 0.0f, 0.0f)}
		};
		wall.indices = { 1,0,3,3,2,1 };
		wall.normal = vec3(1.0f, 0.0f, 0.0f);
		wall.dist = 0.0f;

		set_wall_vao(wall);

		walls.push_back(wall);
	}

	{ // right
		/*
			23
			10
		*/
		wall_t wall;
		wall.color = GREEN;
		wall.vertices =
		{
			{vec3(552.8f, 0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f)},
			{vec3(549.6f, 0.0f, -559.2f), vec3(-1.0f, 0.0f, 0.0f)},
			{vec3(556.0f, 548.8f, -559.2f), vec3(-1.0f, 0.0f, 0.0f)},
			{vec3(556.0f, 548.8f, 0.0f), vec3(-1.0f, 0.0f, 0.0f)}
		};
		wall.indices = { 3,2,1,1,0,3 };
		wall.normal = vec3(-1.0f, 0.0f, 0.0f);
		wall.dist = -556.0f;

		set_wall_vao(wall);

		walls.push_back(wall);
	}

	{	// front
		/*
			32
			01
		*/
		wall_t wall;
		wall.color = GRAY;
		wall.vertices =
		{
			{vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)},
			{vec3(552.8f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)},
			{vec3(556.0f, 548.8f, 0.0f), vec3(0.0f, 0.0f, -1.0f)},
			{vec3(0.0f, 548.8f, 0.0f), vec3(0.0f, 0.0f, -1.0f)}
		};
		wall.indices = { 3,2,1,1,0,3 };
		wall.normal = vec3(0.0f, 0.0f, -1.0f);
		wall.dist = 0.0f;

		set_wall_vao(wall);

		walls.push_back(wall);
	}
	
	return walls;
}

#endif