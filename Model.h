#pragma once

#include <iostream>
#include <vector>

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

struct Tri {
	glm::vec4 p0;
	glm::vec4 p1;
	glm::vec4 p2;
	glm::vec4 norm;
	glm::vec4 tex0;
	glm::vec4 tex1;
	glm::vec4 tex2;
};

struct Texture {
	GLint id;
	aiString path;
	std::string type;
};


class Model
{
	public:
		Model(std::string path);
		~Model();

		std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, std::string directory);
		void loadModel(std::string path);
		void processMaterial(aiMesh* mesh, const aiScene* scene, std::vector<Texture> *textures);
		void processMesh(aiMesh* mesh);
		void processModel(const aiScene* scene, aiNode* node);

		std::vector<Tri> getModelTris();
		std::vector<Texture> getTextures();

		GLint loadTextureFromFile(const char* path);
		

	private:
		std::vector<Tri> modelTris;
		std::string directory;
		std::vector<Texture> texturesLoaded;

};

