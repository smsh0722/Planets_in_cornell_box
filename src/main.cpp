#include "cgmath.h"		// slee's simple math library
#include "cgut.h"		// slee's OpenGL utility
#include "wall.h"		// wall class definition
#include "sphere.h"		// sphere class definition
#include "trackball.h" // virtual trackball

//*************************************
// common structures
struct light_t
{
	vec4	position = vec4(278.0f, 278.0f, 0.1f, 1.0f);
	vec4	ambient = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	vec4	diffuse = vec4(0.78f, 0.78f, 0.78f, 1.0f);
	vec4	specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

struct material_t
{
	vec4	ambient = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	vec4	diffuse = vec4(0.8f, 0.8f, 0.8f, 1.0f);
	vec4	specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	float	shininess = 1000.0f;
};

//*************************************
// global constants
static const char*	window_name = "cgbase - circle";
static const char*	vert_shader_path = "shaders/circ.vert";
static const char*	frag_shader_path = "shaders/circ.frag";
static const uint	MIN_TESS = 3;		// minimum tessellation factor (down to a triangle)
static const uint	MAX_TESS = 256;		// maximum tessellation factor (up to 256 triangles)
uint				NUM_TESS = 7;		// initial tessellation factor of the circle as a polygon
uint				NUM_LONGITUDE = 72;
uint				NUM_LATITUDE = 36;
static const char* sun_image_path		= "images/sunmap.jpg";
static const char* mercury_image_path	= "images/mercurymap.jpg";
static const char* venus_image_path		= "images/venusmap.jpg";
static const char* earth_image_path		= "images/earthmap1k.jpg";
static const char* mars_image_path		= "images/mars_1k_color.jpg";
static const char* jupiter_image_path	= "images/jupitermap.jpg";
static const char* saturn_image_path	= "images/saturnmap.jpg";
static const char* uranus_image_path	= "images/uranusmap.jpg";
static const char* neptune_image_path	= "images/neptunemap.jpg";

//*************************************
// window objects
GLFWwindow*	window = nullptr;
// ivec2		window_size = cg_default_window_size(); // initial window size
ivec2		window_size = ivec2(16*60, 9*60);

//*************************************
// OpenGL objects
GLuint	program = 0;		// ID holder for GPU program
GLuint	vertex_array = 0;	// ID holder for vertex array object
GLuint	SUN = 0;
GLuint	MERCURY = 0;
GLuint	VENUS = 0;
GLuint	EARTH = 0;
GLuint	MARS = 0;
GLuint	JUPITER = 0;
GLuint	SATURN = 0;
GLuint	URANUS = 0;
GLuint	NEPTUNE = 0;

//*************************************
// global variables
int		frame = 0;						// index of rendering frames
double	t = 0.0;						// current simulation parameter
bool	b_solid_color = false;			// use circle's color?
bool	b_index_buffer = true;			// use index buffering?
#ifndef GL_ES_VERSION_2_0
bool	b_wireframe = false;
#endif
std::vector<sphere_t> spheres;
struct { bool add=false, sub=false; operator bool() const { return add||sub; } } b; // flags of keys for smooth changes
bool	b_is_rotate = true;				// is rotate or stop
double  simul_time = 0.0f;				// simulation time
int		color_option = 0;				// color option (0, 1, 2) check circ.frag
uint	sphere_count = 9;				// 9 planets
bool	b_shadow = true;				// Shadow Toggle option

std::vector<wall_t> cornell_box;

//*************************************
// scene objects
camera		cam = camera(), home = cam;
trackball	tb;
light_t		light;
material_t	material;

//*************************************
// holder of vertices and indices of a unit sphere
std::vector<vertex>	unit_sphere_vertices;	// host-side vertices


//*************************************
void update()
{
	// update global simulation parameter
	t = simul_time;
	if (b_is_rotate)
		simul_time = glfwGetTime();
	else
		glfwSetTime(simul_time);

	cam.aspect = window_size.x / float(window_size.y);
	cam.projection_matrix = mat4::perspective(cam.fovy
		, cam.aspect
		, cam.dnear
		, cam.dfar);
	// mat4 view_projection_matrix = cam.projection_matrix * cam.view_matrix;

	// update common uniform variables in vertex/fragment shaders
	GLint uloc;
	uloc = glGetUniformLocation(program, "b_shadow");				if (uloc > -1) glUniform1i(uloc, b_shadow);
	uloc = glGetUniformLocation(program, "color_option");			if (uloc > -1) glUniform1i(uloc, color_option);
	uloc = glGetUniformLocation(program, "view_matrix");			if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.view_matrix);
	uloc = glGetUniformLocation(program, "projection_matrix");		if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.projection_matrix);

	// setup light properties
	glUniform4fv(glGetUniformLocation(program, "light_position"), 1, light.position);
	glUniform4fv(glGetUniformLocation(program, "Ia"), 1, light.ambient);
	glUniform4fv(glGetUniformLocation(program, "Id"), 1, light.diffuse);
	glUniform4fv(glGetUniformLocation(program, "Is"), 1, light.specular);

	// setup material properties
	glUniform4fv(glGetUniformLocation(program, "Ka"), 1, material.ambient);
	glUniform4fv(glGetUniformLocation(program, "Ks"), 1, material.specular);
	glUniform1f(glGetUniformLocation(program, "shininess"), material.shininess);

	// setup spheres properties
	float sphere_data[4 * 9] = { 0.0 };
	for (int i = 0; i < 9; i++) {
		vec3 c = spheres[i].center;
		float r = spheres[i].radius;
		sphere_data[i * 4] = c.x;
		sphere_data[i * 4 + 1] = c.y;
		sphere_data[i * 4 + 2] = c.z;
		sphere_data[i * 4 + 3] = r;
	}
	glUniform4fv(glGetUniformLocation(program, "spheres"), 9, sphere_data);

	// update vertex buffer by the pressed keys
	// void update_tess(); // forward declaration
	// if(b) update_tess(); 
}

