#include "ModelImporter.h"

void ModelImporter::loadContent(const std::string &path, unsigned long int supportFlag) {
	Importer importer;

	/*	Load file content. */
	Ref<IO> io = Ref<IO>(fileSystem->openFile(path.c_str(), IO::IOMode::READ));
	std::vector<char> bufferIO = IOUtil::readFile<char>(io);

	/*  */
	const aiScene *pScene =
		importer.ReadFileFromMemory(bufferIO.data(), bufferIO.size(),
									aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
										aiProcess_SplitLargeMeshes | aiProcess_ValidateDataStructure);

	bufferIO.clear();
	if (pScene == nullptr) {
		throw RuntimeException(importer.GetErrorString());
	}

	/*	*/
	this->sceneRef = (aiScene *)pScene;

	if (pScene) {
		aiMatrix4x4 m_GlobalInverseTransform = pScene->mRootNode->mTransformation;
		m_GlobalInverseTransform.Inverse();

		this->initScene(pScene);
		importer.FreeScene();
	} else {
		throw RuntimeException("Failed to load");
	}
	// return importSuccess;
}
void ModelImporter::clear() {
	// foreach all assets and clean.
}

void ModelImporter::initScene(const aiScene *scene) {
	int x;

	// /*	 */
	if (scene->HasMeshes()) {
		for (unsigned int x = 0; x < scene->mNumMeshes; x++) {
			this->initMesh(scene->mMeshes[x], x);
		}
	}

	// /*	*/
	// if (scene->HasAnimations() ) {
	// 	for (unsigned int x = 0; x < scene->mNumAnimations; x++) {
	// 		this->initAnimation(scene->mAnimations[x], x);
	// 	}
	// }

	// /*	*/
	// if (scene->HasTextures()) {
	// 	for (unsigned int x = 0; x < scene->mNumTextures; x++) {
	// 		this->initTexture(scene->mTextures[x], x);
	// 	}
	// }

	// /*	*/
	// if (scene->HasMaterials() ) {
	// 	for (unsigned int x = 0; x < scene->mNumMaterials; x++) {
	// 		this->initMaterial(scene->mMaterials[x]);
	// 	}
	// }

	// /*	*/
	// if (scene->HasLights()) {
	// 	for (unsigned int x = 0; x < scene->mNumLights; x++) {
	// 		this->initLight(scene->mLights[x], x);
	// 	}
	// }

	// /*	*/
	// if (scene->HasCameras() ) {
	// 	for (unsigned int x = 0; x < scene->mNumCameras; x++) {
	// 		this->initCamera(scene->mCameras[x], x);
	// 	}
	// }

	// /*	*/
	// //if (this->getFlag() & IMPORT_HIERARCHY) {
	// 	this->initNoodeRoot(scene->mRootNode);
	// //}
}

void ModelImporter::initNoodeRoot(const aiNode *nodes, NodeObject *parent) {
	unsigned int gameObjectCount = 0;
	unsigned int meshCount = 0;

	/*	iterate through each child of parent node.	*/
	for (unsigned int x = 0; x < nodes->mNumChildren; x++) {
		unsigned int meshCount = 0;
		aiVector3D position, scale;
		aiQuaternion rotation;

		NodeObject *pobject = new NodeObject();

		/*	extract position, rotation, position from transformation matrix.	*/
		nodes->mTransformation.Decompose(scale, rotation, position);
		if (parent) {
			pobject->parent = parent;
			//			pobject->transform()->setParent(parent->transform());
		} else {
			//			pobject->transform()->setParent(nullptr);
		}

		/*	TODO check if works.	*/
		pobject->position = glm::vec3(position.x, position.y, position.z);
		pobject->rotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
		pobject->scale = glm::vec3(scale.x, scale.y, scale.z);
		pobject->name = nodes->mChildren[x]->mName.C_Str();

		/**/
		if (nodes->mChildren[x]->mMeshes) {

			//if (*nodes->mChildren[x]->mMeshes >= 0 && this->meshs.size() > *nodes->mChildren[x]->mMeshes) {

				///*	get mesh by index.	*/
				// mesh = getMesh(*nodes->mChildren[x]->mMeshes);
				//
				// if (!this->sceneRef->mMeshes[*nodes->mChildren[x]->mMeshes]->HasBones()) {
				//	renderer = pobject->addComponet<VDRenderer>();
				//}
				//
				// if (renderer) {
				//	this->setRenderer(renderer);
				//	renderer->setMesh(mesh);
				//}
				// if (this->getFlag() & IMPORT_MATERIAL) {
				//	VDMaterial *mat =
				//		this->getMaterial(this->sceneRef->mMeshes[*nodes->mChildren[x]->mMeshes]->mMaterialIndex);
				//	if (renderer != nullptr) {
				//		renderer->setMaterial(mat);
				//	}
				//}
			//}
		}

		/**/
		// if ((getFlag() & IMPORT_ANIMATION) && this->getAnimationClipCount() > 0) {
		//	if (pobject->transform() == pobject->transform()->root()) {
		//		pobject->addComponet<VDAnimation>();
		//		pobject->animation()->addClip(getAnimationClip(0));
		//	}
		//}

		/**/
		initNoodeRoot(nodes->mChildren[x], pobject);
	}
}

