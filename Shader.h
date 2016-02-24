#pragma once

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

class Shader
{

public:
	Shader();
	~Shader();

	GLuint getShaderProgram();

	void createShader(const char* shaderPath, int shaderType);
	void createProgram();

protected:
	GLuint shaderProgram;
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint computeShader;

	bool vertexShaderSet;
	bool fragmentShaderSet;
	bool computeShaderSet;

};

