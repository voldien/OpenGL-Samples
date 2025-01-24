#include "ModelImporter.h"
#include "Core/SystemInfo.h"
#include "Math/Math.h"
#include "TaskScheduler/IScheduler.h"
#include "assimp/Importer.hpp"
#include "assimp/ProgressHandler.hpp"
#include "assimp/config.h"
#include "assimp/scene.h"
#include <IO/IOUtil.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glm/fwd.hpp>
#include <sys/types.h>
#include <thread>
#include <utility>

namespace fs = std::filesystem;
using namespace fragcore;
using namespace Assimp;

static inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4 *from) noexcept {
	glm::mat4 to;

	to[0][0] = (float)from->a1;
	to[0][1] = (float)from->b1;
	to[0][2] = (float)from->c1;
	to[0][3] = (float)from->d1;
	to[1][0] = (float)from->a2;
	to[1][1] = (float)from->b2;
	to[1][2] = (float)from->c2;
	to[1][3] = (float)from->d2;
	to[2][0] = (float)from->a3;
	to[2][1] = (float)from->b3;
	to[2][2] = (float)from->c3;
	to[2][3] = (float)from->d3;
	to[3][0] = (float)from->a4;
	to[3][1] = (float)from->b4;
	to[3][2] = (float)from->c4;
	to[3][3] = (float)from->d4;

	return to;
}

ModelImporter::ModelImporter(ModelImporter &&other)
	: filepath(other.filepath), nodes(other.nodes), models(other.models), materials(other.materials),
	  textures(other.textures), textureMapping(other.textureMapping), textureIndexMapping(other.textureIndexMapping),
	  skeletons(other.skeletons), animations(other.animations), vertexBoneData(other.vertexBoneData),
	  rootNode(other.rootNode), globalNodeTransform(other.globalNodeTransform) {
	this->fileSystem = std::exchange(other.fileSystem, nullptr);
}

ModelImporter &ModelImporter::operator=(ModelImporter &&other) {
	this->fileSystem = std::exchange(other.fileSystem, nullptr);

	this->globalNodeTransform = other.globalNodeTransform;
	this->rootNode = other.rootNode;
	this->filepath = other.filepath;

	this->nodes = other.nodes;
	this->models = other.models;
	this->materials = other.materials;
	this->textures = other.textures;
	this->textureMapping = other.textureMapping;
	this->textureIndexMapping = other.textureIndexMapping;
	this->skeletons = other.skeletons;
	this->animations = other.animations;
	this->vertexBoneData = other.vertexBoneData;

	return *this;
}

class CustomProgress : public Assimp::ProgressHandler {
  public:
	bool Update(float percentage = -1.f) override {
		std::cout << "\33[2K\r" << "Loading Model: " << percentage * 100.0f << "/100" << std::flush;
		return true;
	}
};

void ModelImporter::loadContent(const std::string &path, unsigned long int supportFlag) {
	Assimp::Importer importer;

	this->filepath = fs::path(fileSystem->getAbsolutePath(path.c_str())).parent_path();

	importer.SetPropertyBool(AI_CONFIG_GLOB_MEASURE_TIME, false);
	importer.SetProgressHandler(new CustomProgress());

	/*	*/
	this->sceneRef = importer.ReadFile(path.c_str(), aiProcessPreset_TargetRealtime_Quality |
														 aiProcess_GenBoundingBoxes | aiProcess_PopulateArmatureData);

	if (this->sceneRef == nullptr) {
		throw RuntimeException("Failed to load file: {} - Error: {}", path, importer.GetErrorString());
	}

	this->globalNodeTransform = aiMatrix4x4ToGlm(&this->sceneRef->mRootNode->mTransformation);

	this->initScene(this->sceneRef);
}

void ModelImporter::clear() noexcept {

	for (size_t i = 0; i < this->textures.size(); i++) {
		if (this->textures[i].data) {
			free(this->textures[i].data);
		}
	}

	this->nodes.clear();
	this->models.clear();
	this->materials.clear();
	this->textures.clear();
	this->animations.clear();
	this->lights.clear();
	this->skeletons.clear();
}

