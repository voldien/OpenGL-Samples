#include "ModelImporter.h"
#include "Core/Math.h"
#include "assimp/Importer.hpp"
#include <Core/IO/IOUtil.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>
#include <cstdint>
#include <filesystem>
#include <glm/fwd.hpp>
#include <sys/types.h>
#include <utility>

namespace fs = std::filesystem;
using namespace fragcore;

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

ModelImporter::ModelImporter(ModelImporter &&other) {
	this->fileSystem = std::exchange(other.fileSystem, nullptr);

	this->global = other.global;
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
}

ModelImporter &ModelImporter::operator=(ModelImporter &&other) {
	this->fileSystem = std::exchange(other.fileSystem, nullptr);

	this->global = other.global;
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

void ModelImporter::loadContent(const std::string &path, unsigned long int supportFlag) {
	Assimp::Importer importer;

	this->filepath = fs::path(fileSystem->getAbsolutePath(path.c_str())).parent_path();

	/*	*/
	const aiScene *pScene =
		importer.ReadFile(path.c_str(), aiProcessPreset_TargetRealtime_Quality | aiProcess_GenBoundingBoxes);

	if (pScene == nullptr) {
		throw RuntimeException("Failed to load file: {} - Error: {}", path, importer.GetErrorString());
	}

	/*	*/
	this->sceneRef = (aiScene *)pScene;

	if (pScene) {

		this->global = aiMatrix4x4ToGlm(&pScene->mRootNode->mTransformation);

		this->initScene(pScene);

	} else {
		throw RuntimeException("Failed to load model: {}", path);
	}
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
}

void ModelImporter::initScene(const aiScene *scene) {

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
	});

	/*	*/
	std::thread process_model_thread([&]() {
		/*	 */
		if (scene->HasMeshes()) {

			this->models.resize(scene->mNumMeshes);

			/*	*/
			for (size_t x = 0; x < scene->mNumMeshes; x++) {
				this->initMesh(scene->mMeshes[x], x);
			}

			/*	*/
			for (size_t x = 0; x < scene->mNumMeshes; x++) {
				this->initBoneSkeleton(scene->mMeshes[x], x);
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
		}
	});

	/*	*/
	std::thread process_animation_light_camera_thread([&]() {
		if (scene->HasAnimations()) {
			for (size_t x = 0; x < scene->mNumAnimations; x++) {
				this->initAnimation(scene->mAnimations[x], x);
			}
		}

		if (scene->HasLights()) {
			for (unsigned int x = 0; x < scene->mNumLights; x++) {
				this->initLight(scene->mLights[x], x);
			}
		}

		if (scene->HasCameras()) {
			for (unsigned int x = 0; x < scene->mNumCameras; x++) {
			}
		}
	});

	/*	Wait intill done.	*/
	process_model_thread.join();

	process_textures_thread.join();
	process_animation_light_camera_thread.join();

	/*	*/
	if (scene->HasMaterials()) {
		/*	Extract additional texture */

		this->materials.resize(scene->mNumMaterials);
		for (size_t x = 0; x < scene->mNumMaterials; x++) {
			this->initMaterial(scene->mMaterials[x], x);
		}
	}

	this->initNoodeRoot(scene->mRootNode, nullptr);
}

void ModelImporter::initNoodeRoot(const aiNode *node, NodeObject *parent) {
	size_t gameObjectCount = 0;
	size_t meshCount = 0;

	/*	iterate through each child of parent node.	*/
	for (size_t x = 0; x < node->mNumChildren; x++) {
		unsigned int meshCount = 0;
		aiVector3D position, scale;
		aiQuaternion rotation;

		NodeObject *pobject = new NodeObject();

		/*	extract position, rotation, position from transformation matrix.	*/
		node->mChildren[x]->mTransformation.Decompose(scale, rotation, position);
		if (parent) {
			pobject->parent = parent;
		} else {
			pobject->parent = nullptr;
		}

		// TODO, resolve relative to world transform model matrix.
		pobject->localPosition = glm::vec3(position.x, position.y, position.z);
		pobject->localRotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
		pobject->localScale = glm::vec3(scale.x, scale.y, scale.z);

		pobject->modelLocalTransform = aiMatrix4x4ToGlm(&node->mChildren[x]->mTransformation);

		if (parent) {
			pobject->modelGlobalTransform = parent->modelGlobalTransform * pobject->modelLocalTransform;
		} else {
			pobject->modelGlobalTransform = this->globalTransform() * pobject->modelLocalTransform;
		}

		pobject->name = node->mChildren[x]->mName.C_Str();

		/*	*/
		if (node->mChildren[x]->mMeshes) {

			/*	*/
			for (size_t y = 0; y < node->mChildren[x]->mNumMeshes; y++) {

				/*	Get material for mesh object.	*/
				const MaterialObject &materialRef =
					getMaterials()[this->sceneRef->mMeshes[*node->mChildren[x]->mMeshes]->mMaterialIndex];

				/*	*/
				const int meshIndex = node->mChildren[x]->mMeshes[y];
				pobject->materialIndex.push_back(this->sceneRef->mMeshes[meshIndex]->mMaterialIndex);

				pobject->geometryObjectIndex.push_back(meshIndex);

				// TODO: compute max volume.*
				pobject->bound = this->models[meshIndex].bound;
			}
		}

		this->nodes.push_back(pobject);

		/*	*/
		this->initNoodeRoot(node->mChildren[x], pobject);
	}
}