void render()
{
	// clear screen (with background color) and clear depth buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// notify GL that we use our own program
	glUseProgram( program );

	// bind textures
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, SUN);
		glUniform1i(glGetUniformLocation(program, "TEX0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, MERCURY);
		glUniform1i(glGetUniformLocation(program, "TEX1"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, VENUS);
		glUniform1i(glGetUniformLocation(program, "TEX2"), 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, EARTH);
		glUniform1i(glGetUniformLocation(program, "TEX3"), 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, MARS);
		glUniform1i(glGetUniformLocation(program, "TEX4"), 4);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, JUPITER);
		glUniform1i(glGetUniformLocation(program, "TEX5"), 5);

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, SATURN);
		glUniform1i(glGetUniformLocation(program, "TEX6"), 6);

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, URANUS);
		glUniform1i(glGetUniformLocation(program, "TEX7"), 7);

		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, NEPTUNE);
		glUniform1i(glGetUniformLocation(program, "TEX8"), 8);
	}

	// bind vertex array object
	glBindVertexArray( vertex_array );

	static double t0 = 0;
	double dt = t - t0;

	// trigger shader program to process vertex data
	for( auto& s : spheres )
	{
		// per-circle update
		s.update(float(t), float(dt), spheres, cornell_box );

		// update per-circle uniforms
		GLint uloc;
		uloc = glGetUniformLocation(program, "tex_idx");
		if (uloc > -1) glUniform1i(uloc, s.tex_idx);

		uloc = glGetUniformLocation( program, "model_matrix" );
		if(uloc>-1) glUniformMatrix4fv( uloc, 1, GL_TRUE, s.model_matrix );

		// per-circle draw calls
		glDrawElements(GL_TRIANGLES
					, (NUM_LONGITUDE * NUM_LATITUDE * 3 * 2)
					, GL_UNSIGNED_INT
					, nullptr );
	}
	
	for (auto& w : cornell_box)
	{
		// bind vertex array object
		glBindVertexArray(w.vertex_array);

		GLint uloc;
		uloc = glGetUniformLocation(program, "is_wall");
		if (uloc > -1) glUniform1i(uloc, true);
		uloc = glGetUniformLocation(program, "wall_color");		if (uloc > -1) glUniform4fv(uloc, 1, w.color);	// pointer version
		uloc = glGetUniformLocation(program, "model_matrix");		if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, w.model_matrix);

		// per-circle draw calls
		glDrawElements(GL_TRIANGLES
			, int(w.indices.size())
			, GL_UNSIGNED_INT
			, nullptr);
	}
	GLint uloc;
	uloc = glGetUniformLocation(program, "is_wall");
	if (uloc > -1) glUniform1i(uloc, false);
	t0 = float(t);

	// swap front and back buffers, and display to screen
	glfwSwapBuffers( window );
}