void ModelImporter::initScene(const aiScene *scene) {

	fragcore::IScheduler *schedular = this->getFileSystem()->getScheduler().ptr();

	this->models.resize(scene->mNumMeshes);

	/*	*/
	std::thread process_textures_thread([&]() {
		/*	*/
		if (scene->HasTextures()) {

			this->textures.resize(scene->mNumTextures);

			for (size_t i = 0; i < scene->mNumTextures; i++) {
				std::cout << scene->mTextures[i]->mFilename.C_Str() << std::endl;

				/*	*/
				this->textures[i] = *this->initTexture(scene->mTextures[i], i);
				this->textureMapping[scene->mTextures[i]->mFilename.C_Str()] = &this->textures[i];

				this->textureIndexMapping[scene->mTextures[i]->mFilename.C_Str()] = i;
			}
		}

		/*	*/
		for (size_t x = 0; x < scene->mNumMaterials; x++) {
			this->loadTexturesFromMaterials(scene->mMaterials[x]);
		}

		for (size_t x = 0; x < scene->mNumMeshes; x++) {
			// TODO: relocate.
			C_STRUCT aiAABB aabb = scene->mMeshes[x]->mAABB;

			this->models[x].bound.aabb.min[0] = aabb.mMin.x;
			this->models[x].bound.aabb.min[1] = aabb.mMin.y;
			this->models[x].bound.aabb.min[2] = aabb.mMin.z;

			this->models[x].bound.aabb.max[0] = aabb.mMax.x;
			this->models[x].bound.aabb.max[1] = aabb.mMax.y;
			this->models[x].bound.aabb.max[2] = aabb.mMax.z;
		}
	});

	/*	*/
	const size_t nr_threads = fragcore::Math::clamp<size_t>(scene->mNumMeshes / 4, 1, SystemInfo::getCPUCoreCount());
	std::vector<std::thread> model_threads(nr_threads);

	/*	Multithread the loading of all the geometry data.	*/
	for (size_t index_thread = 0; index_thread < model_threads.size(); index_thread++) {

		const size_t start_mesh = (scene->mNumMeshes / nr_threads) * index_thread;
		size_t num_mesh = (scene->mNumMeshes / nr_threads);
		if (index_thread == model_threads.size() - 1) {
			num_mesh *= 2;
		}

		model_threads[index_thread] = std::thread([&, start_mesh, num_mesh]() {
			if (scene->HasMeshes()) {

				for (size_t x = start_mesh; x < fragcore::Math::min<size_t>(start_mesh + num_mesh, scene->mNumMeshes);
					 x++) {
					this->initMesh(scene->mMeshes[x], x);
				}
			}
		});
	}
	/*	*/
	std::thread process_animation_light_camera_thread([&]() {
		if (scene->HasAnimations()) {
			for (size_t x = 0; x < scene->mNumAnimations; x++) {
				this->initAnimation(scene->mAnimations[x], x);
			}
		}

		if (scene->HasLights()) {
			this->lights.resize(scene->mNumLights);
			for (unsigned int x = 0; x < scene->mNumLights; x++) {
				this->initLight(scene->mLights[x], x);
			}
		}

		if (scene->HasCameras()) {
			for (unsigned int x = 0; x < scene->mNumCameras; x++) {
				/*	*/
			}
		}
	});
	// process_animation_light_camera_thread.detach();

	process_textures_thread.join();
	process_animation_light_camera_thread.join();

	for (size_t i = 0; i < model_threads.size(); i++) {
		model_threads[i].join();
	}

	/*	*/
	if (scene->HasMaterials()) {
		/*	Extract additional texture */

		this->materials.resize(scene->mNumMaterials);
		for (size_t x = 0; x < scene->mNumMaterials; x++) {
			this->initMaterial(scene->mMaterials[x], x);
		}
	}

	this->initNoodeRoot(scene->mRootNode, nullptr);

	/*	*/
	for (size_t x = 0; x < scene->mNumMeshes; x++) {
		this->initBoneSkeleton(scene->mMeshes[x], x);
	}
}

