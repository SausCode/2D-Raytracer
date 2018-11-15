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

#ifdef __WIN32
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
    std::shared_ptr<Program> prog_wall, prog_mouse, prog_deferred, voxprog, prog2;
 
	// Shape to be used (from obj file)
    shared_ptr<Shape> wall, mouse;
 
	//camera
	camera mycam;

	//texture for sim
	GLuint wall_texture, wall_normal_texture;

	// textures for position, color, and normal
	GLuint fb, depth_rb, FBOpos, FBOcol, FBOnorm;
	GLuint FBOvoxelize, FBOvoxelizeTex, FBOvoxelizeDepth;
	GLuint voxeltexture[7];
	GLuint CSmipmap;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our triangle to OpenGL
	GLuint VertexBufferID;

	GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;
    
    double mouse_posX, mouse_posY;
	int pass_number = 1;

	ssbo_data ssbo_CPUMEM;
	GLuint ssbo_GPU_id;

	bool show_shadowmap = false;

	int key_n = 0, key_m = 0;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
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

		if (key == GLFW_KEY_N && action == GLFW_PRESS)		key_n = 1;
		if (key == GLFW_KEY_N && action == GLFW_RELEASE)	key_n = 0;
		if (key == GLFW_KEY_M && action == GLFW_PRESS)		key_m = 1;
		if (key == GLFW_KEY_M && action == GLFW_RELEASE)	key_m = 0;
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


	//-------------------------------------------
	void init_voxelize_texture_fbo()
	{
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

		glBindTexture(GL_TEXTURE_2D, FBOvoxelizeTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//NULL means reserve texture memory, but texels are undefined
		//**** Tell OpenGL to reserve level 0
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		//You must reserve memory for other mipmaps levels as well either by making a series of calls to
		//glTexImage2D or use glGenerateMipmapEXT(GL_TEXTURE_2D).
		//Here, we'll use :
		glGenerateMipmap(GL_TEXTURE_2D);
		//-------------------------
		glGenFramebuffers(1, &FBOvoxelize);
		glBindFramebuffer(GL_FRAMEBUFFER, FBOvoxelize);
		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOvoxelizeTex, 0);
		//-------------------------
		glGenRenderbuffers(1, &FBOvoxelizeDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, FBOvoxelizeDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		//-------------------------
		//Attach depth buffer to FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, FBOvoxelizeDepth);
		//-------------------------
		//Does the GPU support current FBO configuration?
	//	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	//	switch (status)
	//	{
	//	case GL_FRAMEBUFFER_COMPLETE:
	//		cout << "status framebuffer: good" << std::endl;
	//		break;
	//	default:
	//		cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	//	}
	//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

        if (! prog_wall->init())
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

        if (! prog_mouse->init())
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
		prog_deferred->addAttribute("vertPos");
		prog_deferred->addAttribute("vertTex");

		/*
		// Initialize the GLSL program.
		voxprog = make_shared<Program>();
		voxprog->setVerbose(true);
		voxprog->setShaderNames(resourceDirectory + "/vox_vert.glsl", resourceDirectory + "/vox_frag.glsl");
		if (!voxprog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		voxprog->init();
		voxprog->addUniform("P");
		voxprog->addUniform("V");
		voxprog->addUniform("lightSpace");
		voxprog->addUniform("M");
		voxprog->addUniform("campos");
		voxprog->addUniform("lightpos");
		voxprog->addUniform("lightdir");
		voxprog->addAttribute("vertPos");
		voxprog->addAttribute("vertNor");
		voxprog->addAttribute("vertTex");
		*/

		prog2 = make_shared<Program>();
		prog2->setVerbose(true);
		prog2->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/frag_nolight.glsl");
		if (!prog2->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog2->init();
		prog2->addUniform("P");
		prog2->addUniform("V");
		prog2->addUniform("M");
		prog2->addAttribute("vertPos");
		prog2->addAttribute("vertTex");
		prog2->addUniform("manualmipmaplevel");
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
        
		int width, height, channels;
		char filepath[1000];

        //texture
        string str = resourceDirectory + "/lvl1.jpg";
        strcpy(filepath, str.c_str());
        unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_FLOAT, NULL);
		
		// Generate Position Texture
		glGenTextures(1, &FBOpos);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_FLOAT, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		// Generate Normal Texture
		glGenTextures(1, &FBOnorm);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_FLOAT, NULL);

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
		glUseProgram(prog_deferred->pid);

		glUniform1i(Tex1Loc, 0);
		glUniform1i(Tex2Loc, 1);
		glUniform1i(Tex3Loc, 2);


		GLuint Tex0Location = glGetUniformLocation(prog2->pid, "tex"); //tex sampler in the fragment shader
		Tex1Location = glGetUniformLocation(prog2->pid, "texnorm");
		Tex2Location = glGetUniformLocation(prog2->pid, "texpos");
		glUseProgram(prog2->pid);
		glUniform1i(Tex0Location, 0);
		glUniform1i(Tex1Location, 1);
		glUniform1i(Tex2Location, 2);

		/*
		glGenTextures(1, &FBOvoxelizeTex);
		init_voxelize_texture_fbo();
		*/

		GLenum status;
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			cout << "status framebuffer: good";
			break;
		default:
			cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!";
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

	void render_to_texture() // aka render to framebuffer
	{
		glfwGetCursorPos(windowManager->windowHandle, &mouse_posX, &mouse_posY);

		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
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
		T = glm::translate(glm::mat4(1), glm::vec3(0.5,0.9,0));
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

		// Save output to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, FBOcol);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	void render_to_screen()
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		auto P = std::make_shared<MatrixStack>();
		P->pushMatrix();
		P->perspective(70., width, height, 0.1, 100.0f);
		glm::mat4 M, V, S, T;

		V = glm::mat4(1);

		glm::vec3 mouse_pos = glm::vec3(mouse_posX, mouse_posY, 0);
		mouse_pos.x /= width;
		mouse_pos.y /= height;
		mouse_pos.x *= 2;
		mouse_pos.y *= 2;
		mouse_pos.x -= 1;
		mouse_pos.y -= 1;
		mouse_pos.y *= -1;

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		prog_mouse->bind();
		// MOUSE
		T = glm::translate(glm::mat4(1), mouse_pos);
		S = glm::scale(glm::mat4(1), glm::vec3(0.025, 0.05, 0.05));
		M = T * S;
		glUniformMatrix4fv(prog_mouse->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		mouse->draw(prog_mouse);
		prog_mouse->unbind();

		prog2->bind();
		glActiveTexture(GL_TEXTURE0);
		// Debug, shows shadow map when 'y' is pressed
		//show_shadowmap ? glBindTexture(GL_TEXTURE_2D, FBOpos) : glBindTexture(GL_TEXTURE_2D, FBOtex);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		
		/*
		glBindTextureUnit(3, voxeltexture[0]);
		glBindTextureUnit(4, voxeltexture[1]);
		glBindTextureUnit(5, voxeltexture[2]);
		glBindTextureUnit(6, voxeltexture[3]);
		glBindTextureUnit(7, voxeltexture[4]);
		glBindTextureUnit(8, voxeltexture[5]);
		glBindTextureUnit(9, voxeltexture[6]);
		//glBindImageTexture(2, voxeltexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
		*/

		M = glm::scale(glm::mat4(1), glm::vec3(1.2, 1, 1)) * glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5, -1));
		glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog2->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		static float mml = 0;
		if (key_n == 1)	mml -= 0.5;
		if (key_m == 1)	mml += 0.5;
		if (mml < 0)mml = 0;
		if (mml > 8)mml = 8;
		glUniform1f(prog2->getUniform("manualmipmaplevel"), mml);

		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		prog2->unbind();

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
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOcol);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		prog_deferred->unbind();

		if (pass_number == 1) {
			pass_number = 2;
		}
		else {
			pass_number = 1;
		}
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
	windowManager->init(1920, 1920);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);


	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);
	

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		application->create_SSBO();
		application->render_to_texture();
		application->render_to_screen();
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
