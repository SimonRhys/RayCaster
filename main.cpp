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

struct cube {
	glm::vec4 cubeMin;
	glm::vec4 cubeMax;
};

bool KEYS[1024];
float AVG_DT = 0;
bool CUBE_TESTING = false;
bool MODEL_TESTING = false;
std::ofstream OUTPUT_FILE;
std::vector<Texture> texturesLoaded;

cube* generateCubeData(int numCubes)
{
	if (numCubes == 0)
	{
		return new cube[1];
	}
	cube *cubes = new cube[numCubes];
	float nearestSqrt = sqrt(numCubes);
	nearestSqrt = (int)nearestSqrt;
	nearestSqrt++;

	int count = 0;

	for (int i = 0; i < nearestSqrt; i++)
	{
		for (int j = 0; j < nearestSqrt; j++)
		{
			cubes[count++] = { glm::vec4(5 + i*15, 0 + j * 15, 5, 1), glm::vec4(10 + i*15, 5 + j * 15, 10, 1) };
			if (count == numCubes)
			{
				return cubes;
			}
		}
	}
	
	return cubes;
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

glm::vec3 getCubeCentre(cube c)
{
	glm::vec4 diff = c.cubeMin+(c.cubeMax - c.cubeMin)*0.5f;
	glm::vec3 result = glm::vec3(diff.x, diff.y, diff.z);

	return result;
}

std::string getConfigValue(std::string line, std::string config)
{
	std::size_t found = line.find(config);
	if (found == std::string::npos)
	{
		return "";
	}

	found = line.find("=");
	if (found == std::string::npos)
	{
		return "";
	}

	return line.substr(found + 1);
}

void loadConfig(GLuint *w, GLuint *h, std::string *modelPath, int *numCubes, bool *useQuadtree)
{
	std::ifstream configFile;
	configFile.open("config.txt");
	if (!configFile.is_open())
	{
		std::cout << "ERROR OPENING CONFIG" << std::endl;
		return;
	}

	std::string line;
	while (getline(configFile, line))
	{
		std::string value = getConfigValue(line, "width");
		if (value != "") 
		{
			*w = stoi(value);
		}

		value = getConfigValue(line, "height");
		if (value != "")
		{
			*h = stoi(value);
		}

		value = getConfigValue(line, "modelPath");
		if (value != "")
		{
			*modelPath = value;
		}

		value = getConfigValue(line, "numCubes");
		if (value != "")
		{
			*numCubes = stoi(value);
		}

		value = getConfigValue(line, "testing");
		if(value != "")
		{
			if (value == "cube")
			{
				CUBE_TESTING = true;
			}

			if (value == "model")
			{
				MODEL_TESTING = true;
			}
		}

		value = getConfigValue(line, "useQuadtree");
		if (value == "true")
		{
			*useQuadtree = true;
		}
	}

	configFile.close();
}

int main()
{
	//Config Values
	GLuint WIDTH = 512, HEIGHT = 384;
	std::string modelPath = "";
	int NUM_CUBES = 0;
	bool useQuadtree = false;

	loadConfig(&WIDTH, &HEIGHT, &modelPath, &NUM_CUBES, &useQuadtree);

	if (CUBE_TESTING || MODEL_TESTING)
	{
		OUTPUT_FILE.open("output.csv");
	}

	//Init GLFW
	glfwInit();
	//Set all the required options for GLFW
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	//Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	//Set the required callback functions
	glfwSetKeyCallback(window, key_callback);

	glfwShowWindow(window);

	//Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW\n";
		return -1;
	}

	Model model(modelPath);
	std::vector<Tri> modelTriangles = model.getModelTris();

	//Define the viewport dimensions
	glViewport(0, 0, WIDTH, HEIGHT);

	GLuint tex = createFramebufferTexture(WIDTH, HEIGHT);
	GLuint vao = quadFullScreenVAO();

	//Setup compute program
	Shader computeProgram;
	computeProgram.createShader("compute.csh", GL_COMPUTE_SHADER);
	computeProgram.createProgram();

	glUseProgram(computeProgram.getShaderProgram());
	GLint workGroupSize[3];
	glGetProgramiv(computeProgram.getShaderProgram(), GL_COMPUTE_WORK_GROUP_SIZE, workGroupSize);
	GLint workGroupSizeX = workGroupSize[0];
	GLint workGroupSizeY = workGroupSize[1];

	//Setup the uniforms needed for the compute shader
	GLuint eyeUniform = glGetUniformLocation(computeProgram.getShaderProgram(), "eye");
	GLuint ray00Uniform = glGetUniformLocation(computeProgram.getShaderProgram(), "ray00");
	GLuint ray10Uniform = glGetUniformLocation(computeProgram.getShaderProgram(), "ray10");
	GLuint ray01Uniform = glGetUniformLocation(computeProgram.getShaderProgram(), "ray01");
	GLuint ray11Uniform = glGetUniformLocation(computeProgram.getShaderProgram(), "ray11");
	GLuint lightPosUniform = glGetUniformLocation(computeProgram.getShaderProgram(), "lightPos");
	GLuint numCubesUniform = glGetUniformLocation(computeProgram.getShaderProgram(), "NUM_CUBES");
	GLuint numTriUniform = glGetUniformLocation(computeProgram.getShaderProgram(), "NUM_TRIANGLES");

	cube *cubes = new cube[NUM_CUBES + 1];
	cubes = generateCubeData(NUM_CUBES);

	if (CUBE_TESTING)
	{
		NUM_CUBES = 100;
		delete[] cubes;
		cubes = generateCubeData(NUM_CUBES);
	}

	Quadtree<cube> quad(glm::vec2(50, 50), glm::vec2(100, 100));
	if (useQuadtree)
	{
		for (int i = 0; i < NUM_CUBES; i++)
		{
			glm::vec3 centre = getCubeCentre(cubes[i]);
			quad.insert(cubes[i], glm::vec2(centre.x, centre.y));
		}
	}


	//Setup Cube Shader Buffer
	GLuint cubeShaderBuffer;
	glGenBuffers(1, &cubeShaderBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(cube)*NUM_CUBES, &cubes[0], GL_STATIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
	GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	memcpy(p, &cubes[0], sizeof(cube)*NUM_CUBES);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	GLuint blockIndex;
	blockIndex = glGetProgramResourceIndex(computeProgram.getShaderProgram(), GL_SHADER_STORAGE_BLOCK, "cubes");
	glShaderStorageBlockBinding(computeProgram.getShaderProgram(), blockIndex, 2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cubeShaderBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cubeShaderBuffer);

	//Setup Triangle Shader Buffer
	GLuint triShaderBuffer;
	glGenBuffers(1, &triShaderBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triShaderBuffer);
	if (modelTriangles.size() != 0)
	{
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Tri)*modelTriangles.size(), &modelTriangles[0], GL_STATIC_COPY);
	}
	else
	{
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Tri)*modelTriangles.size(), nullptr, GL_STATIC_COPY);
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triShaderBuffer);
	p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	if (modelTriangles.size() != 0)
	{
		memcpy(p, &modelTriangles[0], sizeof(Tri)*modelTriangles.size());
	}
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

	//Output scene information to console
	std::cout << "Resolution: Width=" << WIDTH << " Height=" << HEIGHT << std::endl;

	if (NUM_CUBES > 0)
	{
		std::cout << "Number of cubes: " << NUM_CUBES << std::endl;
	}

	if (MODEL_TESTING)
	{
		std::cout << "TESTING MODE: MODEL" << std::endl;
	}

	if (CUBE_TESTING)
	{
		std::cout << "TESTING MODE: CUBE" << std::endl;
	}

	if (modelTriangles.size() > 0)
	{
		std::cout << "Model polygon count: " << modelTriangles.size() << std::endl;
	}

	
	//Window loop
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
			
			if (CUBE_TESTING)
			{
				NUM_CUBES+=100;
				delete[] cubes;
				cubes = generateCubeData(NUM_CUBES);

				glUseProgram(computeProgram.getShaderProgram());
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(cube)*NUM_CUBES, &cubes[0], GL_STATIC_COPY);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

				glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
				p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
				memcpy(p, &cubes[0], sizeof(cube)*NUM_CUBES);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				glUseProgram(0);

				float fps = 1 / AVG_DT;
				std::cout << NUM_CUBES << " cubes at " << fps << " fps" << std::endl;
				OUTPUT_FILE << fps << ", " << NUM_CUBES << "\n";
				if (fps <= 10 || NUM_CUBES > 10000)
				{
					glfwSetWindowShouldClose(window, 1);
				}

				if (useQuadtree)
				{
					cube firstCube = cubes[0];
					cube lastCube = cubes[NUM_CUBES - 1];
					glm::vec4 distance = lastCube.cubeMax - firstCube.cubeMin;
					glm::vec4 centre = distance*0.5f;
					quad = Quadtree<cube>(glm::vec2(centre.x, centre.y), glm::vec2(distance.x, distance.y));
					for (int i = 0; i < NUM_CUBES; i++)
					{
						glm::vec3 centre = getCubeCentre(cubes[i]);
						quad.insert(cubes[i], glm::vec2(centre.x, centre.y));
					}
				}
			}
		}

		//Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		glViewport(0, 0, WIDTH, HEIGHT);
		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		VP = handleControls(&camera, &currentAngle, projection, VP, dt);

		glm::mat4 inverseVP = glm::inverse(VP);
		glm::vec4 eyeRay;
		glm::vec2 topLeftCorner;
		glm::vec2 botRightCorner;

		glUseProgram(computeProgram.getShaderProgram());

		//Set viewing frustum corner rays in shader
		glUniform3f(eyeUniform, camera.x, camera.y, camera.z);

		eyeRay = calculateEyeRay(glm::vec4(-1, -1, 0, 1), camera, inverseVP);
		glUniform3f(ray00Uniform, eyeRay.x, eyeRay.y, eyeRay.z);
		topLeftCorner = glm::vec2(eyeRay.x, eyeRay.y);

		eyeRay = calculateEyeRay(glm::vec4(-1, 1, 0, 1), camera, inverseVP);
		glUniform3f(ray01Uniform, eyeRay.x, eyeRay.y, eyeRay.z);

		eyeRay = calculateEyeRay(glm::vec4(1, -1, 0, 1), camera, inverseVP);
		glUniform3f(ray10Uniform, eyeRay.x, eyeRay.y, eyeRay.z);

		eyeRay = calculateEyeRay(glm::vec4(1, 1, 0, 1), camera, inverseVP);
		glUniform3f(ray11Uniform, eyeRay.x, eyeRay.y, eyeRay.z);
		botRightCorner = glm::vec2(eyeRay.x, eyeRay.y);

		glUniform3f(lightPosUniform, 5, 5, 5);
		glUniform1i(numCubesUniform, NUM_CUBES);
		glUniform1i(numTriUniform, modelTriangles.size());

		if (useQuadtree)
		{
			topLeftCorner = topLeftCorner*1000.f;
			botRightCorner = botRightCorner*1000.f;

			glm::vec2 diff = glm::abs(botRightCorner - topLeftCorner);
			glm::vec2 mid = glm::min(topLeftCorner, botRightCorner) + (diff*0.5f);
			std::vector<cube> vec = quad.search(glm::vec2(mid.x, mid.y), glm::vec2(diff.x, diff.y));

			GLvoid *data;

			if (vec.size() == 0)
			{
				data = nullptr;
			}
			else
			{
				data = &vec[0];
			}

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(cube)*vec.size(), data, GL_STATIC_COPY);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubeShaderBuffer);
			p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
			memcpy(p, data, sizeof(cube)*vec.size());
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glUniform1i(numCubesUniform, vec.size());
		}



		//Bind framebuffer texture to image unit 0 as writable image in the shader.
		glBindImageTexture(0, tex, 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);
		//Bind model texture to image unit 1 as readable image in the shader
		if(model.hasTexture())
		{
			glBindImageTexture(1, model.getTextures()[0].id, 0, false, 0, GL_READ_ONLY, GL_RGBA32F);
		}


		//Compute appropriate invocation dimension. 
		int worksizeX = nextPowerOfTwo(WIDTH);
		int worksizeY = nextPowerOfTwo(HEIGHT);

		//Invoke the compute shader. 
		glDispatchCompute(worksizeX / workGroupSizeX, worksizeY / workGroupSizeY, 1);

		//Reset image binding. 
		glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, 0, 0, false, 0, GL_READ_ONLY, GL_RGBA32F);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glUseProgram(0);

		//Draw the rendered image on the screen using textured full-screen
		//quad.
		glUseProgram(quadProgram.getShaderProgram());
		glBindVertexArray(vao);
		glBindTexture(GL_TEXTURE_2D, tex);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		//Swap the screen buffers
		glfwSwapBuffers(window);
	}
	//Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &vao);
	//Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	delete[] cubes;
	modelTriangles.clear();

	if (CUBE_TESTING || MODEL_TESTING)
	{
		OUTPUT_FILE.close();
	}
	return 0;
}