void ModelImporter::initNoodeRoot(const aiNode *ai_node, NodeObject *parent) {
	size_t gameObjectCount = 0;
	size_t meshCount = 0;

	/*	iterate through each child of parent node.	*/
	for (size_t node_index = 0; node_index < ai_node->mNumChildren; node_index++) {
		aiNode *child_node = ai_node->mChildren[node_index];

		unsigned int meshCount = 0;
		aiVector3D position, scale;
		aiQuaternion rotation;

		NodeObject *pobject = new NodeObject();

		/*	extract position, rotation, position from transformation matrix.	*/
		child_node->mTransformation.Decompose(scale, rotation, position);
		if (parent) {
			pobject->parent = parent;
		} else {
			pobject->parent = nullptr;
		}

		pobject->localPosition = glm::vec3(position.x, position.y, position.z);
		pobject->localRotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
		pobject->localScale = glm::vec3(scale.x, scale.y, scale.z);
		/*	*/
		pobject->modelLocalTransform = aiMatrix4x4ToGlm(&child_node->mTransformation);

		if (parent) {
			pobject->modelGlobalTransform = parent->modelGlobalTransform * pobject->modelLocalTransform;
		} else {
			pobject->modelGlobalTransform = this->globalTransform() * pobject->modelLocalTransform;
		}

		pobject->name = ai_node->mChildren[node_index]->mName.C_Str();

		/*	*/
		if (ai_node->mChildren[node_index]->mMeshes) {

			/*	*/
			for (unsigned int mesh_index = 0; mesh_index < child_node->mNumMeshes; mesh_index++) {

				/*	Get material for mesh object.	*/
				const MaterialObject &materialRef =
					getMaterials()[this->sceneRef->mMeshes[*child_node->mMeshes]->mMaterialIndex];

				/*	*/
				const int meshIndex = child_node->mMeshes[mesh_index];
				pobject->materialIndex.push_back(this->sceneRef->mMeshes[meshIndex]->mMaterialIndex);

				pobject->geometryObjectIndex.push_back(meshIndex);

				pobject->bound = this->models[meshIndex].bound;
			}
		}

		/*	*/
		this->nodes.push_back(pobject);
		this->nodeByName[std::string(child_node->mName.C_Str())] = pobject;

		/*	*/
		this->initNoodeRoot(child_node, pobject);
	}
}

SkeletonSystem *ModelImporter::initBoneSkeleton(const aiMesh *mesh, unsigned int index) {

	SkeletonSystem skeleton;

	/*	Load bones.	*/
	if (mesh->HasBones()) {

		for (uint32_t bone_index = 0; bone_index < mesh->mNumBones; bone_index++) {

			const std::string BoneName(mesh->mBones[bone_index]->mName.data);

			if (skeleton.bones.find(BoneName) == skeleton.bones.end()) {

				NodeObject *nodeObj = this->getNodeByName(BoneName);

				glm::mat4 nodeGlobalTransform = glm::mat4(1);
				if (nodeObj) {
					nodeGlobalTransform = nodeObj->modelGlobalTransform;
				}

				Bone bone;
				bone.name = BoneName;
				bone.boneIndex = bone_index;
				bone.offsetBoneMatrix = aiMatrix4x4ToGlm(&mesh->mBones[bone_index]->mOffsetMatrix);
				bone.finalTransform =
					nodeGlobalTransform * bone.offsetBoneMatrix; /*	Compute default final transformation*/
				bone.armature_bone = nodeObj;

				skeleton.bones[BoneName] = bone;
			}
		}

		this->skeletons.push_back(skeleton);
		return &this->skeletons.back();
	}
	return nullptr;
}