ModelSystemObject *ModelImporter::initMesh(const aiMesh *aimesh, unsigned int index) {
	ModelSystemObject *pmesh;
	unsigned int z;

	size_t vertexSize =
		(3 + 2 + 3 + 3) * sizeof(float); // VDMesh::getVertexStrideSize((VDMesh::MeshComponent)this->flags);
	// GLuint vertexSize = getdataStructSize(this->supportFlag);
	size_t indicesSize = 4; // VDGeometryUtility::getindexBufferSize(aimesh->mNumFaces);

	unsigned int VertexIndex = 0, IndicesIndex = 0, bonecount = 0, initilzebone = 0;

	indicesSize = 4; /*TODO remove later, if not it cased the geometry notbe rendered proparly*/
	float *vertices = (float *)malloc(aimesh->mNumVertices * vertexSize);
	unsigned char *Indice = (unsigned char *)malloc(indicesSize * aimesh->mNumFaces * 3);

	float *temp = vertices;
	unsigned char *Itemp = Indice;

	const aiVector3D Zero = aiVector3D(0, 0, 0);

	for (unsigned int x = 0; x < aimesh->mNumVertices; x++) {
		const aiVector3D *Pos = &(aimesh->mVertices[x]);
		aiVector3D *pNormal = &(aimesh->mNormals[x]);
		const aiVector3D *pTexCoord = aimesh->HasTextureCoords(0) ? &(aimesh->mTextureCoords[0][x]) : &Zero;
		glm::vec3 mtangent;

		*vertices++ = Pos->x;
		*vertices++ = Pos->y;
		*vertices++ = Pos->z;

		*vertices++ = pTexCoord->x;
		*vertices++ = pTexCoord->y;
		// uvs[x] = pTexCoord->x;
		// uvs[x] = pTexCoord->y;

		pNormal->Normalize();
		*vertices++ = pNormal->x;
		*vertices++ = pNormal->y;
		*vertices++ = pNormal->z;

		// mtangent = tangent(VDVector3(pNormal->x, pNormal->y, pNormal->z));
		// mtangent.makeUnitVector();

		*vertices++ = mtangent.x;
		*vertices++ = mtangent.y;
		*vertices++ = mtangent.z;

	} /**/

	vertices = temp;

	/*	some issues with this I thing? */
	for (unsigned int x = 0; x < aimesh->mNumFaces; x++) {
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

	// if (this->getFlag() & IMPORT_MESH) {
	//
	//	VDMesh *mmesh = VDMesh::createMesh();
	//	mmesh->assemblyMesh(
	//		vertices, vertexSize, aimesh->mNumVertices * vertexSize, Indice, indicesSize,
	//		indicesSize * aimesh->mNumFaces * 3,
	//		(((unsigned int)flags & ((VDMesh::eVertex | VDMesh::eNormal | VDMesh::eTangent | VDMesh::eTextureCoord))) |
	//		 (aimesh->HasBones() == true ? (VDMesh::eBoneID | VDMesh::eBoneWeight) : 0)));
	//	/**/
	//	mmesh->setName(aimesh->mName.data);
	//	this->setMesh(mmesh);
	//
	//	// VDAssetManager::assignAsset(mmesh, this->getPath());
	//}

	/**/
	// free(vertices);
	// free(Indice);

	/**/
	return pmesh;// this->getMesh(this->getMeshCount() - 1);
}

// VDMaterial *ModelImporter::initMaterial(aiMaterial *pmaterial) {
//	unsigned int x;
//	unsigned int y;
//	char absolutepath[PATH_MAX];
//	VDMaterial *mate = nullptr;
//	VDTexture *tex;
//
//	char *data;
//	aiString name;
//	unsigned int index;
//	aiString path;
//	aiTextureMapping mapping;
//	unsigned int uvindex;
//	float blend;
//	aiTextureOp op;
//	aiTextureMapMode mapmode = aiTextureMapMode::aiTextureMapMode_Wrap;
//	aiShadingMode model;
//
//	float specular;
//	float blendfunc;
//	VDColor color;
//	float shininessStrength;
//
//	if (!pmaterial) {
//		VDDebug::debugLog("(nullptr) Assimp Material.\n");
//		return nullptr;
//	}
//
//	/**/
//	mate = VDMaterial::createMaterial();
//	pmaterial->Get(AI_MATKEY_NAME, name);
//	mate->setName(name.C_Str());
//
//	/*	load all texture assoicated with texture.	*/
//	for (y = 0; y < aiTextureType::aiTextureType_UNKNOWN; y++) {
//		for (x = 0; x < pmaterial->GetTextureCount((aiTextureType)y); x++) {
//
//			/*	extract texture information.	*/
//			if (pmaterial->GetTexture((aiTextureType)y, x, &path, nullptr, nullptr, nullptr, nullptr, &mapmode) ==
//				aiReturn::aiReturn_SUCCESS) {
//				if (path.C_Str()) {
//					std::string thepath = VDFile::getDirectory(this->getPath()) + "/" + path.C_Str();
//					tex = findTexture(thepath.c_str());
//
//					if (tex == nullptr) {
//						tex = VDResources::load<VDTexture2D>(thepath.c_str());
//
//						/*	texture mapmode.	*/
//						if (tex != nullptr) {
//							switch (mapmode) {
//							case aiTextureMapMode_Clamp:
//								tex->setWrapMode(VDTexture::eClamp);
//								break;
//							case aiTextureMapMode_Wrap:
//								tex->setWrapMode(VDTexture::eRepeat);
//								break;
//							}
//						}
//					}
//
//					mate->setTexture(y - 1, tex);
//				}
//			}
//
//			tex = nullptr;
//		} /**/
//	}	  /**/
//
//	/*	Determine shader type.	*/
//	pmaterial->Get(AI_MATKEY_SHADING_MODEL, name);
//	pmaterial->Get(AI_MATKEY_SHININESS_STRENGTH, specular);
//	pmaterial->Get(AI_MATKEY_BLEND_FUNC, blendfunc);
//	pmaterial->Get(AI_MATKEY_TWOSIDED, blendfunc);
//
//	if (mate->getTexture(VDShaderConstant::eNormalTexture) != nullptr) {
//		if (specular > 0.0)
//			mate->setShader(VDShaderCreator::SpecularNormal());
//		else
//			mate->setShader(VDShaderCreator::NormalDiffuse());
//	}
//
//	/*	Assign shader attributes.	*/
//	pmaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color[0]);
//	mate->setColor("DiffuseColor", color);
//	pmaterial->Get(AI_MATKEY_COLOR_SPECULAR, color[0]);
//	mate->setColor("SpecularColor", color);
//	pmaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, color[0]);
//	mate->setColor("", color);
//	pmaterial->Get(AI_MATKEY_REFLECTIVITY, color[0]);
//	mate->setColor("", color);
//	pmaterial->Get(AI_MATKEY_SHININESS, shininessStrength);
//	mate->setColor("", color);
//	pmaterial->Get(AI_MATKEY_SHININESS_STRENGTH, color[0]);
//	mate->setColor("", color);
//
//	if (mate->getShader() == nullptr) {
//		mate->setShader(VDMaterial::getDefaultMaterial()->getShader());
//	}
//	this->setMaterial(mate);
//	return mate;
//}
//
// VDTexture2D *ModelImporter::initTexture(aiTexture *texture, unsigned int index) {
//	VDTexture2D *mtexture = new VDTexture2D(texture->mWidth, texture->mWidth, texture->achFormatHint[0]);
//	mtexture->setPixelData(0, mtexture->width(), mtexture->height(), 0, VDTexture::eRGB, VDTexture::eRGB,
//						   VDTexture::eUnsignedByte, texture->pcData, 0);
//	setTexture(mtexture);
//	return mtexture;
//}
//
// VDCamera *ModelImporter::initCamera(aiCamera *pCamera, unsigned int Index) {
//	VDCamera *camera = new VDCamera();
//
//	/*TODO resolve */
//	camera->setFar(pCamera->mClipPlaneFar);
//	camera->setNear(pCamera->mClipPlaneNear);
//	camera->setFov(pCamera->mHorizontalFOV);
//	camera->setRatio(pCamera->mAspect);
//
//	this->setCamera(camera);
//	return camera;
//}
//
// VDLight *ModelImporter::initLight(const aiLight *Llight, unsigned int index) {
//	VDLight *light = nullptr;
//	light = new VDLight();
//
//	light->setColor(*(VDColor *)&Llight->mColorDiffuse);
//	light->setIntensity(((VDVector3 *)&Llight->mColorDiffuse)->length());
//
//	light->setOuterCone(Llight->mAngleOuterCone);
//	light->setInnerCone(Llight->mAngleInnerCone);
//
//	switch (Llight->mType) {
//	case aiLightSource_POINT:
//		light->setType(VDLight::ePoint);
//		break;
//	case aiLightSource_DIRECTIONAL:
//		light->setType(VDLight::eDirection);
//		break;
//	case aiLightSource_SPOT:
//		light->setType(VDLight::eSpot);
//		break;
//	case aiLightSource_UNDEFINED:
//	case _aiLightSource_Force32Bit:
//		break;
//	default:
//		break;
//	}
//	light->enableShadow(true);
//	this->setLight(light);
//
//	for (unsigned int x = 0; x < this->object.size(); x++) {
//		if (strcmp(object[x]->getName(), Llight->mName.data) == 0) {
//			this->object[x]->addComponet(light);
//
//			this->object[x]->transform()->setPosition(*(VDVector3 *)&Llight->mPosition);
//
//			break;
//		}
//	}
//	return light;
//}
//
// VDAnimationClip *ModelImporter::initAnimation(const aiAnimation *panimation, unsigned int index) {
//	VDAnimationClip *clip = new VDAnimationClip(panimation->mDuration, panimation->mTicksPerSecond);
//	unsigned int channel_index = 0;
//
//	clip->setName(panimation->mName.data);
//	for (unsigned int x = 0; x < panimation->mNumChannels; x++) {
//		this->initAnimationPosition(panimation->mChannels[x], clip);
//		this->initAnimationRotation(panimation->mChannels[x], clip);
//		this->initAnimationScale(panimation->mChannels[x], clip);
//	}
//	this->setAnimationClip(clip);
//	return clip;
//}
//
// void ModelImporter::initAnimationPosition(aiNodeAnim *position, VDAnimationClip *animationClip) {
//	if (position->mNumPositionKeys <= 0) {
//		return;
//	}
//
//	animationClip->addCurve(VDCurve(position->mNumPositionKeys));
//	animationClip->addCurve(VDCurve(position->mNumPositionKeys));
//	animationClip->addCurve(VDCurve(position->mNumPositionKeys));
//
//	animationClip->getCurve(animationClip->getNumCurves() - 3).setCurveFlag(VDCurve::eTransformPosX);
//	animationClip->getCurve(animationClip->getNumCurves() - 2).setCurveFlag(VDCurve::eTransformPosY);
//	animationClip->getCurve(animationClip->getNumCurves() - 1).setCurveFlag(VDCurve::eTransformPosZ);
//
//	animationClip->getCurve(animationClip->getNumCurves() - 3).setName(position->mNodeName.data);
//	animationClip->getCurve(animationClip->getNumCurves() - 2).setName(position->mNodeName.data);
//	animationClip->getCurve(animationClip->getNumCurves() - 1).setName(position->mNodeName.data);
//
//	for (unsigned int x = 0; x < position->mNumPositionKeys; x++) {
//		// curve Position X
//		animationClip->getCurve(animationClip->getNumCurves() - 3)
//			.getKey(x)
//			.setValue(position->mPositionKeys[x].mValue.x);
//		animationClip->getCurve(animationClip->getNumCurves() - 3).getKey(x).setTime(position->mPositionKeys[x].mTime);
//
//		// curve Position Y
//		animationClip->getCurve(animationClip->getNumCurves() - 2)
//			.getKey(x)
//			.setValue(position->mPositionKeys[x].mValue.y);
//		animationClip->getCurve(animationClip->getNumCurves() - 2).getKey(x).setTime(position->mPositionKeys[x].mTime);
//		// curve Position Z
//		animationClip->getCurve(animationClip->getNumCurves() - 1)
//			.getKey(x)
//			.setValue(position->mPositionKeys[x].mValue.z);
//		animationClip->getCurve(animationClip->getNumCurves() - 1).getKey(x).setTime(position->mPositionKeys[x].mTime);
//	}
//}
//
// void ModelImporter::initAnimationRotation(aiNodeAnim *rotation, VDAnimationClip *animationClip) {
//	if (rotation->mNumRotationKeys <= 0)
//		return;
//
//	animationClip->addCurve(VDCurve(rotation->mNumRotationKeys));
//	animationClip->addCurve(VDCurve(rotation->mNumRotationKeys));
//	animationClip->addCurve(VDCurve(rotation->mNumRotationKeys));
//	animationClip->addCurve(VDCurve(rotation->mNumRotationKeys));
//
//	animationClip->getCurve(animationClip->getNumCurves() - 4).setCurveFlag(VDCurve::eTransformQuadX);
//	animationClip->getCurve(animationClip->getNumCurves() - 3).setCurveFlag(VDCurve::eTransformQuadY);
//	animationClip->getCurve(animationClip->getNumCurves() - 2).setCurveFlag(VDCurve::eTransformQuadZ);
//	animationClip->getCurve(animationClip->getNumCurves() - 1).setCurveFlag(VDCurve::eTransformQuadW);
//
//	animationClip->getCurve(animationClip->getNumCurves() - 4).setName(rotation->mNodeName.data);
//	animationClip->getCurve(animationClip->getNumCurves() - 3).setName(rotation->mNodeName.data);
//	animationClip->getCurve(animationClip->getNumCurves() - 2).setName(rotation->mNodeName.data);
//	animationClip->getCurve(animationClip->getNumCurves() - 1).setName(rotation->mNodeName.data);
//
//	for (unsigned int x = 0; x < rotation->mNumRotationKeys; x++) {
//		// curve rotation X
//		animationClip->getCurve(animationClip->getNumCurves() - 4)
//			.getKey(x)
//			.setValue(rotation->mRotationKeys[x].mValue.x);
//		animationClip->getCurve(animationClip->getNumCurves() - 4).getKey(x).setTime(rotation->mRotationKeys[x].mTime);
//		// curve rotation Y
//		animationClip->getCurve(animationClip->getNumCurves() - 3)
//			.getKey(x)
//			.setValue(rotation->mRotationKeys[x].mValue.y);
//		animationClip->getCurve(animationClip->getNumCurves() - 3).getKey(x).setTime(rotation->mRotationKeys[x].mTime);
//		// curve rotation Z
//		animationClip->getCurve(animationClip->getNumCurves() - 2)
//			.getKey(x)
//			.setValue(rotation->mRotationKeys[x].mValue.z);
//		animationClip->getCurve(animationClip->getNumCurves() - 2).getKey(x).setTime(rotation->mRotationKeys[x].mTime);
//		// curve rotation W
//		animationClip->getCurve(animationClip->getNumCurves() - 1)
//			.getKey(x)
//			.setValue(rotation->mRotationKeys[x].mValue.w);
//		animationClip->getCurve(animationClip->getNumCurves() - 1).getKey(x).setTime(rotation->mRotationKeys[x].mTime);
//	}
//}
//
// void ModelImporter::initAnimationScale(aiNodeAnim *scale, VDAnimationClip *animationClip) {
//	if (scale->mNumScalingKeys <= 0) {
//		return;
//	}
//
//	animationClip->addCurve(VDCurve(scale->mNumScalingKeys));
//	animationClip->addCurve(VDCurve(scale->mNumScalingKeys));
//	animationClip->addCurve(VDCurve(scale->mNumScalingKeys));
//
//	animationClip->getCurve(animationClip->getNumCurves() - 3).setCurveFlag(VDCurve::eTransformScaX);
//	animationClip->getCurve(animationClip->getNumCurves() - 2).setCurveFlag(VDCurve::eTransformScaY);
//	animationClip->getCurve(animationClip->getNumCurves() - 1).setCurveFlag(VDCurve::eTransformScaZ);
//
//	animationClip->getCurve(animationClip->getNumCurves() - 3).setName(scale->mNodeName.data);
//	animationClip->getCurve(animationClip->getNumCurves() - 2).setName(scale->mNodeName.data);
//	animationClip->getCurve(animationClip->getNumCurves() - 1).setName(scale->mNodeName.data);
//
//	for (unsigned int x = 0; x < scale->mNumScalingKeys; x++) {
//		/* curve Scale X	*/
//		animationClip->getCurve(animationClip->getNumCurves() - 3).getKey(x).setValue(scale->mScalingKeys[x].mValue.x);
//		animationClip->getCurve(animationClip->getNumCurves() - 3).getKey(x).setTime(scale->mScalingKeys[x].mTime);
//		/* curve Scale Y	*/
//		animationClip->getCurve(animationClip->getNumCurves() - 2).getKey(x).setValue(scale->mScalingKeys[x].mValue.y);
//		animationClip->getCurve(animationClip->getNumCurves() - 2).getKey(x).setTime(scale->mScalingKeys[x].mTime);
//		/* curve Scale Z	*/
//		animationClip->getCurve(animationClip->getNumCurves() - 1).getKey(x).setValue(scale->mScalingKeys[x].mValue.z);
//		animationClip->getCurve(animationClip->getNumCurves() - 1).getKey(x).setTime(scale->mScalingKeys[x].mTime);
//	}
//}