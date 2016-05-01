#include "Shader.h"



Shader::Shader()
{
	vertexShaderSet = false;
	fragmentShaderSet = false;
	computeShaderSet = false;
	shaderProgram = glCreateProgram();
}

GLuint Shader::getShaderProgram()
{
	return shaderProgram;
}

void Shader::createShader(const char* shaderPath, int shaderType)
{
	// Create the shaders
	GLuint shaderID = glCreateShader(shaderType);

	// Read the shader code from the file
	std::string shaderCode;
	std::ifstream shaderStream(shaderPath, std::ios::in);
	if (shaderStream.is_open())
	{
		std::string line = "";
		while (getline(shaderStream, line))
			shaderCode += "\n" + line;
		shaderStream.close();
	}

	GLint result = GL_FALSE;
	int infoLogLength;

	// Compile shader
	printf("Compiling shader : %s\n", shaderPath);
	char const * sourcePointer = shaderCode.c_str();
	glShaderSource(shaderID, 1, &sourcePointer, NULL);
	glCompileShader(shaderID);

	// Check shader
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	std::vector<char> shaderErrorMessage(infoLogLength);
	if (shaderErrorMessage.size() > 0) 
	{
		glGetShaderInfoLog(shaderID, infoLogLength, NULL, &shaderErrorMessage[0]);
		fprintf(stdout, "%s\n", &shaderErrorMessage[0]);
	}

	if (shaderType == GL_VERTEX_SHADER)
	{
		vertexShaderSet = true;
		vertexShader = shaderID;
	}
	else if (shaderType == GL_FRAGMENT_SHADER)
	{
		fragmentShaderSet = true;
		fragmentShader = shaderID;
	}
	else if (shaderType == GL_COMPUTE_SHADER)
	{
		computeShaderSet = true;
		computeShader = shaderID;
	}
}

void Shader::createProgram()
{

	// Link the program
	fprintf(stdout, "Linking program\n");
	if (vertexShaderSet)
	{
		glAttachShader(shaderProgram, vertexShader);
	}

	if (fragmentShaderSet)
	{
		glAttachShader(shaderProgram, fragmentShader);
	}

	if (computeShaderSet)
	{
		glAttachShader(shaderProgram, computeShader);
	}
	glLinkProgram(shaderProgram);

	// Check the program
	GLint result = GL_FALSE;
	int infoLogLength;

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &result);
	glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infoLogLength);
	std::vector<char> programErrorMessage(std::max(infoLogLength, int(1)));
	glGetProgramInfoLog(shaderProgram, infoLogLength, NULL, &programErrorMessage[0]);
	fprintf(stdout, "%s\n", &programErrorMessage[0]);

	if (vertexShaderSet)
	{
		glDeleteShader(vertexShader);
	}

	if (fragmentShaderSet)
	{
		glDeleteShader(fragmentShader);
	}

	if (computeShaderSet)
	{
		glDeleteShader(computeShader);
	}

}

Shader::~Shader()
{
}