SkeletonSystem *ModelImporter::initBoneSkeleton(const aiMesh *mesh, unsigned int index) {

	SkeletonSystem skeleton;

	/*	Load bones.	*/
	if (mesh->HasBones()) {

		for (uint32_t i = 0; i < mesh->mNumBones; i++) {
			const uint32_t BoneIndex = 0;
			const std::string BoneName(mesh->mBones[i]->mName.data);

			glm::mat4 to = aiMatrix4x4ToGlm(&mesh->mBones[i]->mOffsetMatrix);

			Bone bone;
			bone.inverseBoneMatrix = to;
			bone.name = BoneName;
			bone.boneIndex = i;

			skeleton.bones[BoneName] = bone;
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
	const size_t uvSize = nrUVs * sizeof(float) * 2;
	const size_t normalSize = sizeof(float) * 3;
	const size_t tangentSize = sizeof(float) * 3;

	/*	*/
	const size_t boneWeightCount = 4;
	size_t boneByteSize = 0;
	size_t bonecount = 0;
	if (aimesh->HasBones()) {
		boneByteSize = sizeof(float) * boneWeightCount + sizeof(unsigned int) * boneWeightCount;
		bonecount = boneWeightCount;
	}

	/*	*/
	const size_t StrideSize = vertexSize + uvSize + normalSize + tangentSize + boneByteSize;
	const uint floatStride = StrideSize / sizeof(float);

	const size_t indicesSize = 4;

	// TODO: add
	aimesh->HasTextureCoords(0);

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

			float *pVertex = &vertices[floatStride * x];

			/*	*/
			const aiVector3D *Pos = &(aimesh->mVertices[x]);
			aiVector3D *pNormal = &(aimesh->mNormals[x]);
			const aiVector3D *Tangent = aimesh->mTangents ? &(aimesh->mTangents[x]) : nullptr;

			/*	Vertex position.	*/

			*pVertex++ = Pos->x;
			*pVertex++ = Pos->y;
			*pVertex++ = Pos->z;

			/*	*/
			if (aimesh->GetNumUVChannels() > 0) {
				for (unsigned int i = 0; i < aimesh->GetNumUVChannels(); i++) {
					if (aimesh->HasTextureCoords(i)) {
						*pVertex++ = aimesh->mTextureCoords[i][x].x;
						*pVertex++ = aimesh->mTextureCoords[i][x].y;
					}
				}
			} else {
				*pVertex++ = 0;
				*pVertex++ = 0;
			}

			/*	*/
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

			/*	Offset only. assign later.	*/
			if (boneByteSize > 0 && bonecount > 0) {
				/*	BoneID	*/
				for (int i = 0; i < bonecount; i++) {
					*pVertex++ = 0;
				}
				/*	BoneWeight	*/
				for (int i = 0; i < bonecount; i++) {
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

		pmesh->boneIndexOffset = (vertexSize + uvSize + normalSize + tangentSize);
		pmesh->boneWeightOffset = (vertexSize + uvSize + normalSize + tangentSize + bonecount * sizeof(unsigned int));

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

	aimesh->HasFaces();
	size_t nrFaces = 0;
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
		for (int i = 0; i < aimesh->mNumAnimMeshes; i++) {
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

	pmesh->indicesData = Indice;
	pmesh->indicesStride = indicesSize;
	pmesh->nrIndices = nrFaces;
	pmesh->nrVertices = aimesh->mNumVertices;
	pmesh->vertexData = vertices;
	pmesh->vertexStride = StrideSize;
	pmesh->primitiveType = aimesh->mPrimitiveTypes;
	pmesh->name = std::move(std::string(aimesh->mName.C_Str()));

	return pmesh;
}

MaterialObject *ModelImporter::initMaterial(aiMaterial *pmaterial, size_t index) {

	char absolutepath[PATH_MAX];

	char *data;
	aiString name;
	aiString path;

	aiTextureMapping mapping;
	unsigned int uvindex;
	float blend;
	aiTextureOp op;
	aiTextureMapMode mapmode = aiTextureMapMode::aiTextureMapMode_Wrap;

	float specular;

	glm::vec4 color;
	float shininessStrength;

	MaterialObject *material = &this->materials[index];

	if (!pmaterial) {
		return nullptr;
	}

	const bool isTextureEmpty = this->textures.size() == 0;

	/*	*/
	pmaterial->Get(AI_MATKEY_NAME, name);
	material->name = name.C_Str();

	/*	load all texture assoicated with material.	*/
	for (size_t textureType = aiTextureType::aiTextureType_DIFFUSE; textureType < aiTextureType::aiTextureType_UNKNOWN;
		 textureType++) {

		/*	*/
		for (size_t textureIndex = 0; textureIndex < pmaterial->GetTextureCount((aiTextureType)textureType);
			 textureIndex++) {

			/*	*/
			aiString textureName;
			if (pmaterial->Get(AI_MATKEY_TEXTURE(textureType, textureIndex), textureName) ==
				aiReturn::aiReturn_SUCCESS) {
				/*	*/
			}

			aiTextureFlags textureFlag;
			if (pmaterial->Get(AI_MATKEY_TEXFLAGS(textureType, textureIndex), textureFlag) ==
				aiReturn::aiReturn_SUCCESS) {
				/*	*/
			}

			/*	*/
			auto *embeededTexture = sceneRef->GetEmbeddedTexture(textureName.C_Str());

			if (pmaterial->GetTexture((aiTextureType)textureType, textureIndex, &path, &mapping, &uvindex, &blend, &op,
									  &mapmode) == aiReturn::aiReturn_SUCCESS) {

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
					break;
				case aiTextureType::aiTextureType_LIGHTMAP:
					break;
				case aiTextureType::aiTextureType_REFLECTION:
					break;
				case aiTextureType::aiTextureType_BASE_COLOR: /*	PBR.	*/
					break;
				case aiTextureType::aiTextureType_NORMAL_CAMERA:
					break;
				case aiTextureType::aiTextureType_EMISSION_COLOR:
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
	}	  /**/

	/*	Assign shader attributes.	*/

	/*	Color.	*/
	if (pmaterial->Get(AI_MATKEY_COLOR_AMBIENT, color[0]) == aiReturn::aiReturn_SUCCESS) {
		material->ambient = color;
	}
	if (pmaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color[0]) == aiReturn::aiReturn_SUCCESS) {
		material->diffuse = color;
	}
	if (pmaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color[0]) == aiReturn::aiReturn_SUCCESS) {
		material->emission = color;
	}
	if (pmaterial->Get(AI_MATKEY_COLOR_SPECULAR, color[0]) == aiReturn::aiReturn_SUCCESS) {
		material->specular = color;
	}
	if (pmaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, color[0]) == aiReturn::aiReturn_SUCCESS) {
		material->transparent = color;
	}
	if (pmaterial->Get(AI_MATKEY_REFLECTIVITY, color[0]) == aiReturn::aiReturn_SUCCESS) {
		material->reflectivity = color;
	}
	if (pmaterial->Get(AI_MATKEY_SHININESS, shininessStrength) == aiReturn::aiReturn_SUCCESS) {
		material->shinininessStrength = shininessStrength;
	}

	float tmp;
	if (pmaterial->Get(AI_MATKEY_SHININESS_STRENGTH, tmp) == aiReturn::aiReturn_SUCCESS) {
		//	material->shinininessStrength = tmp;
	}
	if (pmaterial->Get(AI_MATKEY_BUMPSCALING, tmp) == aiReturn::aiReturn_SUCCESS) {
		//	material->shinininessStrength = tmp;
	}
	if (pmaterial->Get(AI_MATKEY_SHININESS, tmp) == aiReturn::aiReturn_SUCCESS) {
		//	material->shinininessStrength = tmp;
	}
	if (pmaterial->Get(AI_MATKEY_REFLECTIVITY, tmp) == aiReturn::aiReturn_SUCCESS) {
		//	material->shinininessStrength = tmp;
	}
	if (pmaterial->Get(AI_MATKEY_OPACITY, tmp) == aiReturn::aiReturn_SUCCESS) {
		material->opacity = tmp;
	}

	aiBlendMode blendfunc;
	if (pmaterial->Get(AI_MATKEY_BLEND_FUNC, blendfunc) == aiReturn::aiReturn_SUCCESS) {
		material->blend_mode = blendfunc;
	}

	int twosided;
	if (pmaterial->Get(AI_MATKEY_TWOSIDED, twosided) == aiReturn::aiReturn_SUCCESS) {
		material->culling_both_side_mode = twosided;
	}

	aiShadingMode model;
	if (pmaterial->Get(AI_MATKEY_SHADING_MODEL, model) == aiReturn::aiReturn_SUCCESS) {
		material->shade_model = model;
	}
	//_AI_MATKEY_TEXFLAGS_BASE

	int use_wireframe;
	if (pmaterial->Get(AI_MATKEY_ENABLE_WIREFRAME, use_wireframe) == aiReturn::aiReturn_SUCCESS) {
		material->wireframe_mode = use_wireframe;
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

			/*	*/
			auto *embeededTexture = sceneRef->GetEmbeddedTexture(textureName.C_Str());

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
	}	  /*	*/
}

AnimationObject *ModelImporter::initAnimation(const aiAnimation *pAnimation, unsigned int index) {

	AnimationObject clip = AnimationObject();

	clip.name = pAnimation->mName.C_Str();

	unsigned int channel_index = 0;

	clip.duration = pAnimation->mDuration;

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

				positionCurve.keyframes[x];
			}
			clip.curves.push_back(positionCurve);
		}

		if (nodeAnimation->mNumRotationKeys > 0) {
			for (unsigned int x = 0; x < nodeAnimation->mNumRotationKeys; x++) {
			}
		}

		if (nodeAnimation->mNumScalingKeys > 0) {
			for (unsigned int x = 0; x < nodeAnimation->mNumScalingKeys; x++) {
			}
		}
	}

	// for (unsigned int x = 0; x < panimation->mNumChannels; x++) {
	//	this->initAnimationPosition(panimation->mChannels[x], clip);
	//	this->initAnimationRotation(panimation->mChannels[x], clip);
	//	this->initAnimationScale(panimation->mChannels[x], clip);
	//}

	this->animations.push_back(clip);

	return &this->animations.back();
}
LightObject *ModelImporter::initLight(const aiLight *light, unsigned int index) {
	LightObject *lightOb;

	return lightOb;
}

void ModelImporter::loadCurve(aiNodeAnim *nodeAnimation, Curve *curve) {

	if (nodeAnimation->mNumPositionKeys <= 0) {
		return;
	}

	// nodeAnimation->mNodeName

	// curve->addCurve(VDCurve(position->mNumPositionKeys));
	// curve->addCurve(VDCurve(position->mNumPositionKeys));
	// curve->addCurve(VDCurve(position->mNumPositionKeys));
	//
	// curve->getCurve(curve->getNumCurves() - 3).setCurveFlag(VDCurve::eTransformPosX);
	// curve->getCurve(curve->getNumCurves() - 2).setCurveFlag(VDCurve::eTransformPosY);
	// animationClip->getCurve(animationClip->getNumCurves() - 1).setCurveFlag(VDCurve::eTransformPosZ);
	//
	// animationClip->getCurve(animationClip->getNumCurves() - 3).setName(position->mNodeName.data);
	// animationClip->getCurve(animationClip->getNumCurves() - 2).setName(position->mNodeName.data);
	// animationClip->getCurve(animationClip->getNumCurves() - 1).setName(position->mNodeName.data);
	//
	// for (unsigned int x = 0; x < position->mNumPositionKeys; x++) {
	//	// curve Position X
	//	animationClip->getCurve(animationClip->getNumCurves() - 3)
	//		.getKey(x)
	//		.setValue(position->mPositionKeys[x].mValue.x);
	//	animationClip->getCurve(animationClip->getNumCurves() -
	// 3).getKey(x).setTime(position->mPositionKeys[x].mTime);
	//
	//	// curve Position Y
	//	animationClip->getCurve(animationClip->getNumCurves() - 2)
	//		.getKey(x)
	//		.setValue(position->mPositionKeys[x].mValue.y);
	//	animationClip->getCurve(animationClip->getNumCurves() -
	// 2).getKey(x).setTime(position->mPositionKeys[x].mTime);
	//	// curve Position Z
	//	animationClip->getCurve(animationClip->getNumCurves() - 1)
	//		.getKey(x)
	//		.setValue(position->mPositionKeys[x].mValue.z);
	//	animationClip->getCurve(animationClip->getNumCurves() -
	// 1).getKey(x).setTime(position->mPositionKeys[x].mTime);
	//}
}

TextureAssetObject *ModelImporter::initTexture(aiTexture *texture, unsigned int index) {
	TextureAssetObject *mTexture = &this->textures[index];

	mTexture->width = texture->mWidth;
	mTexture->height = texture->mHeight;

	if (mTexture->height == 0) {
		mTexture->dataSize = texture->mWidth;
	} else if (texture->pcData != nullptr) {
		mTexture->dataSize = texture->mWidth * texture->mHeight * 4;
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
