#include "ModelImporter.h"
#include "Core/Math3D.h"
#include "Core/math3D/AABB.h"
#include "GeometryUtil.h"
#include <Core/IO/IOUtil.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>
#include <cstdint>
#include <filesystem>
#include <glm/fwd.hpp>

namespace fs = std::filesystem;
using namespace fragcore;

static inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4 *from) {
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

void ModelImporter::loadContent(const std::string &path, unsigned long int supportFlag) {
	Importer importer;

	/*	Load file content. */
	Ref<IO> io = Ref<IO>(fileSystem->openFile(path.c_str(), IO::IOMode::READ));
	std::vector<char> bufferIO = fragcore::IOUtil::readFile<char>(io);
	io->close();
	bufferIO.clear();
	this->filepath = fs::path(fileSystem->getAbsolutePath(path.c_str())).parent_path();

	/*	*/
	const aiScene *pScene = importer.ReadFile(path.c_str(), aiProcessPreset_TargetRealtime_Quality);

	if (pScene == nullptr) {
		throw RuntimeException("Failed to load file: {} - Error: {}", path, importer.GetErrorString());
	}

	/*	*/
	this->sceneRef = (aiScene *)pScene;

	if (pScene) {

		/*	inverse.	*/
		aiMatrix4x4 m_GlobalInverseTransform = pScene->mRootNode->mTransformation;
		m_GlobalInverseTransform.Inverse();

		this->initScene(pScene);
		importer.FreeScene();
	} else {
		throw RuntimeException("Failed to load model: {}", path);
	}
}

void ModelImporter::clear() {
	// foreach all assets and clean.
	this->nodes.clear();
	this->models.clear();
	this->materials.clear();
	this->textures.clear();
	this->animations.clear();
}

void ModelImporter::initScene(const aiScene *scene) {

	/*	*/
	std::thread process_textures_thread([&]() {
		// /*	*/
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
		}
	});

	/*	*/
	std::thread process_animation_thread([&]() {
		if (scene->HasAnimations()) {
			for (size_t x = 0; x < scene->mNumAnimations; x++) {
				this->initAnimation(scene->mAnimations[x], x);
			}
		}
	});

	if (scene->HasLights()) {
		for (unsigned int x = 0; x < scene->mNumLights; x++) {
			// this->initLight(scene->mLights[x], x);
		}
	}

	if (scene->HasCameras()) {
		for (unsigned int x = 0; x < scene->mNumCameras; x++) {
		}
	}

	/*	Wait intill done.	*/
	process_model_thread.join();

	/*	Compute bonding box.	*/
	std::thread process_bounding_boxes([&]() {
		if (scene->HasMeshes()) {

			for (size_t x = 0; x < scene->mNumMeshes; x++) {
				// Compute bounding box.
				this->models[x].boundingBox = GeometryUtility::computeBoundingBox(
					(const Vector3 *)scene->mMeshes[x]->mVertices, scene->mMeshes[x]->mNumVertices, sizeof(aiVector3D));
			}
		}
	});

	process_textures_thread.join();
	process_animation_thread.join();

	// /*	*/
	if (scene->HasMaterials()) {
		/*	Extract additional texture */

		this->materials.resize(scene->mNumMaterials);
		for (size_t x = 0; x < scene->mNumMaterials; x++) {
			this->initMaterial(scene->mMaterials[x], x);
		}
	}

	this->initNoodeRoot(scene->mRootNode);

	process_bounding_boxes.join();
}

