// input attributes of vertices
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texcoord;

// outputs of vertex shader = input to fragment shader
// out vec4 gl_Position: a built-in output variable that should be written in main()
out vec4 epos;	// eye-space position
out vec3 norm;	// the second output: not used yet
out vec2 tc;	// the third output: not used yet
out vec4 wpos;

// uniform variables
uniform mat4	model_matrix;	// 4x4 transformation matrix: explained later in the lecture
uniform mat4	view_matrix;	// tricky 4x4 aspect-correction matrix
uniform mat4	projection_matrix;

void main()
{
	wpos = model_matrix * vec4(position,1);
	epos = view_matrix * wpos;
	gl_Position = projection_matrix * epos;

	// other outputs to rasterizer/fragment shader
	norm = normalize(mat3(view_matrix*model_matrix)*normal);
	tc = texcoord;
}