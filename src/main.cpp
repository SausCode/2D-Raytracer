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

class Application : public EventCallbacks
{

public:
 
	WindowManager * windowManager = nullptr;

	// Our shader program
    std::shared_ptr<Program> prog_wall, prog_mouse, prog_deferred;
 
	// Shape to be used (from obj file)
    shared_ptr<Shape> wall, mouse;
 
	//camera
	camera mycam;

	//texture for sim
	GLuint wall_texture, wall_normal_texture;

	// textures for position, color, and normal
	GLuint fb, depth_rb, FBOpos, FBOcol, FBOnorm;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our triangle to OpenGL
	GLuint VertexBufferID;

	GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;
    
    double mouse_posX, mouse_posY;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
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

        if (! prog_wall->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
        }

        prog_wall->init();
        prog_wall->addUniform("P");
        prog_wall->addUniform("V");
        prog_wall->addUniform("M");
        prog_wall->addUniform("campos");
        prog_wall->addAttribute("vertPos");
        prog_wall->addAttribute("vertNor");
        prog_wall->addAttribute("vertTex");
        prog_wall->addAttribute("vertTan");
        prog_wall->addAttribute("vertBinorm");
        
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
        prog_mouse->addUniform("P");
        prog_mouse->addUniform("V");
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
		prog_deferred->addUniform("P");
		prog_deferred->addUniform("V");
		prog_deferred->addUniform("M");
		prog_deferred->addUniform("campos");
		prog_deferred->addUniform("bloom");
		prog_deferred->addAttribute("vertPos");
		prog_deferred->addAttribute("vertNor");
		prog_deferred->addAttribute("vertTex");
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

		rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;

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
		glGenTextures(1, &FBOpos);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Generate Normal Texture
		glGenTextures(1, &FBOcol);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOcol);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_FLOAT, NULL);

		// Generate Position Texture
		glGenTextures(1, &FBOnorm);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_FLOAT, NULL);

		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOpos, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOcol , 0);
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

		int Tex1Loc = glGetUniformLocation(prog_deferred->pid, "pos_tex");
		int Tex2Loc = glGetUniformLocation(prog_deferred->pid, "col_tex");
		int Tex3Loc = glGetUniformLocation(prog_deferred->pid, "norm_tex");

		glUniform1i(Tex1Loc, 0);
		glUniform1i(Tex2Loc, 1);
		glUniform1i(Tex3Loc, 2);

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

    
	float map(float x, float in_min, float in_max, float out_min, float out_max)
	{
		return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
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


		float pihalf = 3.1415926 / 2.0;

		auto P = std::make_shared<MatrixStack>();
		P->pushMatrix();
		P->perspective(70., width, height, 0.1, 100.0f);
		glm::mat4 M, V, Rz, T, S, makeItBig, followTheCam, faceTheCam;
		V = mycam.process();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		prog_mouse->bind();
		glUniformMatrix4fv(prog_mouse->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));


		// MOUSE
		T = glm::translate(glm::mat4(1), glm::vec3((mouse_posX / width) * 5 - 2.5, (mouse_posY / height)*(-3) + 1.5, -3));
		S = glm::scale(glm::mat4(1), glm::vec3(0.1, 0.1, 0.1));
		M = T * S;
		glUniformMatrix4fv(prog_mouse->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniformMatrix4fv(prog_mouse->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		mouse->draw(prog_mouse);

		prog_mouse->unbind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, wall_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, wall_normal_texture);

		prog_wall->bind();
		glUniformMatrix4fv(prog_wall->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniform3fv(prog_wall->getUniform("campos"), 1, &mycam.pos.x);
		faceTheCam = glm::rotate(glm::mat4(1), -mycam.rot.y, glm::vec3(0, 1, 0));

		// WALLS
		//topleftwall
		T = glm::translate(glm::mat4(1), glm::vec3(-1.17, 1.25, -3));
		S = glm::scale(glm::mat4(1), glm::vec3(1.5, 0.2, 1));
		M = T * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniformMatrix4fv(prog_wall->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		wall->draw(prog_wall);

		//toprightwall
		T = glm::translate(glm::mat4(1), glm::vec3(1.17, 1.25, -3));
		M = T * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

		//sidewall
		S = glm::scale(glm::mat4(1), glm::vec3(1.05, 0.25, 1));
		T = glm::translate(glm::mat4(1), glm::vec3(2.41, 0, -3));
		Rz = glm::rotate(glm::mat4(1), pihalf, glm::vec3(0, 0, -1));
		M = T * Rz * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

		//bottomleftwall
		S = glm::scale(glm::mat4(1), glm::vec3(1.5, 0.2, 1));
		T = glm::translate(glm::mat4(1), glm::vec3(-1.17, -1.25, -3));
		Rz = glm::rotate(glm::mat4(1), pihalf * 2, glm::vec3(0, 0, 1));
		M = T * Rz * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

		//bottomrightwall
		S = glm::scale(glm::mat4(1), glm::vec3(1.5, 0.2, 1));
		T = glm::translate(glm::mat4(1), glm::vec3(1.17, -1.25, -3));
		Rz = glm::rotate(glm::mat4(1), pihalf * 2, glm::vec3(0, 0, 1));
		M = T * Rz * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

		//midwall
		T = glm::translate(glm::mat4(1), glm::vec3(-1.17, 0, -3));
		S = glm::scale(glm::mat4(1), glm::vec3(1.5, 0.2, 1));
		M = T * S;
		glUniformMatrix4fv(prog_wall->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		wall->draw(prog_wall);

		//done, unbind stuff
		prog_wall->unbind();

		//Save output to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOcol);
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

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		prog_deferred->bind();

		glUniform3fv(prog_deferred->getUniform("campos"), 1, &mycam.pos.x);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOpos);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, FBOcol);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, FBOnorm);

		M = glm::scale(glm::mat4(1), glm::vec3(1, 1, 1));
		T = glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5, -1));
		M = M * T;
		glUniformMatrix4fv(prog_deferred->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog_deferred->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog_deferred->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		prog_deferred->unbind();
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


	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		application->render_to_texture();
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
