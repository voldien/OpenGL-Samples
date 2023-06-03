#pragma once
#include "../GLSampleSession.h"
#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/camera.h>
#include <assimp/light.h>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <assimp/types.h>
#include <assimp/vector3.h>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

typedef struct vertex_bone_data_t {
	static const int NUM_BONES_PER_VERTEX = 4;
	uint IDs[NUM_BONES_PER_VERTEX];
	float Weights[NUM_BONES_PER_VERTEX];
} VertexBoneData;

typedef struct vertex_bone_buffer_t {
	std::vector<VertexBoneData> vertexBoneData;
} VertexBoneBuffer;

typedef struct material_object_t {
	unsigned int program;
	std::string name;

	/*	*/
	unsigned int diffuseIndex;
	unsigned int normalIndex;
	unsigned int emissionIndex;
	unsigned int heightIndex;

	/*	TODO its own struct.	*/
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 emission;
	glm::vec4 specular;
	glm::vec4 transparent;
	glm::vec4 reflectivity;
	float shinininess;
	float hinininessStrength;
} MaterialObject;

typedef struct model_object {
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 transform;

	std::vector<unsigned int> geometryObjectIndex;
	std::vector<unsigned int> materialIndex;

	struct model_object *parent;
	std::string name;
} NodeObject;

typedef struct model_system_object {
	size_t nrVertices;
	size_t nrIndices;
	size_t vertexStride;
	size_t indicesStride;
	void *vertexData;
	void *indicesData;

	unsigned int vertexOffset;
	unsigned int normalOffset;
	unsigned int tangentOffset;
	unsigned int uvOffset;
	unsigned int boneOffset;

} ModelSystemObject;

typedef struct texture_object_t {
	size_t texture = 0;
	size_t width = 0;
	size_t height = 0;
	std::string filepath;
	void *data;
} TextureObject;

typedef struct animation_object_t {

} AnimationObject;

using namespace Assimp;

class FVDECLSPEC ModelImporter {
  public:
	IFileSystem *fileSystem;
	ModelImporter(IFileSystem *fileSystem) : fileSystem(fileSystem) {}
	~ModelImporter() { this->clear(); }

	void loadContent(const std::string &path, unsigned long int supportFlag);

	void clear();

  protected:
	void initScene(const aiScene *scene);

	/**
	 *
	 */
	void initNoodeRoot(const aiNode *nodes, NodeObject *parent = nullptr);

	MaterialObject *initMaterial(aiMaterial *material, size_t index);

	ModelSystemObject *initMesh(const aiMesh *mesh, unsigned int index);

	TextureObject *initTexture(aiTexture *texture, unsigned int index);

	// VDAnimationClip *initAnimation(const aiAnimation *animation, unsigned int index);
	//
	// VDLight *initLight(const aiLight *light, unsigned int index);

  public:
	const std::vector<NodeObject *> getNodes() const { return this->nodes; }
	const std::vector<ModelSystemObject> &getModels() const { return this->models; }
	const NodeObject *getNodeRoot() const { return this->rootNode; }

	const std::vector<MaterialObject> &getMaterials() const { return this->materials; }
	std::vector<MaterialObject> &getMaterials() { return this->materials; }

	/*	*/
	const std::vector<TextureObject> &getTextures() const { return this->textures; }
	std::vector<TextureObject> &getTextures() { return this->textures; }

  private:
	aiScene *sceneRef;
	std::vector<NodeObject *> nodes;
	std::vector<ModelSystemObject> models;
	std::vector<MaterialObject> materials;
	std::vector<TextureObject> textures;
	std::map<std::string, TextureObject *> textureMapping;
	std::vector<AnimationObject> animations;
	std::map<std::string, VertexBoneData> vertexBoneData;

	NodeObject *rootNode;
};

static void drawScene(NodeObject *node) {}