ModelSystemObject *ModelImporter::initMesh(const aiMesh *aimesh, unsigned int index) {
	ModelSystemObject *pmesh = &this->models[index];

	const unsigned int nrUVs = fragcore::Math::max<unsigned int>(aimesh->GetNumUVChannels(), 1);
	const unsigned int nrVertexColors = aimesh->GetNumColorChannels();

	/*	*/
	const size_t vertexSize = sizeof(float) * 3;
	const size_t uvSize = nrUVs * (sizeof(float) * 2);
	const size_t normalSize = sizeof(float) * 3;
	const size_t tangentSize = sizeof(float) * 3;
	const size_t boneIDSize = sizeof(unsigned int);
	const size_t boneWeightSize = sizeof(float);

	/*	*/
	const size_t boneWeightCount = 4;
	size_t boneByteSize = 0;
	size_t bonecount = 0;
	if (aimesh->HasBones()) {
		boneByteSize = boneIDSize * boneWeightCount + boneWeightSize * boneWeightCount;
		bonecount = boneWeightCount;
	}

	/*	*/
	const size_t StrideSize = vertexSize + uvSize + normalSize + tangentSize + boneByteSize;
	const uint floatStride = StrideSize / sizeof(float);

	const size_t indicesSize = 4;

	/*	*/
	float *vertices = (float *)malloc(aimesh->mNumVertices * StrideSize);
	unsigned char *Indice =
		(unsigned char *)malloc(indicesSize * aimesh->mNumFaces * 3); // TODO: compute number of faces.

	/*	*/
	unsigned char *Itemp = Indice;

	/*	*/
	const aiVector3D Zero = aiVector3D(0, 0, 0);
	if (aimesh->HasPositions()) {

		for (unsigned int x = 0; x < aimesh->mNumVertices; x++) {

			float *pVertex = &vertices[static_cast<size_t>(floatStride * x)];

			/*	*/
			const aiVector3D *Pos = &(aimesh->mVertices[x]);
			aiVector3D *pNormal = &(aimesh->mNormals[x]);
			const aiVector3D *Tangent = aimesh->mTangents ? &(aimesh->mTangents[x]) : nullptr;

			/*	Vertex position.	*/
			*pVertex++ = Pos->x;
			*pVertex++ = Pos->y;
			*pVertex++ = Pos->z;

			/*	UV coordinates.	*/
			if (aimesh->GetNumUVChannels() > 0) {
				for (unsigned int uv_index = 0; uv_index < aimesh->GetNumUVChannels(); uv_index++) {
					if (aimesh->HasTextureCoords(uv_index)) {
						*pVertex++ = aimesh->mTextureCoords[uv_index][x].x;
						*pVertex++ = aimesh->mTextureCoords[uv_index][x].y;
					}
				}
			} else {
				*pVertex++ = 0;
				*pVertex++ = 0;
			}

			/*	Normals.	*/
			if (aimesh->HasNormals()) {
				pNormal->Normalize();
				*pVertex++ = pNormal->x;
				*pVertex++ = pNormal->y;
				*pVertex++ = pNormal->z;
			} else {
				*pVertex++ = 0;
				*pVertex++ = 0;
				*pVertex++ = 0;
			}

			/*	*/
			if (Tangent) {
				*pVertex++ = Tangent->x;
				*pVertex++ = Tangent->y;
				*pVertex++ = Tangent->z;
			} else {

				*pVertex++ = 0;
				*pVertex++ = 0;
				*pVertex++ = 0;
			}

			if (aimesh->GetNumColorChannels() > 0) {
				for (unsigned int color_index = 0; color_index < aimesh->GetNumColorChannels(); color_index++) {
					aimesh->mColors[color_index][x];
				}
			}

			/*	Offset only. assign later.	*/
			if (boneByteSize > 0 && bonecount > 0) {
				/*	BoneID	*/
				for (unsigned int i = 0; i < bonecount; i++) {
					*pVertex++ = 0;
				}
				/*	BoneWeight	*/
				for (unsigned int i = 0; i < bonecount; i++) {
					*pVertex++ = 0;
				}
			}

		} /*	*/
	}

	/*	Assign data offset.	*/
	pmesh->vertexOffset = 0;
	pmesh->uvOffset = vertexSize;
	pmesh->normalOffset = vertexSize + uvSize;
	pmesh->tangentOffset = vertexSize + uvSize + normalSize;

	/*	Load bones.	*/
	if (aimesh->HasBones()) {

		/*	*/
		pmesh->boneIndexOffset = (vertexSize + uvSize + normalSize + tangentSize);
		pmesh->boneWeightOffset = (vertexSize + uvSize + normalSize + tangentSize + bonecount * boneIDSize);

		const uint BoneStrideOffset = (pmesh->boneIndexOffset / sizeof(float));

		for (uint i = 0; i < aimesh->mNumBones; i++) {
			const unsigned int BoneIndex = i;

			aiBone *bone = aimesh->mBones[i];
			std::string BoneName(aimesh->mBones[i]->mName.data);

			for (uint j = 0; j < aimesh->mBones[i]->mNumWeights; j++) {

				const unsigned int VertexID = aimesh->mBones[i]->mWeights[j].mVertexId;
				const float Weight = aimesh->mBones[i]->mWeights[j].mWeight;

				float *boneData = &vertices[(VertexID * floatStride) + BoneStrideOffset];

				/*	Assign next bone without any value.	*/
				for (uint x = 0; x < bonecount; x++) {
					/*	Check weight.	*/
					if (boneData[bonecount + x] == 0.0) {

						uint32_t *vertexUint = (uint32_t *)&boneData[x];
						*vertexUint = BoneIndex;

						boneData[bonecount + x] = Weight;
						break;
					}
				}
			}
		}
	}

	/*	Primitive Indices.	*/
	size_t nrFaces = 0;
	if (aimesh->HasFaces()) {

		if (indicesSize == sizeof(unsigned int)) {

			for (size_t x = 0; x < aimesh->mNumFaces; x++) {
				const aiFace &face = aimesh->mFaces[x];

				/*	*/
				std::memcpy(Indice, &face.mIndices[0], indicesSize * face.mNumIndices);
				Indice += indicesSize * face.mNumIndices;

				nrFaces += face.mNumIndices;
			}

		} else { // TODO determine if can be removed.
			for (size_t x = 0; x < aimesh->mNumFaces; x++) {
				const aiFace &face = aimesh->mFaces[x];

				if (face.mNumIndices == 3) {

					std::memcpy(Indice, &face.mIndices[0], indicesSize);
					Indice += indicesSize;
					std::memcpy(Indice, &face.mIndices[1], indicesSize);
					Indice += indicesSize;
					std::memcpy(Indice, &face.mIndices[2], indicesSize);
					Indice += indicesSize;

				} else if (face.mNumIndices == 2) {
					std::memcpy(Indice, &face.mIndices[0], indicesSize);
					Indice += indicesSize;
					std::memcpy(Indice, &face.mIndices[1], indicesSize);
					Indice += indicesSize;
				}

				nrFaces += face.mNumIndices;
			}
		}

		/*	Reset pointer.	*/
		Indice = Itemp;

		/*	*/
		if (aimesh->mNumAnimMeshes > 0) {
			for (unsigned int i = 0; i < aimesh->mNumAnimMeshes; i++) {
				const aiAnimMesh *animMesh = aimesh->mAnimMeshes[i];
				if (animMesh->HasPositions()) {
				}
				if (animMesh->HasNormals()) {
				}

				if (animMesh->HasTangentsAndBitangents()) {
				}

				if (animMesh->HasTextureCoords(0)) {
				}

				for (size_t x = 0; x < animMesh->mNumVertices; x++) {
				}
			}
		}
	}

	/*	*/
	pmesh->indicesData = Indice;
	pmesh->indicesStride = indicesSize;
	pmesh->nrIndices = nrFaces;
	pmesh->nrVertices = aimesh->mNumVertices;
	pmesh->vertexData = vertices;
	pmesh->vertexStride = StrideSize;
	pmesh->primitiveType = aimesh->mPrimitiveTypes;
	pmesh->name = std::string(aimesh->mName.C_Str());

	return pmesh;
}