void reshape( GLFWwindow* window, int width, int height )
{
	// set current viewport in pixels (win_x, win_y, win_width, win_height)
	// viewport: the window area that are affected by rendering 
	window_size = ivec2(width,height);
	glViewport( 0, 0, width, height );
}

void print_help()
{
	printf( "[help]\n" );
	printf( "- press ESC or 'q' to terminate the program\n" );
	printf( "- press F1 or 'h' to see help\n" );
	printf( "- press 'w' to toggle wireframe\n" );
	// printf( "- press 'd' to toggle (tc.xy,0) > (tc.xxx) > (tx.yyy) \n");
	// printf( "- press 'r' to rotate the sphere\n" );
	printf( "- press Home to reset camer\n");
	printf( "- press 'e' to toggle shadows\n");
	printf( "- press Space (or Pause) to pause the simulation");

#ifndef GL_ES_VERSION_2_0
#endif
	printf( "\n" );
}

std::vector<vertex> create_circle_vertices( uint N )
{
	std::vector<vertex> v = {{ vec3(0), vec3(0,0,-1.0f), vec2(0.5f) }}; // origin
	for( uint k=0; k <= N; k++ )
	{
		float t=PI*2.0f*k/float(N), c=cos(t), s=sin(t);
		v.push_back( { vec3(c,s,0), vec3(0,0,-1.0f), vec2(c,s)*0.5f+0.5f } );
	}
	return v;
}

std::vector<vertex> create_sphere_vertices(uint longitude, uint latitude)
{
	std::vector<vertex> v;
	for (uint lat_i = 0; lat_i <= latitude; lat_i++) {
		float t = PI * lat_i / latitude;
		for (uint lon_i = 0; lon_i <= longitude; lon_i++) {
			float p = 2.0f * PI * lon_i / longitude;
			float x = sin(t) * cos(p);
			float y = sin(t) * sin(p);
			float z = cos(t);
			v.push_back({ 
				vec3(x, y, z)
				, normalize( vec3(x, y, z))
				, vec2(p / (2.0f*PI), 1.0f - t/PI) });
		}
	}
	return v;
}

void update_vertex_buffer( const std::vector<vertex>& vertices, const uint longitude, const uint latitude )
{
	static GLuint vertex_buffer = 0;	// ID holder for vertex buffer
	static GLuint index_buffer = 0;		// ID holder for index buffer

	// clear and create new buffers
	if(vertex_buffer){	glDeleteBuffers( 1, &vertex_buffer );	vertex_buffer = 0; }
	if(index_buffer){	glDeleteBuffers( 1, &index_buffer );	index_buffer = 0; }

	// check exceptions
	if(vertices.empty()){ printf("[error] vertices is empty.\n"); return; }

	// create buffers
	std::vector<uint> indices;
	for (uint lat_i = 0; lat_i < latitude; lat_i++) {
		for (uint lon_i = 0; lon_i < longitude; lon_i++) {
			/* A - D
			*  |   |
			*  B - C
			*/
			uint a = lat_i * (longitude + 1) + lon_i; // longitude = (# of edges), vertices = (# of edges) + 1
			uint b = a + (longitude + 1);
			uint c = b + 1;
			uint d = a + 1;

			indices.push_back(a); indices.push_back(b); indices.push_back(d);

			indices.push_back(d); indices.push_back(b); indices.push_back(c);

		}
	}

	// generation of vertex buffer: use vertices as it is
	glGenBuffers( 1, &vertex_buffer );
	glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(vertex)*vertices.size(), &vertices[0], GL_STATIC_DRAW);

	// geneation of index buffer
	glGenBuffers( 1, &index_buffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_buffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(uint)*indices.size(), &indices[0], GL_STATIC_DRAW );

	// generate vertex array object, which is mandatory for OpenGL 3.3 and higher
	if(vertex_array) glDeleteVertexArrays(1,&vertex_array);
	vertex_array = cg_create_vertex_array( vertex_buffer, index_buffer );
	if(!vertex_array){ printf("%s(): failed to create vertex aray\n",__func__); return; }
}

void update_tess()
{
	uint n = NUM_TESS; if(b.add) n++; if(b.sub) n--;
	if(n==NUM_TESS||n<MIN_TESS||n>MAX_TESS) return;
	
	// unit_circle_vertices = create_circle_vertices(NUM_TESS=n);
	unit_sphere_vertices = create_sphere_vertices(NUM_LONGITUDE, NUM_LATITUDE);

	update_vertex_buffer( unit_sphere_vertices
						, NUM_LONGITUDE
						, NUM_LATITUDE );
	printf( "> NUM_TESS = % -4d\r", NUM_TESS );
}

