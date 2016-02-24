#include <iostream>
#include <math.h> 

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"

#define PI 3.14159265358979323846

bool KEYS[1024];

GLuint createFramebufferTexture(GLuint width, GLuint height)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}


/**
* Create the frame buffer object that our ray tracing shader uses to render
* into the framebuffer texture.
*
* @return the FBO id
*/
GLuint createFrameBufferObject(GLuint tex)
{
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	int fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Could not create FBO: " + fboStatus << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return fbo;
}

/**
* Creates a VAO with a full-screen quad VBO.
*/
GLuint quadFullScreenVAO()
{
	GLuint vao;
	GLuint vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLbyte bufferData[12];
	bufferData[0] = -1;
	bufferData[1] = -1;

	bufferData[2] = 1;
	bufferData[3] = -1;

	bufferData[4] = 1;
	bufferData[5] = 1;

	bufferData[6] = 1;
	bufferData[7] = 1;

	bufferData[8] = -1;
	bufferData[9] = 1;

	bufferData[10] = -1;
	bufferData[11] = -1;

	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLbyte), bufferData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_BYTE, false, 0, 0L);
	glBindVertexArray(0);


	return vao;
}

int nextPowerOfTwo(int x)
{
	x--;
	x |= x >> 1; // handle 2 bit numbers
	x |= x >> 2; // handle 4 bit numbers
	x |= x >> 4; // handle 8 bit numbers
	x |= x >> 8; // handle 16 bit numbers
	x |= x >> 16; // handle 32 bit numbers
	x++;
	return x;
}

glm::vec4 calculateEyeRay(glm::vec4 eyeRay, glm::vec3 cameraPos, glm::mat4 inverseVP)
{
	glm::vec4 result = inverseVP * eyeRay;

	result = result * (1 / result.w);
	result -= glm::vec4(cameraPos, 1);

	return result;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	else if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			KEYS[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			KEYS[key] = false;
		}
	}
}

glm::mat4 handleControls(glm::vec3 *pos, float *currentAngle, glm::mat4 projection, glm::mat4 vp, float dt)
{

	const float SPEED = 10;
	const float ROTATE_SPEED = 10;
	bool viewChanged = false;

	if (KEYS[GLFW_KEY_W])
	{
		viewChanged = true;

		pos->x += SPEED*dt*sin(*currentAngle + PI);
		pos->z += SPEED*dt*cos(*currentAngle + PI);
	}
	else if (KEYS[GLFW_KEY_S])
	{
		viewChanged = true;

		pos->x += -SPEED*dt*sin(*currentAngle + PI);
		pos->z += -SPEED*dt*cos(*currentAngle + PI);
	}

	if (KEYS[GLFW_KEY_A])
	{
		viewChanged = true;

		pos->x += SPEED*dt*sin(*currentAngle);
		pos->z += SPEED*dt*cos(*currentAngle);
	}
	else if (KEYS[GLFW_KEY_D])
	{
		viewChanged = true;

		pos->x += -SPEED*dt*sin(*currentAngle);
		pos->z += -SPEED*dt*cos(*currentAngle);
	}

	if (KEYS[GLFW_KEY_LEFT])
	{
		viewChanged = true;

		float angle = -PI*ROTATE_SPEED*dt;
		*currentAngle += angle;

		if (*currentAngle >= 2 * PI || *currentAngle <= -2 * PI)
		{
			//*currentAngle = 0;
		}
	}
	else if (KEYS[GLFW_KEY_RIGHT])
	{
		viewChanged = true;

		float angle = PI*ROTATE_SPEED*dt;
		*currentAngle += angle;

		if (*currentAngle >= 2 * PI || *currentAngle <= -2 * PI)
		{
			//*currentAngle = 0;
		}
	}

	if (KEYS[GLFW_KEY_SPACE])
	{
		viewChanged = true;

		pos->y -= SPEED*dt;
	}
	else if (KEYS[GLFW_KEY_LEFT_SHIFT])
	{
		viewChanged = true;

		pos->y += SPEED*dt;
	}

	if (viewChanged)
	{
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0), *currentAngle, glm::vec3(0, 1, 0));
		glm::mat4 translation = glm::translate(glm::mat4(1.0), *pos);
		glm::mat4 view = rotation * translation;
		return projection * view;
	}
	else
	{
		return vp;
	}
}

void printMatrix(glm::mat4 m)
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			std::cout << m[i][j] << " ";
		}
		std::cout << std::endl;
	}
}