MaterialObject *ModelImporter::initMaterial(aiMaterial *ref_material, size_t index) {

	aiString name;
	aiString path;

	aiTextureMapping mapping;
	unsigned int uvindex = 0;
	float blend = NAN;
	aiTextureOp op;
	aiTextureMapMode mapmode = aiTextureMapMode::aiTextureMapMode_Wrap;

	glm::vec4 color;
	float shininessStrength = NAN;

	MaterialObject *material = &this->materials[index];

	if (!ref_material) {
		return nullptr;
	}

	const bool isTextureEmpty = this->textures.size() == 0;

	/*	*/
	if (ref_material->Get(AI_MATKEY_NAME, name) == aiReturn_SUCCESS) {
		material->name = name.C_Str();
	}

	/*	load all texture assoicated with material.	*/
	for (size_t textureType = aiTextureType::aiTextureType_DIFFUSE; textureType < aiTextureType::aiTextureType_UNKNOWN;
		 textureType++) {

		/*	*/
		for (size_t textureIndex = 0; textureIndex < ref_material->GetTextureCount((aiTextureType)textureType);
			 textureIndex++) {

			/*	*/
			aiString textureName;
			if (ref_material->Get(AI_MATKEY_TEXTURE(textureType, textureIndex), textureName) ==
				aiReturn::aiReturn_SUCCESS) {
				/*	*/
			}

			aiTextureFlags textureFlag;
			if (ref_material->Get(AI_MATKEY_TEXFLAGS(textureType, textureIndex), textureFlag) ==
				aiReturn::aiReturn_SUCCESS) {
				/*	*/
			}

			/*	*/
			const auto *embeededTexture = sceneRef->GetEmbeddedTexture(textureName.C_Str());

			if (ref_material->GetTexture((aiTextureType)textureType, textureIndex, &path, &mapping, &uvindex, &blend,
										 &op, &mapmode) == aiReturn::aiReturn_SUCCESS) {

				/*	If embeeded.	*/
				int texIndex = 0;
				if (path.data[0] == '*' && embeededTexture) {
					texIndex = atoi(&path.data[1]);
				} else {
					const TextureAssetObject *textureObj = this->textureMapping[path.C_Str()];
					if (textureObj) {
						texIndex = this->textureIndexMapping[path.C_Str()];
					}
				}

				/*	*/
				if (texIndex >= 0) {
					material->texture_sampling[texIndex].filtering = 0;
					material->texture_sampling[texIndex].wrapping = mapmode;
					material->texture_sampling[texIndex].mapping = mapping;
				}

				switch (textureType) {
				case aiTextureType::aiTextureType_DIFFUSE:
					material->diffuseIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_NORMALS:
					material->normalIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_OPACITY:
					material->maskTextureIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_SPECULAR:
					material->specularIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_HEIGHT:
					material->heightbumpIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_AMBIENT:
					break;
				case aiTextureType::aiTextureType_EMISSIVE:
					material->emissionIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_SHININESS:
					break;
				case aiTextureType::aiTextureType_DISPLACEMENT:
					material->displacementIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_LIGHTMAP:
					break;
				case aiTextureType::aiTextureType_REFLECTION:
					material->reflectionIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_BASE_COLOR: /*	PBR.	*/
					material->diffuseIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_NORMAL_CAMERA:
					material->normalIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_EMISSION_COLOR:
					material->emissionIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_METALNESS:
					break;
				case aiTextureType::aiTextureType_DIFFUSE_ROUGHNESS:
					break;
				case aiTextureType::aiTextureType_AMBIENT_OCCLUSION:
					break;
				case aiTextureType_UNKNOWN:
					break;
				default:
					break;
				}
			}

		} /**/
	} /**/

	/*	Assign shader attributes.	*/
	{
		if (ref_material->Get(AI_MATKEY_COLOR_AMBIENT, color[0]) == aiReturn::aiReturn_SUCCESS) {
			material->ambient = color;
			material->ambient[3] = 1;
		}
		if (ref_material->Get(AI_MATKEY_COLOR_DIFFUSE, color[0]) == aiReturn::aiReturn_SUCCESS) {
			material->diffuse = color;
			material->diffuse[3] = 1;
		}
		if (ref_material->Get(AI_MATKEY_COLOR_EMISSIVE, color[0]) == aiReturn::aiReturn_SUCCESS) {
			material->emission = color;
			material->emission[3] = 1;
		}
		if (ref_material->Get(AI_MATKEY_COLOR_SPECULAR, color[0]) == aiReturn::aiReturn_SUCCESS) {
			material->specular = color;
			material->specular[3] = 1;
		}
		if (ref_material->Get(AI_MATKEY_COLOR_TRANSPARENT, color[0]) == aiReturn::aiReturn_SUCCESS) {
			material->transparent = color;
		}
		if (ref_material->Get(AI_MATKEY_REFLECTIVITY, color[0]) == aiReturn::aiReturn_SUCCESS) {
			material->reflectivity = color;
			material->reflectivity[3] = 1;
		}
		if (ref_material->Get(AI_MATKEY_SHININESS, shininessStrength) == aiReturn::aiReturn_SUCCESS) {
			material->shinininessStrength = shininessStrength;
		}

		float tmp = NAN;
		if (ref_material->Get(AI_MATKEY_SHININESS_STRENGTH, tmp) == aiReturn::aiReturn_SUCCESS) {
			//	material->shinininessStrength = tmp;
		}
		if (ref_material->Get(AI_MATKEY_BUMPSCALING, tmp) == aiReturn::aiReturn_SUCCESS) {
			//	material->shinininessStrength = tmp;
		}

		
		if (ref_material->Get(AI_MATKEY_TRANSPARENCYFACTOR, tmp) == aiReturn::aiReturn_SUCCESS) {
			material->opacity = tmp;
			material->transparent[3] = tmp;
		}
		if (ref_material->Get(AI_MATKEY_OPACITY, tmp) == aiReturn::aiReturn_SUCCESS) {
			material->opacity = tmp;
			//material->transparent[3] = tmp;
		}

		aiBlendMode blendfunc;
		if (ref_material->Get(AI_MATKEY_BLEND_FUNC, blendfunc) == aiReturn::aiReturn_SUCCESS) {
			material->blend_func_mode = blendfunc;
		}

		int twosided = 0;
		if (ref_material->Get(AI_MATKEY_TWOSIDED, twosided) == aiReturn::aiReturn_SUCCESS) {
			material->culling_both_side_mode = twosided;
		}

		aiShadingMode model;
		if (ref_material->Get(AI_MATKEY_SHADING_MODEL, model) == aiReturn::aiReturn_SUCCESS) {
			material->shade_model = model;
		}
		//_AI_MATKEY_TEXFLAGS_BASE

		int use_wireframe = 0;
		if (ref_material->Get(AI_MATKEY_ENABLE_WIREFRAME, use_wireframe) == aiReturn::aiReturn_SUCCESS) {
			material->wireframe_mode = use_wireframe;
		}
	}

	return material;
}