void keyboard( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	if(action==GLFW_PRESS)
	{
		if(key==GLFW_KEY_ESCAPE||key==GLFW_KEY_Q)	glfwSetWindowShouldClose( window, GL_TRUE );
		else if(key==GLFW_KEY_H||key==GLFW_KEY_F1)	print_help();
		else if(key==GLFW_KEY_KP_ADD||(key==GLFW_KEY_EQUAL&&(mods&GLFW_MOD_SHIFT)))	b.add = true;
		else if(key==GLFW_KEY_KP_SUBTRACT||key==GLFW_KEY_MINUS) b.sub = true;
		else if(key==GLFW_KEY_I)
		{
			b_index_buffer = !b_index_buffer;
			update_vertex_buffer( unit_sphere_vertices
								, NUM_LONGITUDE
								, NUM_LATITUDE);
			printf( "> using %s buffering\n", b_index_buffer?"index":"vertex" );
		}
		else if(key==GLFW_KEY_D)
		{
			color_option = (color_option + 1) % 3;
			if (color_option == 0) printf("> using (%s) as color\n", "texcoord.xy,0");
			else if (color_option == 1) printf("> using (%s) as color\n", "texcoord.xxx");
			else if (color_option == 2) printf("> using (%s) as color\n", "texcoord.yyy");
			else printf("> using (%s) as color\n", "Internal error");
		}
		else if (key == GLFW_KEY_E)
		{
			b_shadow = !b_shadow;
		}
#ifndef GL_ES_VERSION_2_0
		else if(key==GLFW_KEY_W)
		{
			b_wireframe = !b_wireframe;
			glPolygonMode( GL_FRONT_AND_BACK, b_wireframe ? GL_LINE:GL_FILL );
			printf( "> using %s mode\n", b_wireframe ? "wireframe" : "solid" );
		}
		else if (key == GLFW_KEY_R)
		{
			b_is_rotate = !b_is_rotate;
		}
		else if (key == GLFW_KEY_HOME)
		{
			cam = tb.reset(cam);
		}
		else if (key == GLFW_KEY_SPACE || key == GLFW_KEY_PAUSE)
		{
			b_is_rotate = !b_is_rotate;
		}
#endif
	}
	else if(action==GLFW_RELEASE)
	{
		if (key == GLFW_KEY_KP_ADD || (key == GLFW_KEY_EQUAL && (mods & GLFW_MOD_SHIFT)))	b.add = false;
		else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) b.sub = false;
	}
}

