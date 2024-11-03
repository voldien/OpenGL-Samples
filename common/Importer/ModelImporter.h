/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#pragma once
#include "Math3D/LinAlg.h"
#include <IO/IFileSystem.h>
#include <Math3D/AABB.h>
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
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace glsample {}

typedef struct asset_object_t {
	std::string name;
} AssetObject;

typedef struct vertex_bone_data_t {
	static const int NUM_BONES_PER_VERTEX = 4;
	uint32_t IDs[NUM_BONES_PER_VERTEX];
	float Weights[NUM_BONES_PER_VERTEX];
} VertexBoneData;

typedef struct vertex_bone_buffer_t {
	std::vector<VertexBoneData> vertexBoneData;
} VertexBoneBuffer;

typedef struct material_texture_sampling_t {
	unsigned int wrapping;
	unsigned int filtering;
	unsigned int mapping;
} MterialTextureSampling;

typedef struct material_object_t : public AssetObject {
	unsigned int program; // TODO: relocate.

	/*	Texture index.	*/
	int diffuseIndex = -1;
	int normalIndex = -1;
	int emissionIndex = -1;
	int heightbumpIndex = -1;
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

	/*	*/
	float shinininess;
	float shinininessStrength;
	float opacity;
	int blend_func_mode; /*	aiBlendMode*/
	int wireframe_mode;
	bool culling_both_side_mode;

	/*	*/

	unsigned int shade_model; /*	aiShadingMode	*/
} MaterialObject;

typedef struct node_object_t : public AssetObject {

	/*	*/
	glm::vec3 localPosition;
	glm::quat localRotation;
	glm::vec3 localScale;

	/*	*/
	glm::mat4 modelGlobalTransform;
	glm::mat4 modelLocalTransform;

	fragcore::Bound bound;

	/*	Geometry and material.	*/
	std::vector<unsigned int> geometryObjectIndex;
	std::vector<unsigned int> materialIndex;

	struct node_object_t *parent = nullptr;
} NodeObject;

typedef struct mesh_data_t : public AssetObject {
	/*	*/
	size_t nrVertices;
	size_t nrIndices;

	size_t vertexStride;
	size_t indicesStride;

	/*	*/
	void *vertexData;
	void *indicesData;

} MeshData;

typedef struct morph_target {

} MorpthTarget;

typedef struct model_system_object : public AssetObject {

	// MeshData mesh;
	// MeshData bone
	size_t nrVertices;
	size_t nrIndices;
	size_t vertexStride;
	size_t indicesStride;

	void *vertexData;
	void *indicesData;

	unsigned int material_index;

	fragcore::Bound bound;

	/*	*/
	unsigned int vertexOffset;
	unsigned int normalOffset;
	unsigned int tangentOffset;
	unsigned int uvOffset;
	unsigned int boneOffset;
	unsigned int boneWeightOffset;
	unsigned int boneIndexOffset;

	unsigned int primitiveType;

} ModelSystemObject;

typedef struct bone_t : public AssetObject {
	glm::mat4 inverseBoneMatrix;
	size_t boneIndex;
} Bone;

typedef struct model_skeleton_t : public AssetObject {

	std::map<std::string, Bone> bones;

} SkeletonSystem;

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

typedef struct curve_t : public AssetObject {
	std::vector<KeyFrame> keyframes;
} Curve;

typedef struct animation_object_t : public AssetObject {
	std::vector<Curve> curves;
	float duration;
} AnimationObject;

typedef struct light_object_t : public AssetObject {

	// C_ENUM aiLightSourceType mType;

	glm::vec3 mPosition;

	glm::vec3 mDirection;

	glm::vec3 mUp;

	float mAttenuationConstant;

	float mAttenuationLinear;

	float mAttenuationQuadratic;

	glm::vec4 mColorDiffuse;

	glm::vec4 mColorSpecular;

	glm::vec4 mColorAmbient;

	float mAngleInnerCone;

	float mAngleOuterCone;
} LightObject;

using namespace Assimp;

class FVDECLSPEC ModelImporter {
  public:
	ModelImporter(fragcore::IFileSystem *fileSystem) : fileSystem(fileSystem) {}
	ModelImporter(const ModelImporter &other) = default;
	ModelImporter(ModelImporter &&other);
	virtual ~ModelImporter() { this->clear(); }

	ModelImporter &operator=(const ModelImporter &other) = default;
	ModelImporter &operator=(ModelImporter &&other);

	virtual void loadContent(const std::string &path, unsigned long int supportFlag);
	// virtual void loadContentMemory(const std::string &path, unsigned long int supportFlag);
	// TODO:add load from memory.
	virtual void clear() noexcept;

	fragcore::IFileSystem *getFileSystem() const noexcept { return this->fileSystem; }

  protected:
	void initScene(const aiScene *scene);

	/**
	 *
	 */
	void initNoodeRoot(const aiNode *nodes, NodeObject *parent = nullptr);

	MaterialObject *initMaterial(aiMaterial *material, size_t index);

	ModelSystemObject *initMesh(const aiMesh *mesh, unsigned int index);

	SkeletonSystem *initBoneSkeleton(const aiMesh *mesh, unsigned int index);

	TextureAssetObject *initTexture(aiTexture *texture, unsigned int index);

	AnimationObject *initAnimation(const aiAnimation *animation, unsigned int index);
	void loadCurve(aiNodeAnim *curve, Curve *animationClip);
	//

	LightObject *initLight(const aiLight *light, unsigned int index);

	void loadTexturesFromMaterials(aiMaterial *material);

	void convert2Adjcent(const aiMesh *mesh, std::vector<unsigned int> &indices);

  public:
	const std::vector<NodeObject *> getNodes() const noexcept { return this->nodes; }
	const std::vector<ModelSystemObject> &getModels() const noexcept { return this->models; }
	const NodeObject *getNodeRoot() const noexcept { return this->rootNode; }

	const std::vector<SkeletonSystem> &getSkeletons() const noexcept { return this->skeletons; }

	const std::vector<MaterialObject> &getMaterials() const noexcept { return this->materials; }
	std::vector<MaterialObject> &getMaterials() noexcept { return this->materials; }

	std::vector<MaterialObject *> getMaterials(const size_t texture_index) noexcept;

	/*	*/
	const std::vector<TextureAssetObject> &getTextures() const noexcept { return this->textures; }
	std::vector<TextureAssetObject> &getTextures() noexcept { return this->textures; }

	const std::string &getDirectoryPath() const noexcept { return this->filepath; }

	const glm::mat4 &globalTransform() const noexcept { return this->global; }

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

	std::vector<SkeletonSystem> skeletons;

	std::vector<AnimationObject> animations;
	std::map<std::string, VertexBoneData> vertexBoneData;

	NodeObject *rootNode;
	glm::mat4 global;
};