void ModelImporter::loadTexturesFromMaterials(aiMaterial *pmaterial) {

	/*	load all texture assoicated with material.	*/
	for (size_t textureType = aiTextureType::aiTextureType_DIFFUSE; textureType < aiTextureType::aiTextureType_UNKNOWN;
		 textureType++) {

		/*	*/
		for (size_t textureIndex = 0; textureIndex < pmaterial->GetTextureCount((aiTextureType)textureType);
			 textureIndex++) {

			/*	*/
			aiString textureName;
			int ret = pmaterial->Get(AI_MATKEY_TEXTURE(textureType, textureIndex), textureName);
			if (ret != aiReturn::aiReturn_SUCCESS) {
			}

			/*	*/
			const auto *embeededTexture = sceneRef->GetEmbeddedTexture(textureName.C_Str());

			aiString path;
			if (pmaterial->GetTexture((aiTextureType)textureType, textureIndex, &path, nullptr, nullptr, nullptr,
									  nullptr, nullptr) == aiReturn::aiReturn_SUCCESS) {

				/*	None embedded.	*/
				if (embeededTexture == nullptr) {

					/*	Check if file exists.	*/
					if (this->textureMapping.find(path.C_Str()) == this->textureMapping.end()) {

						TextureAssetObject textureUp;
						textureUp.filepath = fmt::format("{0}/{1}", this->filepath, path.C_Str());
						std::replace(textureUp.filepath.begin(), textureUp.filepath.end(), '\\', '/');

						/*	add texture.	*/
						this->textures.push_back(textureUp);
						this->textureMapping[path.C_Str()] = &textures.back();
						this->textureIndexMapping[path.C_Str()] = this->textureIndexMapping.size();
					}
				}
			} else {
			}

		} /*	*/
	} /*	*/
}

