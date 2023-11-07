#include "ModelImporter.h"
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <filesystem>
namespace fs = std::filesystem;

void ModelImporter::loadContent(const std::string &path, unsigned long int supportFlag) {
	Importer importer;

	/*	Load file content. */
	Ref<IO> io = Ref<IO>(fileSystem->openFile(path.c_str(), IO::IOMode::READ));
	std::vector<char> bufferIO = IOUtil::readFile<char>(io);
	io->close();
	bufferIO.clear();
	this->filepath = fs::path(fileSystem->getAbsolutePath(path.c_str())).parent_path();

	/*  */
	//	const aiScene *pScene = importer.ReadFileFromMemory(
	//		bufferIO.data(), bufferIO.size(), aiProcessPreset_TargetRealtime_Quality);
	//		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_ValidateDataStructure |
	//			aiProcess_SplitLargeMeshes | aiProcess_FindInstances | aiProcess_EmbedTextures);
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
	});

	/*	*/
	std::thread process_model_thread([&]() {
		/*	 */
		if (scene->HasMeshes()) {
			this->models.resize(scene->mNumMeshes);
			for (size_t x = 0; x < scene->mNumMeshes; x++) {
				this->initMesh(scene->mMeshes[x], x);
			}
		}
	});

	/*	*/
	std::thread process_animation_thread([&]() {
		/*	*/
		if (scene->HasAnimations()) {
			this->animations.reserve(scene->mNumAnimations);
			for (unsigned int x = 0; x < scene->mNumAnimations; x++) {
				//	this->initAnimation(scene->mAnimations[x], x);
			}
		}
	});

	/*	Wait intill done.	*/
	process_model_thread.join();
	process_textures_thread.join();
	process_animation_thread.join();

	// /*	*/
	if (scene->HasMaterials()) {
		for (size_t x = 0; x < scene->mNumMaterials; x++) {
			this->loadTexturesFromMaterials(scene->mMaterials[x]);
		}

		this->materials.resize(scene->mNumMaterials);
		for (size_t x = 0; x < scene->mNumMaterials; x++) {
			this->initMaterial(scene->mMaterials[x], x);
		}
	}

	// /*	*/
	// if (scene->HasLights()) {
	// 	for (unsigned int x = 0; x < scene->mNumLights; x++) {
	// 		this->initLight(scene->mLights[x], x);
	// 	}
	// }

	this->initNoodeRoot(scene->mRootNode);
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

			/*	*/
			for (size_t y = 0; y < nodes->mChildren[x]->mNumMeshes; y++) {

				/*	Get material for mesh object.	*/
				MaterialObject &materialRef =
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

ModelSystemObject *ModelImporter::initMesh(const aiMesh *aimesh, unsigned int index) {
	ModelSystemObject *pmesh = &this->models[index];

	size_t z;

	/*	*/
	size_t vertexSize = (3 + 2 + 3 + 3) * sizeof(float);
	size_t indicesSize = 4;

	/*	*/
	unsigned int VertexIndex = 0, IndicesIndex = 0, bonecount = 0, initilzebone = 0;

	/*	*/
	float *vertices = (float *)malloc(aimesh->mNumVertices * vertexSize);
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
		glm::vec3 mtangent;

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

		// mtangent = tangent(VDVector3(pNormal->x, pNormal->y, pNormal->z));
		// mtangent.makeUnitVector();

		/*	*/
		*vertices++ = mtangent.x;
		*vertices++ = mtangent.y;
		*vertices++ = mtangent.z;

		/*	*/
		// for (size_t joint = 0; joint < bonecount; joint++) {
		//	*vertices++ = 0; /*	Bone ID.	*/
		//	*vertices++ = 0; /*	Vertex Weight.	*/
		//}

	} /**/

	vertices = temp;

	/*	Load bones.	*/
	if (aimesh->HasBones()) {
		bonecount = 4; /*	TODO */
		// vertexSize += bonecount * sizeof(float);

		// TODO add bone support.
		for (uint i = 0; i < aimesh->mNumBones; i++) {
			uint BoneIndex = 0;

			std::string BoneName(aimesh->mBones[i]->mName.data);

			/*	*/
			// if (vertexBoneData.find(BoneName) == vertexBoneData.end()) {
			//	BoneIndex = m_NumBones;
			//	m_NumBones++;
			//	BoneInfo bi;
			//	m_BoneInfo.push_back(bi);
			//} else {
			//	BoneIndex = vertexBoneData[BoneName];
			//}

			// m_BoneMapping[BoneName] = BoneIndex;
			// m_BoneInfo[BoneIndex].BoneOffset = aimesh->mBones[i]->mOffsetMatrix;
			//
			// for (uint j = 0; j < aimesh->mBones[i]->mNumWeights; j++) {
			//	uint VertexID = m_Entries[index].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
			//	float Weight = pMesh->mBones[i]->mWeights[j].mWeight;
			//	Bones[VertexID].AddBoneData(BoneIndex, Weight);
			//}
		}
	}

	/*	some issues with this I thing? */
	for (size_t x = 0; x < aimesh->mNumFaces; x++) {
		const aiFace &face = aimesh->mFaces[x];
		assert(face.mNumIndices == 3); // Check if Indices Count is 3 other case error
		memcpy(Indice, &face.mIndices[0], indicesSize);
		Indice += indicesSize;
		memcpy(Indice, &face.mIndices[1], indicesSize);
		Indice += indicesSize;
		memcpy(Indice, &face.mIndices[2], indicesSize);
		Indice += indicesSize;
	}

	Indice = Itemp;

	pmesh->indicesData = Indice;
	pmesh->indicesStride = indicesSize;
	pmesh->nrIndices = aimesh->mNumFaces * 3;
	pmesh->nrVertices = aimesh->mNumVertices;
	pmesh->vertexData = vertices;
	pmesh->vertexStride = vertexSize;

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
	aiShadingMode model;

	float specular;
	float blendfunc;
	glm::vec4 color;
	float shininessStrength;

	MaterialObject *material = &this->materials[index];

	if (!pmaterial) {
		return nullptr;
	}

	const bool isTextureEmpty = this->textures.size() == 0;

	/**/
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
			int ret = pmaterial->Get(AI_MATKEY_TEXTURE(textureType, textureIndex), textureName);
			/*	*/
			auto *embeededTexture = sceneRef->GetEmbeddedTexture(textureName.C_Str());

			if (pmaterial->GetTexture((aiTextureType)textureType, textureIndex, &path, nullptr, nullptr, nullptr,
									  nullptr, &mapmode) == aiReturn::aiReturn_SUCCESS) {

				if (path.data[0] == '*' && embeededTexture) {

					const int idIndex = atoi(&path.data[1]);

					switch (textureType) {
					case aiTextureType::aiTextureType_DIFFUSE:
						material->diffuseIndex = idIndex;
						break;
					case aiTextureType::aiTextureType_NORMALS:
						material->normalIndex = idIndex;
						break;
					default:
						break;
					}

				} else {
					TextureAssetObject *textureObj = this->textureMapping[path.C_Str()];

					if (textureObj) {
						const int texIndex = this->textureIndexMapping[path.C_Str()];
						switch (textureType) {
						case aiTextureType::aiTextureType_DIFFUSE:
							material->diffuseIndex = texIndex;
							break;
						case aiTextureType::aiTextureType_NORMALS:
							material->normalIndex = texIndex;
							break;
						default:
							break;
						}
					}
				}
			}

		} /**/
	}	  /**/

	/*	Assign shader attributes.	*/
	pmaterial->Get(AI_MATKEY_COLOR_AMBIENT, color[0]);
	material->ambient = color;
	pmaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color[0]);
	material->diffuse = color;
	pmaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color[0]);
	material->emission = color;
	pmaterial->Get(AI_MATKEY_COLOR_SPECULAR, color[0]);
	material->specular = color;
	pmaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, color[0]);
	material->transparent = color;
	pmaterial->Get(AI_MATKEY_REFLECTIVITY, color[0]);
	material->reflectivity = color;
	pmaterial->Get(AI_MATKEY_SHININESS, shininessStrength);
	material->shinininessStrength = shininessStrength;
	pmaterial->Get(AI_MATKEY_SHININESS_STRENGTH, color[0]);

	pmaterial->Get(AI_MATKEY_BLEND_FUNC, blendfunc);
	pmaterial->Get(AI_MATKEY_TWOSIDED, blendfunc);

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
