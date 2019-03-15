/* Lab 6 base code - transforms using local matrix functions
to be written by students -
based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
& Ian Dunn, Christian Eckhardt
*/
#include <iostream>
#include <glad/glad.h>

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
using namespace std;
using namespace glm;

#define ssbo_size 2048
#define FPS 60

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
	std::shared_ptr<Program> prog_wall, prog_mouse, prog_deferred, prog_raytrace, prog_fire, prog_cloud;

	// Shape to be used (from obj file)
	shared_ptr<Shape> wall, mouse, cloud;

	//camera
	camera mycam;

	//texture for sim
	GLuint wall_texture, wall_normal_texture, fire_texture, cloud_texture, cloud_normal_texture;

	// textures for position, color, and normal
	GLuint fb, fb2, depth_rb, FBOpos, FBOcol, FBOnorm, FBOpos2, FBOcol2, FBOnorm2;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID, VertexArrayID2;

	// Data necessary to give our triangle to OpenGL
	GLuint VertexBufferID, VertexBufferID2;

	GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex, VertexArrayIDBox2, VertexBufferIDBox2, VertexBufferTex2;
	GLuint InstanceBuffer;

	double mouse_posX, mouse_posY;
	int pass_number = 1;

	ssbo_data ssbo_CPUMEM;
	GLuint ssbo_GPU_id;

	glm::vec3 mouse_pos;
	glm::vec2 fire_to = glm::vec2(0);
	glm::vec2 fire_to2 = glm::vec2(0);
	double time = 0.0;
	float t = 0.0;
	int voxeltoggle = 0;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
		{
			if (voxeltoggle++ > 3)
			{
				voxeltoggle = 0;
			}
			std::cout << "voxeltoggle = " << voxeltoggle << std::endl;
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
		prog_deferred->addUniform("pass");
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
			//exit(1);
		}

		prog_raytrace->init();
		prog_raytrace->addAttribute("vertPos");
		prog_raytrace->addAttribute("vertTex");
		prog_raytrace->addUniform("dovoxel");
		prog_raytrace->addUniform("mouse_pos");

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
		prog_cloud->addAttribute("vertPos");
		prog_cloud->addAttribute("vertNor");
		prog_cloud->addAttribute("vertTex");
		prog_cloud->addAttribute("InstancePos");
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

		// Initialize mesh.
		wall = make_shared<Shape>();
		wall->loadMesh(resourceDirectory + "/internet_square.obj");
		wall->resize();
		wall->init();

		// Initialize mesh.
		mouse = make_shared<Shape>();
		mouse->loadMesh(resourceDirectory + "/internet_square.obj");
		mouse->resize();
		mouse->init();

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &InstanceBuffer);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer);
		glm::vec4* positions = new glm::vec4[10];
		for (int i = 0; i < 10; i++) {
			float pos = i / 10.f;
			positions[i] = glm::vec4(pos, 0.0f, 0.0f, 0.0f);
			cout << positions[i].x << ", " << positions[i].y << endl;
		}
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 10 * sizeof(glm::vec4), positions, GL_STATIC_DRAW);

		
		int position_loc = glGetAttribLocation(prog_cloud->pid, "InstancePos");
		for (int i = 0; i < 10; i++)
		{
			// Set up the vertex attribute
			glVertexAttribPointer(position_loc + i,              // Location
				1, GL_FLOAT, GL_FALSE,       // vec4
				sizeof(vec4),                // Stride
				(void*)(sizeof(vec4) * i)); // Start offset
											 // Enable it
			glEnableVertexAttribArray(position_loc + i);
			// Make it instanced
			glVertexAttribDivisor(position_loc + i, 0);
		}
		

		int width, height, channels;
		char filepath[1000];

		std::string str = resourceDirectory + "/clouds_sprite.png";
		strcpy(filepath, str.c_str());
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &cloud_texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cloud_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/cloud_sprite_normal.png";
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
		glActiveTexture(GL_TEXTURE0);
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

		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOcol, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOpos, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOnorm, 0);

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

		glUniform1i(Tex1Loc, 0);
		glUniform1i(Tex2Loc, 1);
		glUniform1i(Tex3Loc, 2);

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

		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOcol2, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOpos2, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOnorm2, 0);

		Tex1Loc = glGetUniformLocation(prog_raytrace->pid, "col_tex");
		Tex2Loc = glGetUniformLocation(prog_raytrace->pid, "pos_tex");
		Tex3Loc = glGetUniformLocation(prog_raytrace->pid, "norm_tex");
		int Tex4Loc = glGetUniformLocation(prog_raytrace->pid, "no_light_tex");
		glUniform1i(Tex1Loc, 0);
		glUniform1i(Tex2Loc, 1);
		glUniform1i(Tex3Loc, 2);
		glUniform1i(Tex4Loc, 3);

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

	void render_to_texture() // aka render to framebuffer
	{
		glfwGetCursorPos(windowManager->windowHandle, &mouse_posX, &mouse_posY);

		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, buffers);

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		auto P = std::make_shared<MatrixStack>();
		glm::mat4 M, T, S, R;
		float pi = 3.14159625;
		float pi_half = pi / 2.;

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

		// Bottom Walls
		T = glm::translate(glm::mat4(1), glm::vec3(0.5, -.9, 0));
		S = glm::scale(glm::mat4(1), glm::vec3(0.5, 0.1, 1));
		M = T * R * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);
		T = glm::translate(glm::mat4(1), glm::vec3(-0.5, -.9, 0));
		M = T * R * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

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

		//glDisable(GL_DEPTH_TEST);

		// Draw mesh using GLSL
		prog_cloud->bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cloud_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, cloud_normal_texture);

		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(VertexArrayIDBox2);
		
		T = glm::translate(glm::mat4(1), glm::vec3(0.05, 0.3, 0));
		S = glm::scale(glm::mat4(1), glm::vec3(0.1, 0.1, 1));
		M = T*S;
		static glm::vec2 cloud_offset = glm::vec2(0.0);
		static int x = 0, y = 0;
		glUniformMatrix4fv(prog_cloud->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform2fv(prog_cloud->getUniform("cloud_offset"), 1, &cloud_offset[0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		T = glm::translate(glm::mat4(1), glm::vec3(0.15, 0.3, 0));
		M = T * S;
		x = 1;
		y = 0;
		cloud_offset = glm::vec2((x / 2.0), y / (2.0));
		glUniformMatrix4fv(prog_cloud->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform2fv(prog_cloud->getUniform("cloud_offset"), 1, &cloud_offset[0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		T = glm::translate(glm::mat4(1), glm::vec3(0.1, 0.4, 0));
		M = T * S;
		x = 0;
		y = 1;
		cloud_offset = glm::vec2((x / 2.0), (y / 2.0));
		glUniformMatrix4fv(prog_cloud->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform2fv(prog_cloud->getUniform("cloud_offset"), 1, &cloud_offset[0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		T = glm::translate(glm::mat4(1), glm::vec3(0.1, 0.2, 0));
		M = T * S;
		x = 1;
		y = 1;
		cloud_offset = glm::vec2((x / 2.0), (y / 2.0));
		glUniformMatrix4fv(prog_cloud->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform2fv(prog_cloud->getUniform("cloud_offset"), 1, &cloud_offset[0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		/*T = glm::translate(glm::mat4(1), glm::vec3(0, 0.3, 0));
		S = glm::scale(glm::mat4(1), glm::vec3(0.1, 0.1, 1));
		M = S;
		glUniformMatrix4fv(prog_cloud->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 10);*/
		prog_cloud->unbind();
		glEnable(GL_DEPTH_TEST);


		// Save output to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, FBOcol);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	void render_deferred()
	{
		if (pass_number == 2) {
			glBindFramebuffer(GL_FRAMEBUFFER, fb2);
			GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
			glDrawBuffers(3, buffers);
		}

		glClearColor(0.0, 0.0, 0.0, 0.0);
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
		glUniform1i(prog_deferred->getUniform("pass"), pass_number);
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
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		prog_deferred->unbind();

		if (pass_number == 2) {
			double frametime = get_last_elapsed_time();
			// Draw cursor
			prog_fire->bind();
			glm::mat4 M, S, T, R;
			T = glm::translate(glm::mat4(1), mouse_pos);
			S = glm::scale(glm::mat4(1), glm::vec3(0.025 * 2, 0.05 * 2, 0.05));
			R = glm::rotate(glm::mat4(1), glm::radians(180.f), glm::vec3(0, 0, 1));
			M = T * S * R;
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fire_texture);
			update_fire_tex(frametime);
			glUniform1f(prog_fire->getUniform("t"), t);
			glUniform2fv(prog_fire->getUniform("to"), 1, &fire_to.x);
			glUniform2fv(prog_fire->getUniform("to2"), 1, &fire_to2.x);
			glUniformMatrix4fv(prog_fire->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			mouse->draw(prog_fire);
			prog_fire->unbind();

			// Save output to framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, FBOcol2);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, FBOpos2);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, FBOnorm2);
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
		glUniform1i(prog_raytrace->getUniform("dovoxel"), voxeltoggle);
		glUniform3fv(prog_raytrace->getUniform("mouse_pos"), 1, &mouse_pos.x);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm2);
		glActiveTexture(GL_TEXTURE3);
		// Send in no lighting texture to use for raytracing
		glBindTexture(GL_TEXTURE_2D, FBOcol);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		prog_raytrace->unbind();
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


	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle()))
	{
		application->update_mouse();
		application->create_SSBO();
		application->render_to_texture();
		application->pass_number = 1;
		application->render_deferred();
		application->pass_number = 2;
		application->render_deferred();
		application->render_to_screen();
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	windowManager->shutdown();
	return 0;
}