AnimationObject *ModelImporter::initAnimation(const aiAnimation *pAnimation, unsigned int index) {

	AnimationObject animation_clip = AnimationObject();

	animation_clip.name = pAnimation->mName.C_Str();

	unsigned int channel_index = 0;

	animation_clip.duration = pAnimation->mDuration;

	for (size_t i = 0; i < pAnimation->mNumChannels; i++) {
		const aiNodeAnim *nodeAnimation = pAnimation->mChannels[i];

		if (nodeAnimation->mNumPositionKeys > 0) {
			Curve positionCurve;

			positionCurve.name = nodeAnimation->mNodeName.C_Str();
			positionCurve.keyframes.resize(nodeAnimation->mNumPositionKeys);

			for (unsigned int x = 0; x < nodeAnimation->mNumPositionKeys; x++) {
				KeyFrame key;
				key.time = nodeAnimation->mPositionKeys[x].mTime;
				key.value = nodeAnimation->mPositionKeys[x].mValue.x;
			}
			animation_clip.curves.push_back(positionCurve);
		}

		if (nodeAnimation->mNumRotationKeys > 0) {

			Curve rotation_curve;

			rotation_curve.name = nodeAnimation->mNodeName.C_Str();
			rotation_curve.keyframes.resize(nodeAnimation->mNumRotationKeys);

			for (unsigned int x = 0; x < nodeAnimation->mNumRotationKeys; x++) {
				KeyFrame key;
				key.time = nodeAnimation->mRotationKeys[x].mTime;
				key.value = nodeAnimation->mRotationKeys[x].mValue.x;
			}
			animation_clip.curves.push_back(rotation_curve);
		}

		if (nodeAnimation->mNumScalingKeys > 0) {

			Curve scale_curve;

			scale_curve.name = nodeAnimation->mNodeName.C_Str();
			scale_curve.keyframes.resize(nodeAnimation->mNumScalingKeys);

			for (unsigned int x = 0; x < nodeAnimation->mNumRotationKeys; x++) {
				KeyFrame key;
				key.time = nodeAnimation->mScalingKeys[x].mTime;
				key.value = nodeAnimation->mScalingKeys[x].mValue.x;
			}
			animation_clip.curves.push_back(scale_curve);
		}
	}

	/*	Mesh.	*/
	for (unsigned int i = 0; i < pAnimation->mNumMeshChannels; i++) {
	}

	/*	Morph.	*/
	for (unsigned int i = 0; i < pAnimation->mNumMorphMeshChannels; i++) {
	}

	this->animations.push_back(animation_clip);

	return &this->animations.back();
}