// The MAIN function, from here we start the application and run the game loop
int main()
{
	// Window dimensions
	const GLuint WIDTH = 1024, HEIGHT = 768;

	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
		
	glfwShowWindow(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW\n";
		return -1;
	}

	// Define the viewport dimensions
	glViewport(0, 0, WIDTH, HEIGHT);

	GLuint tex = createFramebufferTexture(WIDTH, HEIGHT);
	GLuint vao = quadFullScreenVAO();


	//Setup compute program
	Shader computeProgram;
	computeProgram.createShader("demo01.glslcs", GL_COMPUTE_SHADER);
	computeProgram.createProgram();

	glUseProgram(computeProgram.getShaderProgram());
	GLint workGroupSize[3];
	glGetProgramiv(computeProgram.getShaderProgram(), GL_COMPUTE_WORK_GROUP_SIZE, workGroupSize);
	GLint workGroupSizeX = workGroupSize[0];
	GLint workGroupSizeY = workGroupSize[1];

	GLuint eyeUniform = glGetUniformLocation(computeProgram.getShaderProgram(), "eye");
	GLuint ray00Uniform = glGetUniformLocation(computeProgram.getShaderProgram(), "ray00");
	GLuint ray10Uniform = glGetUniformLocation(computeProgram.getShaderProgram(), "ray10");
	GLuint ray01Uniform = glGetUniformLocation(computeProgram.getShaderProgram(), "ray01");
	GLuint ray11Uniform = glGetUniformLocation(computeProgram.getShaderProgram(), "ray11");
	glUseProgram(0);

	//Setup drawing program
	Shader quadProgram;
	quadProgram.createShader("quad.vs", GL_VERTEX_SHADER);
	quadProgram.createShader("quad.fs", GL_FRAGMENT_SHADER);
	quadProgram.createProgram();

	glBindAttribLocation(quadProgram.getShaderProgram(), 0, "vertex");
	glBindFragDataLocation(quadProgram.getShaderProgram(), 0, "color");


	//Initialise Quad Program
	glUseProgram(quadProgram.getShaderProgram());
	GLuint texUniform = glGetUniformLocation(quadProgram.getShaderProgram(), "tex");
	glUniform1i(texUniform, 0);
	glUseProgram(0);

	glm::vec3 camera = glm::vec3(3.0f, 2.0f, 7.0f);
	float currentAngle = glm::radians(23.f);

	glm::mat4 projection = glm::perspective(60.0f, (float)WIDTH / HEIGHT, 1.f, 2.f);
	glm::mat4 t = glm::translate(glm::mat4(1.0f), -camera);
	glm::mat4 r = glm::rotate(glm::mat4(1.0f), currentAngle, glm::vec3(0, 1, 0));
	glm::mat4 view = r * t;
	glm::mat4 VP = projection * view;

	//printMatrix(r * t);
	//std::cout << std::endl;
	//printMatrix(view);



	GLfloat lastFrame = glfwGetTime();
	GLfloat dt = glfwGetTime();
	// Game loop
	while (!glfwWindowShouldClose(window))
	{

		GLfloat currentFrame = glfwGetTime();
		dt = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		glViewport(0, 0, WIDTH, HEIGHT);
		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		/*currentAngle += glm::radians(1.f);

		t = glm::translate(glm::mat4(1.0f), -camera);
		r = glm::rotate(glm::mat4(1.0f), currentAngle, glm::vec3(0, 1, 0));
		view = r * t;
		VP = projection * view;*/

		VP = handleControls(&camera, &currentAngle, projection, VP, dt);

		glm::mat4 inverseVP = glm::inverse(VP);
		glm::vec4 eyeRay;

		glUseProgram(computeProgram.getShaderProgram());

		/* Set viewing frustum corner rays in shader */
		glUniform3f(eyeUniform, camera.x, camera.y, camera.z);

		eyeRay = calculateEyeRay(glm::vec4(-1, -1, 0, 1), camera, inverseVP);
		glUniform3f(ray00Uniform, eyeRay.x, eyeRay.y, eyeRay.z);

		eyeRay = calculateEyeRay(glm::vec4(-1, 1, 0, 1), camera, inverseVP);
		glUniform3f(ray01Uniform, eyeRay.x, eyeRay.y, eyeRay.z);

		eyeRay = calculateEyeRay(glm::vec4(1, -1, 0, 1), camera, inverseVP);
		glUniform3f(ray10Uniform, eyeRay.x, eyeRay.y, eyeRay.z);

		eyeRay = calculateEyeRay(glm::vec4(1, 1, 0, 1), camera, inverseVP);
		glUniform3f(ray11Uniform, eyeRay.x, eyeRay.y, eyeRay.z);

		/* Bind level 0 of framebuffer texture as writable image in the shader. */
		glBindImageTexture(0, tex, 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);

		/* Compute appropriate invocation dimension. */
		int worksizeX = nextPowerOfTwo(WIDTH);
		int worksizeY = nextPowerOfTwo(HEIGHT);

		/* Invoke the compute shader. */
		glDispatchCompute(worksizeX / workGroupSizeX, worksizeY / workGroupSizeY, 1);

		/* Reset image binding. */
		glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glUseProgram(0);

		/*
		* Draw the rendered image on the screen using textured full-screen
		* quad.
		*/
		glUseProgram(quadProgram.getShaderProgram());
		glBindVertexArray(vao);
		glBindTexture(GL_TEXTURE_2D, tex);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}
	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &vao);
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}