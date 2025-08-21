/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
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
#include "DataStructure/PoolAllocator.h"
#include "FragDef.h"
#include "Math3D/LinAlg.h"
#include "RenderDesc.h"
#include <IO/IFileSystem.h>
#include <Math3D/AABB.h>

#include <cassert>
#include <cstddef>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

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

namespace glsample {}

using AssetObject = struct asset_object_t {
	std::string name;
};

using VertexBoneData = struct alignas(32) vertex_bone_data_t {
	static const int NUM_BONES_PER_VERTEX = 4;
	uint32_t IDs[NUM_BONES_PER_VERTEX];
	float Weights[NUM_BONES_PER_VERTEX];
};

using VertexBoneBuffer = struct vertex_bone_buffer_t {
	std::vector<VertexBoneData> vertexBoneData;
};

using MaterialTextureSampling = struct material_texture_sampling_t {
	fragcore::TextureWrappingMode wrapping = fragcore::TextureWrappingMode::Repeat;
	fragcore::TextureFilterMode filtering = fragcore::TextureFilterMode::Linear;
	fragcore::TextureUVMappingMode uv_mapping = fragcore::TextureUVMappingMode::UV;
};

using MaterialObject = struct material_object_t : public AssetObject {
	unsigned int program = 0; // TODO: relocate.

	// Material properties.
	glm::vec4 ambient = glm::vec4(1, 1, 1, 1);
	glm::vec4 diffuse = glm::vec4(1);
	glm::vec4 emission = glm::vec4(0);
	glm::vec4 specular = glm::vec4(1);
	glm::vec4 transparent = glm::vec4(1);
	glm::vec4 reflectivity = glm::vec4(1);

	/*	*/
	float shinininess = 1;
	float bumpiness = 1;
	float opacity = 1;
	int blend_func_mode = 0; /*	aiBlendMode*/
	int wireframe_mode = 0;
	bool culling_both_side_mode = false;
	float clipping = 1;
	/*	*/

	unsigned int shade_model = 0; /*	aiShadingMode	*/

	MaterialTextureSampling texture_sampling[32];

	/*	Texture index.	*/
	union {
		struct {
			int diffuseIndex = -1;			/*	*/
			int normalIndex = -1;			/*	*/
			int maskTextureIndex = -1;		/*	*/
			int specularIndex = -1;			/*	*/
			int emissionIndex = -1;			/*	*/
			int reflectionIndex = -1;		/*	*/
			int ambientOcclusionIndex = -1; /*	*/
			int displacementIndex = -1;		/*	*/
			int metalIndex = -1;			/*	*/
			int heightbumpIndex = -1;		/*	*/
			int pad1 = -1;
			int pad2 = -1;
			int pad3 = -1;
			int pad4 = -1;
			int pad5 = -1;
			int pad6 = -1;
			int pad7 = -1;
			int pad8 = -1;
			int pad9 = -1;
			int pad10 = -1;
			int pad11 = -1;
			int pad12 = -1;
			int pad13 = -1;
			int pad14 = -1;
			int pad15 = -1;
			int pad16 = -1;
			int pad17 = -1;
			int pad18 = -1;
			int pad19 = -1;
			int pad20 = -1;
			int pad21 = -1;
			int pad22 = -1;
		};
		std::array<int, 32> texture_index; /*	TextureType.	*/
	};
};

using NodeObject = struct node_object_t : public AssetObject {
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
};

using MeshData = struct mesh_data_t : public AssetObject {
	/*	*/
	size_t nrVertices{};
	size_t nrIndices{};

	size_t vertexStride{};
	size_t indicesStride{};

	/*	*/
	void *vertexData{};
	void *indicesData{};
};

using MorpthTarget = struct morph_target {};

using ModelSystemObject = struct model_system_object : public AssetObject {
	// MeshData mesh;
	// MeshData bone
	size_t nrVertices{};
	size_t nrIndices{};
	unsigned int vertexStride{};  /*	In Bytes.	*/
	unsigned int indicesStride{}; /*	In Bytes.	*/

	void *vertexData{};
	void *indicesData{};

	unsigned int material_index{};

	fragcore::Bound bound{};

	/*	*/
	ssize_t vertexOffset{};
	ssize_t uvOffset{};
	ssize_t normalOffset{};
	ssize_t tangentOffset{};
	ssize_t vertexColorOffset{};
	ssize_t boneOffset{};
	ssize_t boneWeightOffset{};
	ssize_t boneIndexOffset{};

	unsigned int primitiveType{};

	bool processed = false;
};