void ModelImporter::initNoodeRoot(const aiNode *nodes, NodeObject *parent) {
	size_t gameObjectCount = 0;
	size_t meshCount = 0;

	/*	iterate through each child of parent node.	*/
	for (size_t x = 0; x < nodes->mNumChildren; x++) {
		unsigned int meshCount = 0;
		aiVector3D position, scale;
		aiQuaternion rotation;

		NodeObject *pobject = new NodeObject();

		/*	extract position, rotation, position from transformation matrix.	*/
		nodes->mTransformation.Decompose(scale, rotation, position);
		if (parent) {
			pobject->parent = parent;
		} else {
			pobject->parent = nullptr;
		}

		/*	TODO check if works.	*/
		pobject->position = glm::vec3(position.x, position.y, position.z);
		pobject->rotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
		pobject->scale = glm::vec3(scale.x, scale.y, scale.z);
		pobject->name = nodes->mChildren[x]->mName.C_Str();

		/*	*/
		if (nodes->mChildren[x]->mMeshes) {

			/*	*/ // TODO: compute the bounding box.
			for (size_t y = 0; y < nodes->mChildren[x]->mNumMeshes; y++) {

				/*	Get material for mesh object.	*/
				const MaterialObject &materialRef =
					getMaterials()[this->sceneRef->mMeshes[*nodes->mChildren[x]->mMeshes]->mMaterialIndex];

				/*	*/
				pobject->materialIndex.push_back(
					this->sceneRef->mMeshes[nodes->mChildren[x]->mMeshes[y]]->mMaterialIndex);

				pobject->geometryObjectIndex.push_back(nodes->mChildren[x]->mMeshes[y]);
			}
		}

		this->nodes.push_back(pobject);

		/*	*/
		this->initNoodeRoot(nodes->mChildren[x], pobject);
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

	size_t z;

	/*	*/ // TODO: compute vertexSize
	const size_t vertexSize = sizeof(float) * 3;
	const size_t uvSize = sizeof(float) * 2;
	const size_t normalSize = sizeof(float) * 3;
	const size_t tangentSize = sizeof(float) * 3;

	const size_t boudWeightCount = 4;
	size_t boneSize = 0;
	if (aimesh->HasBones()) {
		boneSize = sizeof(float) * boudWeightCount + sizeof(unsigned int) * boudWeightCount;
	}

	const size_t StrideSize = vertexSize + uvSize + normalSize + tangentSize + boneSize;
	const size_t indicesSize = 4;

	/*	*/
	unsigned int VertexIndex = 0, IndicesIndex = 0, bonecount = 0, initilzebone = 0;

	/*	*/
	float *vertices = (float *)malloc(aimesh->mNumVertices * StrideSize);
	unsigned char *Indice = (unsigned char *)malloc(indicesSize * aimesh->mNumFaces * 3);

	/*	*/
	float *temp = vertices;
	unsigned char *Itemp = Indice;

	/*	*/
	const aiVector3D Zero = aiVector3D(0, 0, 0);

	for (unsigned int x = 0; x < aimesh->mNumVertices; x++) {

		/*	*/
		const aiVector3D *Pos = &(aimesh->mVertices[x]);
		aiVector3D *pNormal = &(aimesh->mNormals[x]);

		const aiVector3D *pTexCoord = aimesh->HasTextureCoords(0) ? &(aimesh->mTextureCoords[0][x]) : &Zero;

		/*	*/
		*vertices++ = Pos->x;
		*vertices++ = Pos->y;
		*vertices++ = Pos->z;

		/*	*/
		*vertices++ = pTexCoord->x;
		*vertices++ = pTexCoord->y;
		// uvs[x] = pTexCoord->x;
		// uvs[x] = pTexCoord->y;

		/*	*/
		pNormal->Normalize();
		*vertices++ = pNormal->x;
		*vertices++ = pNormal->y;
		*vertices++ = pNormal->z;

		/*	*/
		*vertices++ = aimesh->mTangents->x;
		*vertices++ = aimesh->mTangents->y;
		*vertices++ = aimesh->mTangents->z;

		/*	*/
		if (boneSize > 0) {
			*vertices += bonecount; /*	Bone ID.	*/
			*vertices += bonecount; /*	Vertex Weight.	*/
		}

	} /**/

	vertices = temp;

	/*	Load bones.	*/
	if (aimesh->HasBones()) {

		pmesh->boneOffset = 32; // TODO:Fix.
		for (uint i = 0; i < aimesh->mNumBones; i++) {
			const uint BoneIndex = 0;

			std::string BoneName(aimesh->mBones[i]->mName.data);

			for (uint j = 0; j < aimesh->mBones[i]->mNumWeights; j++) {

				const uint VertexID = aimesh->mBones[i]->mWeights[j].mVertexId;
				const float Weight = aimesh->mBones[i]->mWeights[j].mWeight;

				vertices[VertexID * (StrideSize / sizeof(float))] = BoneIndex;
				vertices[VertexID * (StrideSize / sizeof(float)) + 1] = Weight;
			}
		}
	}

	for (size_t x = 0; x < aimesh->mNumFaces; x++) {
		const aiFace &face = aimesh->mFaces[x];
		if (face.mNumIndices == 3) {
			// TODO: fix old code.
			memcpy(Indice, &face.mIndices[0], indicesSize);
			Indice += indicesSize;
			memcpy(Indice, &face.mIndices[1], indicesSize);
			Indice += indicesSize;
			memcpy(Indice, &face.mIndices[2], indicesSize);
			Indice += indicesSize;

		} else if (face.mNumIndices == 2) {
		}
	}

	Indice = Itemp;

	pmesh->indicesData = Indice;
	pmesh->indicesStride = indicesSize;
	pmesh->nrIndices = aimesh->mNumFaces * 3;
	pmesh->nrVertices = aimesh->mNumVertices;
	pmesh->vertexData = vertices;
	pmesh->vertexStride = StrideSize;

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
			}

			aiTextureFlags textureFlag;
			if (pmaterial->Get(AI_MATKEY_TEXFLAGS(textureType, textureIndex), textureFlag) ==
				aiReturn::aiReturn_SUCCESS) {
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
					break;
				case aiTextureType::aiTextureType_HEIGHT:
					material->heightIndex = texIndex;
					break;
				case aiTextureType::aiTextureType_AMBIENT:
					break;
				case aiTextureType::aiTextureType_EMISSIVE:
					break;
				case aiTextureType::aiTextureType_SHININESS:
					break;
				case aiTextureType::aiTextureType_DISPLACEMENT:
					break;
				case aiTextureType::aiTextureType_LIGHTMAP:
					break;
				case aiTextureType::aiTextureType_REFLECTION:
					break;
				case aiTextureType::aiTextureType_BASE_COLOR:
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

				/*	Texture property.	*/
			}

		} /**/
	}	  /**/

	/*	Assign shader attributes.	*/
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
	if (pmaterial->Get(AI_MATKEY_OPACITY, tmp) == aiReturn::aiReturn_SUCCESS) {
		material->opacity = tmp;
	}

	aiBlendMode blendfunc;
	if (pmaterial->Get(AI_MATKEY_BLEND_FUNC, blendfunc) == aiReturn::aiReturn_SUCCESS) {
	}

	int twosided;
	if (pmaterial->Get(AI_MATKEY_TWOSIDED, twosided) == aiReturn::aiReturn_SUCCESS) {
		material->culling_mode = twosided;
	}

	aiShadingMode model;
	if (pmaterial->Get(AI_MATKEY_SHADING_MODEL, model) == aiReturn::aiReturn_SUCCESS) {
		material->shade_model = model;
	}

	int use_wireframe;
	if (pmaterial->Get(AI_MATKEY_ENABLE_WIREFRAME, use_wireframe) == aiReturn::aiReturn_SUCCESS) {
		material->wireframe_mode = model;
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

		} /**/
	}	  /**/
}
AnimationObject *ModelImporter::initAnimation(const aiAnimation *panimation, unsigned int index) {

	AnimationObject clip = AnimationObject();

	clip.name = panimation->mName.C_Str();

	unsigned int channel_index = 0;

	// for (unsigned int x = 0; x < panimation->mNumChannels; x++) {
	//	this->initAnimationPosition(panimation->mChannels[x], clip);
	//	this->initAnimationRotation(panimation->mChannels[x], clip);
	//	this->initAnimationScale(panimation->mChannels[x], clip);
	//}

	this->animations.push_back(clip);

	return &this->animations.back();
}

void ModelImporter::loadCurve(aiNodeAnim *nodeAnimation, Curve *curve) {

	if (nodeAnimation->mNumPositionKeys <= 0) {
		return;
	}

	curve->keyframes.resize(nodeAnimation->mNumPositionKeys);

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
	TextureAssetObject *mTexture = &textures[index];
	mTexture->width = texture->mWidth;
	mTexture->height = texture->mHeight;
	if (mTexture->height == 0) {
		mTexture->dataSize = texture->mWidth;
	}
	mTexture->filepath = texture->mFilename.C_Str();
	mTexture->data = (char *)texture->pcData;
	return mTexture;
}
