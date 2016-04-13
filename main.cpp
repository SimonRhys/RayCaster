#include <iostream>
#include <fstream>
#include <math.h> 

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//SOIL
#include <SOIL/SOIL.h>

#include "Shader.h"
#include "Quadtree.h"
#include "Model.h"

#define PI 3.14159265358979323846

struct box {
	glm::vec4 boxMin;
	glm::vec4 boxMax;
};

bool KEYS[1024];
float AVG_DT = 0;
bool TESTING = false;
std::ofstream OUTPUT_FILE;
std::vector<Texture> texturesLoaded;

box* generateTestData(int numBoxes)
{

	box *boxes = new box[numBoxes];
	float nearestSqrt = sqrt(numBoxes);
	nearestSqrt = (int)nearestSqrt;
	nearestSqrt++;

	int count = 0;

	for (int i = 0; i < nearestSqrt; i++)
	{
		for (int j = 0; j < nearestSqrt; j++)
		{
			//boxes[count++] = { glm::vec4(-0.5 + i, -0.5, -0.5 + j, 1), glm::vec4(0.5 + i, 0.5, 0.5 + j, 1) };
			boxes[count++] = { glm::vec4(5 + i*5, 5, 5 + j*5, 1), glm::vec4(10 + i*5, 10, 10 + j*5, 1) };
			if (count == numBoxes)
			{
				return boxes;
			}
		}
	}
	
	return boxes;
}

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
	const float ROTATE_SPEED = 1;
	bool viewChanged = false;

	if (KEYS[GLFW_KEY_F])
	{
		std::cout << "Delta Time: " << AVG_DT*1000 << "ms" << std::endl;
		KEYS[GLFW_KEY_F] = false;
	}

	if (KEYS[GLFW_KEY_W])
	{
		viewChanged = true;

		pos->x += -SPEED*dt*sin(*currentAngle);
		pos->z += -SPEED*dt*cos(*currentAngle);
	}
	else if (KEYS[GLFW_KEY_S])
	{
		viewChanged = true;

		pos->x += SPEED*dt*sin(*currentAngle);
		pos->z += SPEED*dt*cos(*currentAngle);
	}

	if (KEYS[GLFW_KEY_A])
	{
		viewChanged = true;

		pos->x += -SPEED*dt*sin(*currentAngle + PI/2);
		pos->z += -SPEED*dt*cos(*currentAngle + PI/2);
	}
	else if (KEYS[GLFW_KEY_D])
	{
		viewChanged = true;

		pos->x += SPEED*dt*sin(*currentAngle + PI/2);
		pos->z += SPEED*dt*cos(*currentAngle + PI/2);
	}

	if (KEYS[GLFW_KEY_LEFT])
	{
		viewChanged = true;

		float angle = PI*ROTATE_SPEED*dt;
		*currentAngle += angle;

		if (*currentAngle >= 2 * PI || *currentAngle <= -2 * PI)
		{
			*currentAngle = 0;
		}
	}
	else if (KEYS[GLFW_KEY_RIGHT])
	{
		viewChanged = true;

		float angle = -PI*ROTATE_SPEED*dt;
		*currentAngle += angle;

		if (*currentAngle >= 2 * PI || *currentAngle <= -2 * PI)
		{
			*currentAngle = 0;
		}
	}

	if (KEYS[GLFW_KEY_SPACE])
	{
		viewChanged = true;

		pos->y += SPEED*dt;
	}
	else if (KEYS[GLFW_KEY_LEFT_SHIFT])
	{
		viewChanged = true;

		pos->y -= SPEED*dt;
	}

	if (viewChanged)
	{
		glm::mat4 rotation = glm::rotate(glm::mat4(1.0), -*currentAngle, glm::vec3(0, 1, 0));
		glm::mat4 translation = glm::translate(glm::mat4(1.0), -*pos);
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

glm::vec3 getBoxCentre(box b)
{
	glm::vec4 diff = b.boxMax - b.boxMin;
	glm::vec3 result = glm::vec3(diff.x, diff.y, diff.z);

	return result;
}

// The MAIN function, from here we start the application and run the game loop
int main()
{
	Model model("obj_files/Rock1/Rock1.obj");
	std::vector<Tri> modelTriangles = model.getModelTris();

	if (TESTING)
	{
		OUTPUT_FILE.open("output.csv");
	}

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
	GLuint lightPosUniform = glGetUniformLocation(computeProgram.getShaderProgram(), "lightPos");
	GLuint numBoxesUniform = glGetUniformLocation(computeProgram.getShaderProgram(), "NUM_BOXES");
	GLuint numTriUniform = glGetUniformLocation(computeProgram.getShaderProgram(), "NUM_TRIANGLES");

	int NUM_BOXES = 1;
	
	box *boxes = new box[1];
	boxes = generateTestData(1);

	/*Quadtree<box> root(glm::vec2(50, 50), glm::vec2(100, 100));

	for (int i = 0; i < 10; i++)
	{
		glm::vec3 cent = getBoxCentre(boxes[i]);
		std::cout << root.insert(boxes[i], glm::vec2(cent.x, cent.z)) << std::endl;
	}

	std::vector<box> res = root.search(glm::vec2(50, 50), glm::vec2(100, 100));

	std::cout << "FOUND " << res.size() << " BOXES: " << std::endl;
	for (int i = 0; i < res.size(); i++)
	{
		std::cout << "x = " << boxes[i].boxMin.y << " y = " << boxes[i].boxMax.z << std::endl;
	}*/

	for (int i = 0; i < NUM_BOXES; i++)
	{
		//boxes[i] = { glm::vec4(-5.0, -0.1 + i, -5.0, 1), glm::vec4(5.0, 0.0 + i, 5.0, 1) };
	}

	if (TESTING)
	{
		NUM_BOXES = 1;
		delete[] boxes;
		boxes = generateTestData(NUM_BOXES);
	}
	
	//Setup Cube Shader Buffer
	GLuint cubeShaderBuffer;
	glGenBuffers(1, &cubeShaderBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(box)*NUM_BOXES, &boxes[0], GL_STATIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
	GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	memcpy(p, &boxes[0], sizeof(box)*NUM_BOXES);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	GLuint blockIndex;
	blockIndex = glGetProgramResourceIndex(computeProgram.getShaderProgram(), GL_SHADER_STORAGE_BLOCK, "boxes");
	glShaderStorageBlockBinding(computeProgram.getShaderProgram(), blockIndex, 2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cubeShaderBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cubeShaderBuffer);

	//Setup Triangle Shader Buffer
	GLuint triShaderBuffer;
	glGenBuffers(1, &triShaderBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triShaderBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Tri)*modelTriangles.size(), &modelTriangles[0], GL_STATIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triShaderBuffer);
	p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	memcpy(p, &modelTriangles[0], sizeof(Tri)*modelTriangles.size());
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	blockIndex = glGetProgramResourceIndex(computeProgram.getShaderProgram(), GL_SHADER_STORAGE_BLOCK, "triangles");
	glShaderStorageBlockBinding(computeProgram.getShaderProgram(), blockIndex, 3);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, triShaderBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, triShaderBuffer);

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

	GLuint modelTexUniform = glGetUniformLocation(quadProgram.getShaderProgram(), "modelTex");
	glUniform1i(modelTexUniform, 1);
	glUseProgram(0);

	glm::vec3 camera = glm::vec3(0.0f, 0.0f, 7.0f);
	float currentAngle = glm::radians(0.f);

	glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)WIDTH / HEIGHT, 1.f, 2.f);
	glm::mat4 t = glm::translate(glm::mat4(1.0f), -camera);
	glm::mat4 r = glm::rotate(glm::mat4(1.0f), -currentAngle, glm::vec3(0, 1, 0));
	glm::mat4 view = r * t;
	glm::mat4 VP = projection * view;

	GLfloat lastFrame = glfwGetTime();
	GLfloat dt = glfwGetTime();

	float totalDT = 0;
	int frameNum = 0;

	// Game loop
	while (!glfwWindowShouldClose(window))
	{

		GLfloat currentFrame = glfwGetTime();
		dt = currentFrame - lastFrame;
		lastFrame = currentFrame;

		totalDT += dt;
		frameNum++;

		if (frameNum == 100)
		{
			AVG_DT = totalDT / 100;
			frameNum = 0;
			totalDT = 0;
			
			if (TESTING)
			{
				NUM_BOXES++;
				delete[] boxes;
				boxes = generateTestData(NUM_BOXES);

				glUseProgram(computeProgram.getShaderProgram());
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(box)*NUM_BOXES, &boxes[0], GL_STATIC_COPY);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

				glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
				p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
				memcpy(p, &boxes[0], sizeof(box)*NUM_BOXES);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				glUseProgram(0);

				float fps = 1 / AVG_DT;
				std::cout << NUM_BOXES << " boxes at " << fps << " fps" << std::endl;
				OUTPUT_FILE << fps << ", " << NUM_BOXES << "\n";
				if (fps <= 10)
				{
					glfwSetWindowShouldClose(window, 1);
				}
			}
		}

		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		glViewport(0, 0, WIDTH, HEIGHT);
		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

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

		glUniform3f(lightPosUniform, 5, 5, 5);

		glUniform1i(numBoxesUniform, NUM_BOXES);
		glUniform1i(numTriUniform, modelTriangles.size());

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

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, model.getTextures()[0].id);
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
	delete[] boxes;

	if (TESTING)
	{
		OUTPUT_FILE.close();
	}
	return 0;
}