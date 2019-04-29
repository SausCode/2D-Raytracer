#include <iostream>
#include <glad/glad.h>

#define GLM_ENABLE_EXPERIMENTAL

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cstdlib>
#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "camera.h"
#include "time.h"
// used for helper in perspective
#include "glm/glm.hpp"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtx/string_cast.hpp"
using namespace std;
using namespace glm;

#define ssbo_size 2048
#define FPS 60

const int NUM_CLOUDS = 100;

double get_last_elapsed_time() {
	static double lasttime = glfwGetTime();
	double actualtime = glfwGetTime();
	double difference = actualtime - lasttime;
	lasttime = actualtime;
	return difference;
}

#ifdef WIN32
// Use dedicated GPU on windows
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
#endif

class ssbo_data
{
public:
	ivec4 angle_list[ssbo_size];
};

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog_wall, prog_mouse, prog_deferred, prog_raytrace, prog_fire, prog_cloud, prog_water;

	// Shape to be used (from obj file)
	shared_ptr<Shape> wall, mouse, cloud, water, fire;

	//camera
	camera mycam;

	//texture for sim
	GLuint wall_texture, wall_normal_texture, fire_texture, cloud_texture, cloud_normal_texture, water_texture, water_displacement_texture;

	// textures for position, color, and normal
	GLuint fb, fb2, fb3, fb4, depth_rb, FBOpos, FBOcol, FBOnorm, FBOmask, FBOpos2, FBOcol2, FBOnorm2, FBOmask2, FBOpos3, FBOcol3, FBOnorm3, FBOmask3, FBOpos4, FBOcol4, FBOnorm4, FBOmask4;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID, VertexArrayID2;

	// Data necessary to give our triangle to OpenGL
	GLuint VertexBufferID, VertexBufferID2;

	GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex, VertexArrayIDBox2, VertexBufferIDBox2, VertexBufferTex2, VertexNormDBox2;
	GLuint InstanceBuffer;

	double mouse_posX, mouse_posY;
	int pass_number = 1;

	ssbo_data ssbo_CPUMEM;
	GLuint ssbo_GPU_id;

	glm::vec3 mouse_pos;
	glm::vec2 fire_to = glm::vec2(0);
	glm::vec2 fire_to2 = glm::vec2(0);
	glm::vec2 cloud_offsets[NUM_CLOUDS];
	glm::vec2 positions[NUM_CLOUDS];
	double time = 0.0;
	float t = 0.0;
	int voxeltoggle = 0;
	float pi = 3.14159625;
	float pi_half = pi / 2.;
	vec2 cloud_center = { 4.75f, 0.0f };
	float cloud_radius = 2.0;
	glm::vec3 water_pos = glm::vec3(-0.200000, -0.900000, -0.400000);
	float rotate_z = 20.f;
	glm::vec2 resolution = glm::vec2(1920, 1080);
	bool fire_status = false;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
		{
			if (voxeltoggle++ > 0)
			{
				voxeltoggle = 0;
			}
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.pos.x -= 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.pos.x += 1;
		}
		if (key == GLFW_KEY_K && action == GLFW_PRESS)
		{
			get_SSBO_back();
		}
		if (key == GLFW_KEY_F && action == GLFW_PRESS)
		{
			fire_status = !fire_status;
		}
		if (key == GLFW_KEY_LEFT && action == GLFW_PRESS || key == GLFW_KEY_LEFT && action == GLFW_REPEAT)
		{
			water_pos.y -= .1;
			std::cout << glm::to_string(water_pos) << std::endl;
		}
		if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS || key == GLFW_KEY_RIGHT && action == GLFW_REPEAT)
		{
			water_pos.y += .1;
			std::cout << glm::to_string(water_pos) << std::endl;
		}
		if (key == GLFW_KEY_UP && action == GLFW_PRESS || key == GLFW_KEY_UP && action == GLFW_REPEAT)
		{
			//water_pos.y += .1;
			//std::cout << glm::to_string(water_pos) << std::endl;
			rotate_z += 1;
			std::cout << rotate_z << std::endl;
		}
		if (key == GLFW_KEY_DOWN && action == GLFW_PRESS || key == GLFW_KEY_DOWN && action == GLFW_REPEAT)
		{
			//water_pos.y -= .1;
			//std::cout << glm::to_string(water_pos) << std::endl;
			rotate_z -= 1;
			std::cout << rotate_z << std::endl;
		}
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			water_pos.z += .1;
			std::cout << glm::to_string(water_pos) << std::endl;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			water_pos.z -= .1;
			std::cout << glm::to_string(water_pos) << std::endl;
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		glfwGetCursorPos(window, &mouse_posX, &mouse_posY);
		cout << "Pos X " << mouse_posX << " Pos Y " << mouse_posY << endl;
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.12f, 0.34f, 0.56f, 1.0f);

		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		//culling:
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		//transparency
		glEnable(GL_BLEND);
		//next function defines how to mix the background color with the transparent pixel in the foreground. 
		//This is the standard:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Initialize the GLSL program.
		prog_wall = make_shared<Program>();
		//prog_wall->setVerbose(true);
		prog_wall->setVerbose(false);
		prog_wall->setShaderNames(resourceDirectory + "/wall_vert.glsl", resourceDirectory + "/wall_frag.glsl");

		if (!prog_wall->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		prog_wall->init();
		prog_wall->addUniform("M");
		prog_wall->addAttribute("vertPos");
		prog_wall->addAttribute("vertNor");
		prog_wall->addAttribute("vertTex");

		// Initialize the GLSL program.
		prog_water = make_shared<Program>();
		//prog_water->setVerbose(true);
		prog_water->setVerbose(false);
		prog_water->setShaderNames(resourceDirectory + "/water_vert.glsl", resourceDirectory + "/water_frag.glsl");

		if (!prog_water->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		prog_water->init();
		prog_water->addUniform("M");
		prog_water->addUniform("t");
		prog_water->addUniform("light_pos");
		prog_water->addUniform("resolution");
		prog_water->addAttribute("vertPos");
		prog_water->addAttribute("vertNor");
		prog_water->addAttribute("vertTex");

		// Initialize the GLSL program.
		prog_mouse = make_shared<Program>();
		prog_mouse->setVerbose(false);
		prog_mouse->setShaderNames(resourceDirectory + "/mouse_vert.glsl", resourceDirectory + "/mouse_frag.glsl");

		if (!prog_mouse->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		prog_mouse->init();
		prog_mouse->addUniform("M");
		prog_mouse->addAttribute("vertPos");
		prog_mouse->addAttribute("vertNor");
		prog_mouse->addAttribute("vertTex");

		// Initialize the GLSL program.
		prog_deferred = make_shared<Program>();
		prog_deferred->setVerbose(true);
		prog_deferred->setShaderNames(resourceDirectory + "/deferred_vert.glsl", resourceDirectory + "/deferred_frag.glsl");

		if (!prog_deferred->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		prog_deferred->init();
		prog_deferred->addUniform("light_pos");
		prog_deferred->addUniform("campos");
		prog_deferred->addUniform("passRender");
		prog_deferred->addUniform("screen_width");
		prog_deferred->addUniform("screen_height");
		prog_deferred->addAttribute("vertPos");
		prog_deferred->addAttribute("vertTex");

		// Initialize the GLSL program.
		prog_raytrace = make_shared<Program>();
		prog_raytrace->setVerbose(true);
		prog_raytrace->setShaderNames(resourceDirectory + "/raytrace_vert.glsl", resourceDirectory + "/raytrace_frag.glsl");

		if (!prog_raytrace->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		prog_raytrace->init();
		prog_raytrace->addAttribute("vertPos");
		prog_raytrace->addAttribute("vertTex");
		prog_raytrace->addUniform("passRender");
		prog_raytrace->addUniform("mouse_pos");
		prog_raytrace->addUniform("cloud_center");
		prog_raytrace->addUniform("cloud_radius");
		prog_raytrace->addUniform("screen_width");
		prog_raytrace->addUniform("screen_height");

		// Initialize the GLSL program.
		prog_fire = make_shared<Program>();
		prog_fire->setVerbose(false);
		prog_fire->setShaderNames(resourceDirectory + "/fire.vert", resourceDirectory + "/fire.frag");

		if (!prog_fire->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		prog_fire->init();
		prog_fire->addUniform("M");
		prog_fire->addUniform("to");
		prog_fire->addUniform("to2");
		prog_fire->addUniform("t");
		prog_fire->addAttribute("vertPos");
		prog_fire->addAttribute("vertNor");
		prog_fire->addAttribute("vertTex");

		// Initialize the GLSL program.
		prog_cloud = make_shared<Program>();
		prog_cloud->setVerbose(false);
		prog_cloud->setShaderNames(resourceDirectory + "/cloud_vert.glsl", resourceDirectory + "/cloud_frag.glsl");
		if (!prog_cloud->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_cloud->init();
		prog_cloud->addUniform("M");
		prog_cloud->addUniform("cloud_offset");
		prog_cloud->addUniform("InstancePos");
		prog_cloud->addAttribute("vertPos");
		prog_cloud->addAttribute("vertNor");
		prog_cloud->addAttribute("vertTex");
		//prog_cloud->addAttribute("InstancePos");
	}

	void initGeom(const std::string& resourceDirectory)
	{
		// Deferred Stuff
		//init rectangle mesh (2 triangles) for the post processing
		glGenVertexArrays(1, &VertexArrayIDBox);
		glBindVertexArray(VertexArrayIDBox);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);

		GLfloat *rectangle_vertices = new GLfloat[18];
		// front
		int verccount = 0;

		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_vertices, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferTex);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);

		float t = 1. / 100.;
		GLfloat *rectangle_texture_coords = new GLfloat[12];
		int texccount = 0;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rectangle_texture_coords, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(2);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenVertexArrays(1, &VertexArrayIDBox2);
		glBindVertexArray(VertexArrayIDBox2);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferIDBox2);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox2);
		GLfloat* cloud_ver = new GLfloat[18];
		int verc = 0;
		cloud_ver[verc++] = -1.0, cloud_ver[verc++] = -1.0, cloud_ver[verc++] = 0.0;
		cloud_ver[verc++] = 1.0, cloud_ver[verc++] = -1.0, cloud_ver[verc++] = 0.0;
		cloud_ver[verc++] = -1.0, cloud_ver[verc++] = 1.0, cloud_ver[verc++] = 0.0;
		cloud_ver[verc++] = 1.0, cloud_ver[verc++] = -1, cloud_ver[verc++] = 0.0;
		cloud_ver[verc++] = 1.0, cloud_ver[verc++] = 1.0, cloud_ver[verc++] = 0.0;
		cloud_ver[verc++] = -1.0, cloud_ver[verc++] = 1.0, cloud_ver[verc++] = 0.0;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), cloud_ver, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &VertexNormDBox2);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexNormDBox2);
		//color
		GLfloat cube_norm[] = {
			// front colors
			1.0, 0.0, 0.0,
			1.0, 0.0, 0.0,
			1.0, 0.0, 0.0,
			1.0, 0.0, 0.0,
			1.0, 0.0, 0.0,
			1.0, 0.0, 0.0
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_norm), cube_norm, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		
		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferTex2);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex2);
		GLfloat* cloud_tex = new GLfloat[12];
		int texc = 0;
		cloud_tex[texc++] = 0, cloud_tex[texc++] = (1.0/2);
		cloud_tex[texc++] = (1.0/2), cloud_tex[texc++] = (1.0/2);
		cloud_tex[texc++] = 0, cloud_tex[texc++] = 0;
		cloud_tex[texc++] = (1.0/2), cloud_tex[texc++] = (1.0 / 2);
		cloud_tex[texc++] = (1.0 / 2), cloud_tex[texc++] = 0;
		cloud_tex[texc++] = 0, cloud_tex[texc++] = 0;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), cloud_tex, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(2);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		calculate_instance_pos();

		// Initialize mesh.
		wall = make_shared<Shape>();
		wall->loadMesh(resourceDirectory + "/internet_square.obj");
		wall->resize();
		wall->init();

		// Initialize mesh.
		water = make_shared<Shape>();
		water->loadMesh(resourceDirectory + "/grid_high.obj");
		water->resize();
		water->init();

		// Initialize mesh.
		mouse = make_shared<Shape>();
		mouse->loadMesh(resourceDirectory + "/sphere.obj");
		mouse->resize();
		mouse->init();

		// Initialize mesh.
		fire = make_shared<Shape>();
		fire->loadMesh(resourceDirectory + "/internet_square.obj");
		fire->resize();
		fire->init();

		int width, height, channels;
		char filepath[1000];

		std::string str = resourceDirectory + "/water_hd.jpg";
		//str = resourceDirectory + "/water_ripple.jpg";
		//str = resourceDirectory + "/water.png";
		unsigned char* data = stbi_load(str.c_str(), &width, &height, &channels, 4);
		glGenTextures(1, &water_texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, water_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/waterdisplacement.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &water_displacement_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, water_displacement_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint waterTex1Location = glGetUniformLocation(prog_water->pid, "tex");
		GLuint waterTex2Location = glGetUniformLocation(prog_water->pid, "tex2");

		// Then bind the uniform samplers to texture units:
		glUseProgram(prog_water->pid);
		glUniform1i(waterTex1Location, 0);
		glUniform1i(waterTex2Location, 1);

		str = resourceDirectory + "/clouds_sprite.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &cloud_texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cloud_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/clouds_sprite_normal4 (1).png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &cloud_normal_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, cloud_normal_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//[TWOTEXTURES]
//set the 2 textures to the correct samplers in the fragment shader:
		GLuint CloudTex1Location = glGetUniformLocation(prog_cloud->pid, "tex");
		GLuint CloudTex2Location = glGetUniformLocation(prog_cloud->pid, "tex2");

		// Then bind the uniform samplers to texture units:
		glUseProgram(prog_cloud->pid);
		glUniform1i(CloudTex1Location, 0);
		glUniform1i(CloudTex2Location, 1);


		str = resourceDirectory + "/fire.png";;
		data = stbi_load(str.c_str(), &width, &height, &channels, 4);
		glGenTextures(1, &fire_texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fire_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//texture
		str = resourceDirectory + "/lvl1.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &wall_texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, wall_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/lvl1normalscombined.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &wall_normal_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, wall_normal_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location = glGetUniformLocation(prog_wall->pid, "tex");
		GLuint Tex2Location = glGetUniformLocation(prog_wall->pid, "tex2");

		// Then bind the uniform samplers to texture units:
		glUseProgram(prog_wall->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		glUseProgram(prog_deferred->pid);
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glGenFramebuffers(1, &fb);
		//glActiveTexture(GL_TEXTURE0);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);

		// Deffered Rendering stuff

		// Generate Color Texture
		glGenTextures(1, &FBOcol);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_BYTE, NULL);

		// Generate Position Texture
		glGenTextures(1, &FBOpos);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Generate Normal Texture
		glGenTextures(1, &FBOnorm);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

		// Generate Mask Texture
		glGenTextures(1, &FBOmask);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, FBOmask);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		
		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOcol, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOpos, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOnorm, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, FBOmask, 0);
		//-------------------------
		glGenRenderbuffers(1, &depth_rb);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		//-------------------------
		//Attach depth buffer to FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
		//-------------------------
		//Does the GPU support current FBO configuration?

		int Tex1Loc = glGetUniformLocation(prog_deferred->pid, "col_tex");
		int Tex2Loc = glGetUniformLocation(prog_deferred->pid, "pos_tex");
		int Tex3Loc = glGetUniformLocation(prog_deferred->pid, "norm_tex");
		int Tex4Loc = glGetUniformLocation(prog_deferred->pid, "mask_tex");
		glUniform1i(Tex1Loc, 0);
		glUniform1i(Tex2Loc, 1);
		glUniform1i(Tex3Loc, 2);
		glUniform1i(Tex4Loc, 3);


		

		glUseProgram(prog_raytrace->pid);
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glGenFramebuffers(1, &fb2);
		glBindFramebuffer(GL_FRAMEBUFFER, fb2);

		// Generate Color Texture
		glGenTextures(1, &FBOcol2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_BYTE, NULL);

		// Generate Position Texture
		glGenTextures(1, &FBOpos2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Generate Normal Texture
		glGenTextures(1, &FBOnorm2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

		// Generate Mask Texture
		glGenTextures(1, &FBOmask2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, FBOmask2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		
		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOcol2, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOpos2, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOnorm2, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, FBOmask2, 0);


		Tex1Loc = glGetUniformLocation(prog_raytrace->pid, "col_tex");
		Tex2Loc = glGetUniformLocation(prog_raytrace->pid, "pos_tex");
		Tex3Loc = glGetUniformLocation(prog_raytrace->pid, "norm_tex");
		Tex4Loc = glGetUniformLocation(prog_raytrace->pid, "mask_tex");
		glUniform1i(Tex1Loc, 0);
		glUniform1i(Tex2Loc, 1);
		glUniform1i(Tex3Loc, 2);
		glUniform1i(Tex4Loc, 3);


		glUseProgram(prog_raytrace->pid);
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glGenFramebuffers(1, &fb3);
		glBindFramebuffer(GL_FRAMEBUFFER, fb3);

		// Generate Color Texture
		glGenTextures(1, &FBOcol3);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_BYTE, NULL);

		// Generate Position Texture
		glGenTextures(1, &FBOpos3);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Generate Normal Texture
		glGenTextures(1, &FBOnorm3);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

		// Generate Mask Texture
		glGenTextures(1, &FBOmask3);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, FBOmask3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOcol3, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOpos3, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOnorm3, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, FBOmask3, 0);


		Tex1Loc = glGetUniformLocation(prog_raytrace->pid, "col_tex");
		Tex2Loc = glGetUniformLocation(prog_raytrace->pid, "pos_tex");
		Tex3Loc = glGetUniformLocation(prog_raytrace->pid, "norm_tex");
		Tex4Loc = glGetUniformLocation(prog_raytrace->pid, "mask_tex");
		glUniform1i(Tex1Loc, 0);
		glUniform1i(Tex2Loc, 1);
		glUniform1i(Tex3Loc, 2);
		glUniform1i(Tex4Loc, 3);
		
		glUseProgram(prog_raytrace->pid);
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glGenFramebuffers(1, &fb4);
		glBindFramebuffer(GL_FRAMEBUFFER, fb4);

		// Generate Color Texture
		glGenTextures(1, &FBOcol4);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_BYTE, NULL);

		// Generate Position Texture
		glGenTextures(1, &FBOpos4);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Generate Normal Texture
		glGenTextures(1, &FBOnorm4);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

		// Generate Mask Texture
		glGenTextures(1, &FBOmask4);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, FBOmask4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOcol4, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOpos4, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOnorm4, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, FBOmask4, 0);


		Tex1Loc = glGetUniformLocation(prog_raytrace->pid, "col_tex");
		Tex2Loc = glGetUniformLocation(prog_raytrace->pid, "pos_tex");
		Tex3Loc = glGetUniformLocation(prog_raytrace->pid, "norm_tex");
		Tex4Loc = glGetUniformLocation(prog_raytrace->pid, "mask_tex");
		glUniform1i(Tex1Loc, 0);
		glUniform1i(Tex2Loc, 1);
		glUniform1i(Tex3Loc, 2);
		glUniform1i(Tex4Loc, 3);

		GLenum status;
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			cout << "status framebuffer: good" << endl;
			break;
		default:
			cout << "status framebuffer: bad" << endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void create_SSBO() {
		// Fill angle list with big numbers
		for (int i = 0; i < ssbo_size; i++) {
			ssbo_CPUMEM.angle_list[i] = ivec4(i, INT_MAX, INT_MAX, INT_MAX);
		}

		glUseProgram(prog_deferred->pid);
		GLuint block_index = 0;
		block_index = glGetProgramResourceIndex(prog_deferred->pid, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 2;
		glShaderStorageBlockBinding(prog_deferred->pid, block_index, ssbo_binding_point_index);

		glGenBuffers(1, &ssbo_GPU_id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &ssbo_CPUMEM, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	}

	void clear_SSBO() {
		//Fill angle list with big numbers
		for (int i = 0; i < ssbo_size; i++) {
			ssbo_CPUMEM.angle_list[i] = ivec4(i, INT_MAX, INT_MAX, INT_MAX);
		}

		glUseProgram(prog_deferred->pid);
		glDeleteBuffers(1, &ssbo_GPU_id);
		glGenBuffers(1, &ssbo_GPU_id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &ssbo_CPUMEM, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	}


	float map(float x, float in_min, float in_max, float out_min, float out_max)
	{
		return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	}

	void get_SSBO_back() {
		// Get SSBO back
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
		int siz = sizeof(ssbo_data);
		memcpy(&ssbo_CPUMEM, p, siz);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		for (int i = 0; i < ssbo_size; i++) {
			cout << ssbo_CPUMEM.angle_list[i].x << " " << ssbo_CPUMEM.angle_list[i].y << " " << ssbo_CPUMEM.angle_list[i].z << " " << ssbo_CPUMEM.angle_list[i].w << endl;
		}
	}

	void update_mouse()
	{
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		mouse_pos = glm::vec3(mouse_posX, mouse_posY, 0);
		mouse_pos.x /= width;
		mouse_pos.y /= height;
		mouse_pos.x *= 2;
		mouse_pos.y *= 2;
		mouse_pos.x -= 1;
		mouse_pos.y -= 1;
		mouse_pos.y *= -1;
	}

	void calculate_instance_pos() {
		int x = 0, y = 0;
		float rho = 0.0, phi = 0.0, m = 0.0, n = 0.0;

		for (int i = 0; i < NUM_CLOUDS; i++) {
			rho = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			phi = static_cast <float> (rand()) / (static_cast <float> (2 * pi));
			m = sqrt(rho) * cos(phi) *cloud_radius;
			n = sqrt(rho) * sin(phi) *cloud_radius;
			glm::vec2 pos = vec2(m+cloud_center.x, n+cloud_center.y);
			x = rand() % 2;
			y = rand() % 2;
			glm::vec2 cloud_offset = glm::vec2((x / 2.0), y / (2.0));
			
			cloud_offsets[i] = cloud_offset;
			positions[i] = pos;
		}
	}

	void render_to_texture() // aka render to framebuffer
	{
		glfwGetCursorPos(windowManager->windowHandle, &mouse_posX, &mouse_posY);

		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, buffers);

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		auto P = std::make_shared<MatrixStack>();
		glm::mat4 M, T, S, R;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, wall_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, wall_normal_texture);

		prog_wall->bind();

		// WALLS
		// Top Walls
		T = glm::translate(glm::mat4(1), glm::vec3(0.5, 0.9, 0));
		S = glm::scale(glm::mat4(1), glm::vec3(0.5, 0.1, 1));
		M = T * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);
		T = glm::translate(glm::mat4(1), glm::vec3(-0.5, 0.9, 0));
		M = T * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

		// Mid wall 1
		T = glm::translate(glm::mat4(1), glm::vec3(-.5, -0.1, 0));
		S = glm::scale(glm::mat4(1), glm::vec3(0.5, 0.1, 1));
		M = T * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

		// Mid wall 2
		T = glm::translate(glm::mat4(1), glm::vec3(-.5, 0.1, 0));
		S = glm::scale(glm::mat4(1), glm::vec3(0.5, 0.1, 1));
		R = glm::rotate(glm::mat4(1), pi, glm::vec3(0, 0, 1));
		M = T * R * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

		//// Bottom Walls
		//T = glm::translate(glm::mat4(1), glm::vec3(0.5, -.9, 0));
		//S = glm::scale(glm::mat4(1), glm::vec3(0.5, 0.1, 1));
		//M = T * R * S;
		//glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		//wall->draw(prog_wall);
		//T = glm::translate(glm::mat4(1), glm::vec3(-0.5, -.9, 0));
		//M = T * R * S;
		//glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		//wall->draw(prog_wall);

		// Right Walls
		T = glm::translate(glm::mat4(1), glm::vec3(0.9, 0.5, 0));
		S = glm::scale(glm::mat4(1), glm::vec3(0.5, 0.1, 1));
		R = glm::rotate(glm::mat4(1), -pi_half, glm::vec3(0, 0, 1));
		M = T * R * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);
		T = glm::translate(glm::mat4(1), glm::vec3(0.9, -0.5, 0));
		M = T * R * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

		//done, unbind stuff
		prog_wall->unbind();


		prog_water->bind();
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, water_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, water_displacement_texture);
		T = glm::translate(glm::mat4(1), water_pos);
		S = glm::scale(glm::mat4(1), glm::vec3(1, 1, 2));
		R = glm::rotate(glm::mat4(1), glm::radians(rotate_z), glm::vec3(1, 0, 0));
		M = T * S * R;
		//glUniform1f(prog_water->getUniform("t"), timeDelta);
		glUniform1f(prog_water->getUniform("t"), glfwGetTime());
		glUniform3fv(prog_water->getUniform("light_pos"), 1, &mouse_pos.x);
		glUniform2f(prog_water->getUniform("resolution"), resolution.x, resolution.y);
		glUniformMatrix4fv(prog_water->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		water->draw(prog_water);
		//done, unbind stuff
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		prog_water->unbind();

		/*
			DRAW CLOUD
		*/

		// Draw mesh using GLSL
		prog_cloud->bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cloud_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, cloud_normal_texture);

		glUniform3fv(prog_cloud->getUniform("light_pos"), 1, &mouse_pos.x);
		glBindVertexArray(VertexArrayIDBox2);
		glDisable(GL_DEPTH_TEST);
		S = glm::scale(glm::mat4(1), glm::vec3(0.1, 0.1, 1));
		for (int i = 0; i < NUM_CLOUDS; i++) {
			glm::vec2 pos = positions[i];
			glm::vec2 cloud_offset = cloud_offsets[i];
			glUniform2fv(prog_cloud->getUniform("InstancePos"), 1, &pos[0]);
			M = S;
			glUniformMatrix4fv(prog_cloud->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glUniform2fv(prog_cloud->getUniform("cloud_offset"), 1, &cloud_offset[0]);
			glDrawArrays(GL_TRIANGLES,0,6);
		}

		prog_cloud->unbind();

		// Save output to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, FBOcol);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOmask);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	void render_deferred()
	{
		if (pass_number == 2) {
			glBindFramebuffer(GL_FRAMEBUFFER, fb2);
			GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
			glDrawBuffers(4, buffers);
		}

		glClearColor(0.0,0.0,0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		prog_deferred->bind();
		// Get SSBO ready to send
		GLuint block_index = 0;
		block_index = glGetProgramResourceIndex(prog_deferred->pid, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 0;
		glShaderStorageBlockBinding(prog_deferred->pid, block_index, ssbo_binding_point_index);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
		glUniform1i(prog_deferred->getUniform("passRender"), pass_number);
		glUniform3fv(prog_deferred->getUniform("light_pos"), 1, &mouse_pos.x);
		glUniform3fv(prog_deferred->getUniform("campos"), 1, &mycam.pos.x);
		glUniform1i(prog_deferred->getUniform("screen_width"), width);
		glUniform1i(prog_deferred->getUniform("screen_height"), height);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, FBOmask);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		prog_deferred->unbind();

		if (pass_number ==2) {
			double frametime = get_last_elapsed_time();
			glm::mat4 M, S, T, R;



			if (fire_status)
			{
				// Draw cursor
				prog_fire->bind();
				T = glm::translate(glm::mat4(1), mouse_pos);
				S = glm::scale(glm::mat4(1), glm::vec3(0.025 * 2, 0.05 * 2, 0));
				R = glm::rotate(glm::mat4(1), glm::radians(180.f), glm::vec3(0, 0, 1));
				M = T * S * R;
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, fire_texture);
				update_fire_tex(frametime);
				glUniform1f(prog_fire->getUniform("t"), t);
				glUniform2fv(prog_fire->getUniform("to"), 1, &fire_to.x);
				glUniform2fv(prog_fire->getUniform("to2"), 1, &fire_to2.x);
				glUniformMatrix4fv(prog_fire->getUniform("M"), 1, GL_FALSE, &M[0][0]);
				fire->draw(prog_fire);
				prog_fire->unbind();
			}
			else
			{
				// Draw cursor
				prog_mouse->bind();
				T = glm::translate(glm::mat4(1), mouse_pos);
				S = glm::scale(glm::mat4(1), glm::vec3(0.05, 0.05 * 2, 0));
				M = T * S;
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, fire_texture);
				glUniformMatrix4fv(prog_fire->getUniform("M"), 1, GL_FALSE, &M[0][0]);
				mouse->draw(prog_mouse);
				prog_mouse->unbind();
			}

			// Save output to framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, FBOcol2);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, FBOpos2);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, FBOnorm2);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, FBOmask2);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}

	void update_fire_tex(double frameTime)
	{
		time += frameTime;

		if (time > 1.f / (float)FPS) {
			// Time to update new frame
			time -= 1.f / (float)FPS;
			fire_to = fire_to2;
			t = time / (float)FPS;

			if (fire_to2.x >= (7.f / 8.f)) {
				if (fire_to2.y >= (7.f / 8.f)) {
					fire_to2.x = 0.f;
					fire_to2.y = 0.f;
				}
				else {
					fire_to2.x = 0.f;
					fire_to2.y += (1.f / 8.f);
				}
			}
			else {
				fire_to2.x += (1.f / 8.f);
			}
		}
		else {
			// Same frame just change t
			t = (float)time / (float)FPS;
		}
	}

	void render_raytrace_geometry()
		{
		glBindFramebuffer(GL_FRAMEBUFFER, fb3);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, buffers);
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);
		// Clear framebuffer.
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		prog_raytrace->bind();
		glUniform1i(prog_raytrace->getUniform("passRender"), 2);
		glUniform3fv(prog_raytrace->getUniform("mouse_pos"), 1, &mouse_pos.x);
		glUniform2fv(prog_raytrace->getUniform("cloud_center"), 1, &cloud_center.x);
		glUniform1f(prog_raytrace->getUniform("cloud_radius"), cloud_radius);
		glUniform1i(prog_raytrace->getUniform("screen_width"), width);
		glUniform1i(prog_raytrace->getUniform("screen_height"), height);

		FBOmask2 = FBOmask;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, FBOmask2);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		prog_raytrace->unbind();
		// Save output to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, FBOcol3);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOpos3);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOnorm3);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOmask3);
		glGenerateMipmap(GL_TEXTURE_2D);

		}
	void render_raytrace_clouds()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb4);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, buffers);
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);
		// Clear framebuffer.
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		prog_raytrace->bind();
		glUniform1i(prog_raytrace->getUniform("passRender"), 3);
		glUniform3fv(prog_raytrace->getUniform("mouse_pos"), 1, &mouse_pos.x);
		glUniform2fv(prog_raytrace->getUniform("cloud_center"), 1, &cloud_center.x);
		glUniform1f(prog_raytrace->getUniform("cloud_radius"), cloud_radius);
		glUniform1i(prog_raytrace->getUniform("screen_width"), width);
		glUniform1i(prog_raytrace->getUniform("screen_height"), height);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol3);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos3);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm3);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, FBOmask2);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		prog_raytrace->unbind();
		// Save output to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, FBOcol4);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOpos4);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOnorm4);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOmask4);
		glGenerateMipmap(GL_TEXTURE_2D);

	}
	void render_to_screen()
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);
		// Clear framebuffer.
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		prog_raytrace->bind();
		glUniform1i(prog_raytrace->getUniform("passRender"), 4);
		glUniform3fv(prog_raytrace->getUniform("mouse_pos"), 1, &mouse_pos.x);
		glUniform2fv(prog_raytrace->getUniform("cloud_center"), 1, &cloud_center.x);
		glUniform1f(prog_raytrace->getUniform("cloud_radius"), cloud_radius);
		glUniform1i(prog_raytrace->getUniform("screen_width"), width);
		glUniform1i(prog_raytrace->getUniform("screen_height"), height);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol4);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos4);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm4);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, FBOmask2);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		prog_raytrace->unbind();
		// Save output to framebuffer
		

	}
};

//*********************************************************************************************************
int main(int argc, char **argv)
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);


	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);
	application->create_SSBO();

	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle()))
	{
		application->clear_SSBO();
		application->update_mouse();
		application->render_to_texture();
		application->pass_number = 1;
		application->render_deferred();//fill shadow list
		application->pass_number = 2;
		application->render_deferred();//apply shadow list
		application->render_raytrace_geometry();//ray trace everything
		application->render_raytrace_clouds();//clouds inside only!
		application->render_to_screen();//ray trace everything but clouds

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	windowManager->shutdown();
	return 0;
}