LightObject *ModelImporter::initLight(const aiLight *light, unsigned int index) {
	LightObject *lightOb = &this->lights[index];

	lightOb->name = light->mName.C_Str();

	lightOb->position = glm::vec3(light->mPosition.x, light->mPosition.y, light->mPosition.z);
	lightOb->direction = glm::vec3(light->mDirection.x, light->mDirection.y, light->mDirection.z);
	lightOb->mUp = glm::vec3(light->mUp.x, light->mUp.y, light->mUp.z);

	lightOb->mColorDiffuse = glm::vec4(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b, 1);

	return lightOb;
}

TextureAssetObject *ModelImporter::initTexture(aiTexture *texture, unsigned int index) {
	TextureAssetObject *mTexture = &this->textures[index];

	mTexture->width = texture->mWidth;
	mTexture->height = texture->mHeight;

	if (mTexture->height == 0) {
		mTexture->dataSize = texture->mWidth;
	} else if (texture->pcData != nullptr) {
		mTexture->dataSize = static_cast<size_t>(texture->mWidth * texture->mHeight) * 4;
	}
	mTexture->filepath = texture->mFilename.C_Str();

	if (texture->pcData != nullptr) {
		mTexture->data = (char *)malloc(mTexture->dataSize);
		memcpy(mTexture->data, texture->pcData, mTexture->dataSize);
	}
	return mTexture;
}

struct Face {
	uint Indices[3];
};

void ModelImporter::convert2Adjcent(const aiMesh *paiMesh, std::vector<unsigned int> &Indices) {}

NodeObject *ModelImporter::getNodeByName(const std::string &name) const noexcept {
	if (this->nodeByName.find(name) != this->nodeByName.end()) {
		return nodeByName.at(name);
	}
	return nullptr;
}

std::vector<MaterialObject *> ModelImporter::getMaterials(const size_t texture_index) noexcept {
	std::vector<MaterialObject *> ref_materials;

	for (size_t i = 0; i < getMaterials().size(); i++) {
		bool found = false;
		if (getMaterials()[i].diffuseIndex == (int)texture_index) {
			found = true;
		}
		if (getMaterials()[i].normalIndex == (int)texture_index) {
			found = true;
		}
		if (getMaterials()[i].emissionIndex == (int)texture_index) {
			found = true;
		}
		if (getMaterials()[i].heightbumpIndex == (int)texture_index) {
			found = true;
		}
		if (getMaterials()[i].specularIndex == (int)texture_index) {
			found = true;
		}
		if (getMaterials()[i].displacementIndex == (int)texture_index) {
			found = true;
		}
		if (getMaterials()[i].maskTextureIndex == (int)texture_index) {
			found = true;
		}
		if (getMaterials()[i].ambientOcclusionIndex == (int)texture_index) {
			found = true;
		}
		// TODO: add more

		/*	*/
		if (found) {
			ref_materials.push_back(&this->materials[i]);
		}
	}
	return ref_materials;
}
