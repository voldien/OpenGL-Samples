#pragma once
#include "Core/IO/IFileSystem.h"
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
#include <cstddef>
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
	int diffuseIndex = -1;
	int normalIndex = -1;
	int emissionIndex = -1;
	int heightIndex = -1;
	int specularIndex = -1;
	int reflectionIndex = -1;
	int ambientOcclusionIndex = -1;
	int metalIndex = -1;
	int maskTextureIndex = -1;

	/*	TODO its own struct.	*/
	// Material properties.
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 emission;
	glm::vec4 specular;
	glm::vec4 transparent;
	glm::vec4 reflectivity;
	float shinininess;
	float shinininessStrength;

	int blend_mode;

	unsigned int shade_model;
} MaterialObject;

typedef struct model_object {

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	glm::mat4 transform;

	/*	Geometry and material.	*/
	std::vector<unsigned int> geometryObjectIndex;
	std::vector<unsigned int> materialIndex;

	struct model_object *parent = nullptr;
	std::string name;
} NodeObject;

typedef struct model_system_object {
	size_t nrVertices;
	size_t nrIndices;
	size_t vertexStride;
	size_t indicesStride;
	void *vertexData;
	void *indicesData;

	unsigned int material_index;

	/*	*/
	unsigned int vertexOffset;
	unsigned int normalOffset;
	unsigned int tangentOffset;
	unsigned int uvOffset;
	unsigned int boneOffset;

	// Bounding box.
	//ColorSpace colorSpace;
} ModelSystemObject;

typedef struct texture_asset_object_t {
	unsigned int texture = 0;
	size_t width = 0;
	size_t height = 0;

	size_t dataSize = 0;
	std::string filepath;
	char *data = nullptr;

} TextureAssetObject;

typedef struct key_frame_t {
	float time;
	float value;
	float tangentIn;
	float tangentOut;
} KeyFrame;

typedef struct curve_t {
	std::vector<KeyFrame> keyframes;
	std::string name;

	/*	Binding.	*/
} Curve;

typedef struct animation_object_t {
	std::vector<Curve> curves;
	std::string name;

} AnimationObject;

using namespace Assimp;

class FVDECLSPEC ModelImporter {
  public:
	ModelImporter(fragcore::IFileSystem *fileSystem) : fileSystem(fileSystem) {}
	~ModelImporter() { this->clear(); }

	virtual void loadContent(const std::string &path, unsigned long int supportFlag);
	virtual void clear();

	fragcore::IFileSystem *getFileSystem() const noexcept { return this->fileSystem; }

  protected:
	void initScene(const aiScene *scene);

	/**
	 *
	 */
	void initNoodeRoot(const aiNode *nodes, NodeObject *parent = nullptr);

	MaterialObject *initMaterial(aiMaterial *material, size_t index);

	ModelSystemObject *initMesh(const aiMesh *mesh, unsigned int index);

	TextureAssetObject *initTexture(aiTexture *texture, unsigned int index);

	AnimationObject *initAnimation(const aiAnimation *animation, unsigned int index);
	void loadCurve(aiNodeAnim *curve, Curve *animationClip);
	//

	void loadTexturesFromMaterials(aiMaterial *material);

	// TODO Compute bounding box

  public:
	const std::vector<NodeObject *> getNodes() const { return this->nodes; }
	const std::vector<ModelSystemObject> &getModels() const { return this->models; }
	const NodeObject *getNodeRoot() const { return this->rootNode; }

	const std::vector<MaterialObject> &getMaterials() const { return this->materials; }
	std::vector<MaterialObject> &getMaterials() { return this->materials; }

	/*	*/
	const std::vector<TextureAssetObject> &getTextures() const { return this->textures; }
	std::vector<TextureAssetObject> &getTextures() { return this->textures; }

	const std::string &getDirectoryPath() const { return this->filepath; }

  private:
	fragcore::IFileSystem *fileSystem;

	std::string filepath;
	aiScene *sceneRef;
	std::vector<NodeObject *> nodes;

	std::vector<ModelSystemObject> models;
	std::vector<MaterialObject> materials;
	/*	*/
	std::vector<TextureAssetObject> textures;
	std::map<std::string, TextureAssetObject *> textureMapping;
	std::map<std::string, unsigned int> textureIndexMapping;

	std::vector<AnimationObject> animations;
	std::map<std::string, VertexBoneData> vertexBoneData;

	NodeObject *rootNode;
};

static void drawScene(NodeObject *node) {}