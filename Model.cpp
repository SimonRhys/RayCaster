#include "Model.h"


Model::Model(std::string path)
{
	this->loadModel(path);
}

// Checks all material textures of a given type and loads the textures if they're not loaded yet.
// The required info is returned as a Texture struct.
std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, std::string directory)
{
	std::vector<Texture> textures;
	for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		// Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		GLboolean skip = false;
		for (GLuint j = 0; j < texturesLoaded.size(); j++)
		{
			if (texturesLoaded[j].path == str)
			{
				textures.push_back(texturesLoaded[j]);
				skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
				break;
			}
		}
		if (!skip)
		{   // If texture hasn't been loaded already, load it
			Texture texture;
			texture.id = this->loadTextureFromFile(str.C_Str());
			texture.type = typeName;
			texture.path = str;
			textures.push_back(texture);
			texturesLoaded.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
		}
	}
	return textures;
}

void Model::loadModel(std::string path)
{
	// Read file via ASSIMP
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
	// Check for errors
	if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}
	// Retrieve the directory path of the filepath
	this->directory = path.substr(0, path.find_last_of('/'));

	// Process ASSIMP's root node recursively
	this->processModel(scene, scene->mRootNode);
}

void Model::processMaterial(aiMesh* mesh, const aiScene* scene, std::vector<Texture> *textures)
{
	// Process materials
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// We assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
		// Same applies to other texture as the following list summarizes:
		// Diffuse: texture_diffuseN
		// Specular: texture_specularN
		// Normal: texture_normalN

		// 1. Diffuse maps
		std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", directory);
		textures->insert(textures->end(), diffuseMaps.begin(), diffuseMaps.end());
		// 2. Specular maps
		std::vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", directory);
		textures->insert(textures->end(), specularMaps.begin(), specularMaps.end());
	}
}

void Model::processMesh(aiMesh* mesh)
{
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; j += 3)
		{
			glm::vec4 vec0;
			glm::vec4 vec1;
			glm::vec4 vec2;
			glm::vec4 norm = glm::vec4(1, 1, 1, 1);
			glm::vec4 tex0 = glm::vec4(-1, -1, -1, -1);
			glm::vec4 tex1 = glm::vec4(-1, -1, -1, -1);
			glm::vec4 tex2 = glm::vec4(-1, -1, -1, -1);

			vec0.x = mesh->mVertices[face.mIndices[j]].x;
			vec0.y = mesh->mVertices[face.mIndices[j]].y;
			vec0.z = mesh->mVertices[face.mIndices[j]].z;

			vec1.x = mesh->mVertices[face.mIndices[j + 1]].x;
			vec1.y = mesh->mVertices[face.mIndices[j + 1]].y;
			vec1.z = mesh->mVertices[face.mIndices[j + 1]].z;

			vec2.x = mesh->mVertices[face.mIndices[j + 2]].x;
			vec2.y = mesh->mVertices[face.mIndices[j + 2]].y;
			vec2.z = mesh->mVertices[face.mIndices[j + 2]].z;

			if (mesh->HasNormals())
			{
				norm.x = mesh->mNormals[face.mIndices[j]].x;
				norm.y = mesh->mNormals[face.mIndices[j]].y;
				norm.z = mesh->mNormals[face.mIndices[j]].z;
			}

			if (mesh->mTextureCoords[0]) // Does the mesh contain texture coordinates?
			{
				tex0.x = mesh->mTextureCoords[0][face.mIndices[j]].x;
				tex0.y = mesh->mTextureCoords[0][face.mIndices[j]].y;

				tex1.x = mesh->mTextureCoords[0][face.mIndices[j + 1]].x;
				tex1.y = mesh->mTextureCoords[0][face.mIndices[j + 1]].y;

				tex2.x = mesh->mTextureCoords[0][face.mIndices[j + 2]].x;
				tex2.y = mesh->mTextureCoords[0][face.mIndices[j + 2]].y;

			}

			modelTris.push_back({ vec0, vec1, vec2, norm, tex0, tex1, tex2});
		}
	}
}

void Model::processModel(const aiScene* scene, aiNode* node)
{
	//Normally we would store the textures per mesh,
	//but we have a limitation of 1 texture per model atm
	std::vector<Texture> textures;
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		this->processMesh(mesh);
		this->processMaterial(mesh, scene, &textures);
	}

	for (int i = 0; i < node->mNumChildren; i++)
	{
		this->processModel(scene, node->mChildren[i]);
	}
}

std::vector<Tri> Model::getModelTris()
{
	return modelTris;
}

std::vector<Texture> Model::getTextures()
{
	return texturesLoaded;
}

GLint Model::loadTextureFromFile(const char* path)
{
	//Generate texture ID and load texture data 
	std::string filename = std::string(path);
	filename = directory + '/' + filename;
	GLuint textureID;
	glGenTextures(1, &textureID);
	int width, height;
	unsigned char* image = SOIL_load_image(filename.c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
	// Assign texture to ID
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glBindTexture(GL_TEXTURE_2D, 0);
	SOIL_free_image_data(image);
	return textureID;
}

bool Model::hasTexture()
{
	return texturesLoaded.size() > 0;
}

Model::~Model()
{
}
