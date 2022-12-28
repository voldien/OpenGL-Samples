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

typedef struct model_object {
	GeometryObject geometryObject;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::mat4 transform;

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
	size_t texture;
	size_t width;
	size_t height;
	std::string filepath;
} TextureObject;

typedef struct material_object_t {
	std::string name;
	glm::vec4 diffuse;
	glm::vec4 specular;
	glm::vec4 transparent;
	glm::vec4 reflectivity;
	float shinininess;
	float hinininessStrength;
} MaterialObject;

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
	const std::vector<TextureObject> &getTextures() const { return this->textures; }

  private:
	aiScene *sceneRef;
	std::vector<NodeObject *> nodes;
	std::vector<ModelSystemObject> models;
	std::vector<MaterialObject> materials;
	std::vector<TextureObject> textures;
	std::vector<AnimationObject> animations;

	NodeObject *rootNode;
};

static void drawScene(NodeObject *node) {}