void mouse( GLFWwindow* window, int button, int action, int mods )
{
	dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
	vec2 npos = cursor_to_ndc(pos, window_size);

	if(button==GLFW_MOUSE_BUTTON_LEFT ) 
	{ 
		if (mods & GLFW_MOD_SHIFT) 
		{ // zooming
			if (action == GLFW_PRESS)			tb.begin_zooming(cam, npos);
			else if (action == GLFW_RELEASE)	tb.end_zooming();
		}
		else if (mods & GLFW_MOD_CONTROL) 
		{ // panning
			if (action == GLFW_PRESS)			tb.begin_panning(cam, npos);
			else if (action == GLFW_RELEASE)	tb.end_panning();
		}
		else 
		{	// tracking
			if (action == GLFW_PRESS)			tb.begin_tracking(cam, npos);
			else if (action == GLFW_RELEASE)	tb.end_tracking();
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{ // panning
		if (action == GLFW_PRESS)			tb.begin_panning(cam, npos);
		else if (action == GLFW_RELEASE)	tb.end_panning();
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{ // zooming
		if (action == GLFW_PRESS)			tb.begin_zooming(cam, npos);
		else if (action == GLFW_RELEASE)	tb.end_zooming();
	}
}

void motion( GLFWwindow* window, double x, double y )
{
	if (tb.is_tracking() == true ||
		tb.is_zomming() == true ||
		tb.is_panning() == true)
	{
		vec2 npos = cursor_to_ndc(dvec2(x, y), window_size);
		cam = tb.update(npos);
	}
}

// NOTE: refer gl-06-texture
GLuint create_texture(const char* image_path, bool mipmap = true, GLenum wrap = GL_CLAMP_TO_EDGE, GLenum filter = GL_LINEAR)
{
	// load image
	image* i = cg_load_image(image_path); if (!i) return 0; // return null texture; 0 is reserved as a null texture
	int		w = i->width, h = i->height, c = i->channels;

	// induce internal format and format from image
	GLint	internal_format = c == 1 ? GL_R8 : c == 2 ? GL_RG8 : c == 3 ? GL_RGB8 : GL_RGBA8;
	GLenum	format = c == 1 ? GL_RED : c == 2 ? GL_RG : c == 3 ? GL_RGB : GL_RGBA;

	// create a src texture (lena texture)
	GLuint texture;
	glGenTextures(1, &texture); if (texture == 0) { printf("%s(): failed in glGenTextures()\n", __func__); return 0; }
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, GL_UNSIGNED_BYTE, i->ptr);
	if (i) { delete i; i = nullptr; } // release image

	// build mipmap
	if (mipmap)
	{
		int mip_levels = 0; for (int k = w > h ? w : h; k; k >>= 1) mip_levels++;
		for (int l = 1; l < mip_levels; l++)
			glTexImage2D(GL_TEXTURE_2D, l, internal_format, (w >> l) == 0 ? 1 : (w >> l), (h >> l) == 0 ? 1 : (h >> l), 0, format, GL_UNSIGNED_BYTE, nullptr);
		if (glGenerateMipmap) glGenerateMipmap(GL_TEXTURE_2D);
	}

	// set up texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, !mipmap ? filter : filter == GL_LINEAR ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);

	return texture;
}

bool user_init()
{
	// log hotkeys
	print_help();

	// init GL states
	glLineWidth( 1.0f );
	glClearColor( 39/255.0f, 40/255.0f, 34/255.0f, 1.0f );	// set clear color
	glEnable( GL_CULL_FACE );								// turn on backface culling
	glEnable( GL_DEPTH_TEST );								// turn on depth tests
	
	// create cornell box
	cornell_box = create_cornellbox();

	// create spheres
	spheres = create_spheres(sphere_count);

	// define the position of four corner vertices
	unit_sphere_vertices = std::move(create_sphere_vertices( NUM_LONGITUDE, NUM_LATITUDE));

	// create vertex buffer; called again when index buffering mode is toggled
	update_vertex_buffer(unit_sphere_vertices
		, NUM_LONGITUDE
		, NUM_LATITUDE);

	// load images to texture
	SUN = create_texture(sun_image_path, true); if (!SUN) return false;
	MERCURY = create_texture(mercury_image_path, true); if (!MERCURY) return false;
	VENUS = create_texture(venus_image_path, true); if (!VENUS) return false;
	EARTH = create_texture(earth_image_path, true); if (!EARTH) return false;
	MARS = create_texture(mars_image_path, true); if (!MARS) return false;
	JUPITER = create_texture(jupiter_image_path, true); if (!JUPITER) return false;
	SATURN = create_texture(saturn_image_path, true); if (!SATURN) return false;
	URANUS = create_texture(uranus_image_path, true); if (!URANUS) return false;
	NEPTUNE = create_texture(neptune_image_path, true); if (!NEPTUNE) return false;

	return true;
}

void user_finalize()
{
}

int main( int argc, char* argv[] )
{
	// create window and initialize OpenGL extensions
	if(!(window = cg_create_window( window_name, window_size.x, window_size.y ))){ glfwTerminate(); return 1; }
	if(!cg_init_extensions( window )){ glfwTerminate(); return 1; }	// init OpenGL extensions

	// initializations and validations of GLSL program
	if(!(program=cg_create_program( vert_shader_path, frag_shader_path ))){ glfwTerminate(); return 1; }	// create and compile shaders/program
	if(!user_init()){ printf( "Failed to user_init()\n" ); glfwTerminate(); return 1; }					// user initialization

	// register event callbacks
	glfwSetWindowSizeCallback( window, reshape );	// callback for window resizing events
    glfwSetKeyCallback( window, keyboard );			// callback for keyboard events
	glfwSetMouseButtonCallback( window, mouse );	// callback for mouse click inputs
	glfwSetCursorPosCallback( window, motion );		// callback for mouse movements

	// enters rendering/event loop
	for( frame=0; !glfwWindowShouldClose(window); frame++ )
	{
		glfwPollEvents();	// polling and processing of events
		update();			// per-frame update
		render();			// per-frame render
	}
	
	// normal termination
	user_finalize();
	cg_destroy_window(window);

	return 0;
}