using CameraData = struct camera_data_t : public AssetObject {
	glm::vec3 position;
	glm::vec3 up;
	glm::vec3 lookAt;
};

using Bone = struct alignas(16) bone_t : public AssetObject {
	glm::mat4 finalTransform{};
	glm::mat4 offsetBoneMatrix{};
	size_t boneIndex{};
	NodeObject *armature_bone{};
};

using SkeletonSystem = struct model_skeleton_t : public AssetObject {

	std::map<std::string, Bone> bones;
};

using TextureAssetObject = struct alignas(32) texture_asset_object_t {
	unsigned int texture = 0;
	size_t width = 0;
	size_t height = 0;

	size_t dataSize = 0;
	std::string filepath;
	char *data = nullptr;
};

using KeyFrame = struct alignas(16) key_frame_t {
	float time;		  /*	*/
	float value;	  /*	*/
	float tangentIn;  /*	*/
	float tangentOut; /*	*/
};

using Curve = struct curve_t : public AssetObject {
	std::vector<KeyFrame> keyframes;
};

using AnimationObject = struct animation_object_t : public AssetObject {
	std::map<std::string, Curve> curves_s;
	std::vector<Curve> curves;
	float duration;
};

using LightObject = struct alignas(32) light_object_t : public AssetObject {

	// C_ENUM aiLightSourceType mType;
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 mUp;

	float mAttenuationConstant;

	float mAttenuationLinear;

	float mAttenuationQuadratic;

	glm::vec4 mColorDiffuse;

	glm::vec4 mColorSpecular;

	glm::vec4 mColorAmbient;

	float mAngleInnerCone;

	float mAngleOuterCone;
};

class FVDECLSPEC ModelImporter {
  public:
	ModelImporter(fragcore::IFileSystem *fileSystem) : fileSystem(fileSystem) {}
	ModelImporter(const ModelImporter &other) = default;
	ModelImporter(ModelImporter &&other) noexcept;
	virtual ~ModelImporter() { this->clear(); }

	ModelImporter &operator=(const ModelImporter &other) = default;
	ModelImporter &operator=(ModelImporter &&other) noexcept;

	virtual void loadContent(const std::string &path, unsigned long int supportFlag);
	virtual void clear() noexcept;

	fragcore::IFileSystem *getFileSystem() const noexcept { return this->fileSystem; }

  protected:
	void initScene(const aiScene *scene);

	/**
	 *
	 */
	void initNodeRoot(const aiNode *nodes, NodeObject *parent = nullptr);

	MaterialObject *initMaterial(aiMaterial *material, size_t index);

	ModelSystemObject *initMesh(const aiMesh *mesh, unsigned int index);

	SkeletonSystem *initBoneSkeleton(const aiMesh *mesh, unsigned int index);

	TextureAssetObject *initTexture(aiTexture *texture, unsigned int index);

	AnimationObject *initAnimation(const aiAnimation *animation, unsigned int index);
	//

	LightObject *initLight(const aiLight *light, unsigned int index);
	void loadTexturesFromMaterials(aiMaterial *material);

	void convert2Adjcent(const aiMesh *mesh, std::vector<unsigned int> &indices);

	NodeObject *getNodeByName(const std::string &name) const noexcept;

  public:
	std::vector<NodeObject *> getNodes() const noexcept { return this->nodes; }
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

	const glm::mat4 &globalTransform() const noexcept { return this->globalNodeTransform; }

  private:
	fragcore::IFileSystem *fileSystem = nullptr;

	std::string filepath;
	const aiScene *sceneRef = nullptr;
	fragcore::PoolAllocator<NodeObject> nodePool;
	std::vector<NodeObject *> nodes;
	std::map<std::string, NodeObject *> nodeByName;

	std::vector<ModelSystemObject> models;
	std::vector<MaterialObject> materials;
	/*	*/
	std::vector<TextureAssetObject> textures;
	std::map<std::string, TextureAssetObject *> textureMapping;
	std::map<std::string, unsigned int> textureIndexMapping;

	std::vector<CameraData> cameras;

	std::vector<SkeletonSystem> skeletons;

	std::vector<AnimationObject> animations;
	std::map<std::string, VertexBoneData> vertexBoneData;

	std::vector<LightObject> lights;

	NodeObject *rootNode = nullptr;
	glm::mat4 globalNodeTransform{};
};
