#include "SceneSettingComponentUI.h"

#include "GLUIComponent.h"
#include "RenderDesc.h"
#include "Scene.h"
#include "Scene/RenderQueue.h"
#include "Util/ImGuiUtil.h"
#include "imgui.h"
#include "magic_enum.hpp"
#include <Math/Bitwise.h>

using namespace glsample;

inline void nodeActive(Node &node) noexcept {
	bool active = node.isActive();
	if (ImGui::Checkbox("Active", &active)) {
		node.setActive(active);
	}
}

SceneSettingsUI::SceneSettingsUI(Scene &scene) : GLUIComponent<Scene>(scene, "Scene Settings") {}

void SceneSettingsUI::draw() {

	Scene &scene = getScene();

	/*	*/
	if (ImGui::CollapsingHeader("Scene Settings")) {

		if (ImGui::TreeNode("Rendering Settings")) {

			/*	*/
			if (ImGui::Checkbox("Use Frustum Culling", &scene.getRenderingSettings().frustumSettings.useFrustum)) {
			}
			enumComboBox<FrustumCullingMode>(
				"Frustum Culling Mode", scene.getRenderingSettings().frustumSettings.FrustumCullingMode,
				(int)FrustumCullingMode::MaxCullingMode,
				[&](auto mode) { scene.getRenderingSettings().frustumSettings.FrustumCullingMode = mode; });

			if (ImGui::Checkbox("Use Z Depth Early Pass",
								&scene.getRenderingSettings().preDepthRenderingSettings.get_value())) {
			}

			bool showWireFrame = (scene.getDebugMode() & DebugMode::Wireframe) == DebugMode::Wireframe;
			if (ImGui::Checkbox("Show Wireframe", &showWireFrame)) {
				if (showWireFrame) {
					scene.getDebugMode() = Bitwise::addFlag<DebugMode>(scene.getDebugMode(), DebugMode::Wireframe);
				} else {
					scene.getDebugMode() = Bitwise::removeFlag<DebugMode>(scene.getDebugMode(), DebugMode::Wireframe);
				}
			}

			bool showBoundingBox = (scene.getDebugMode() & DebugMode::BoundingBox) == DebugMode::BoundingBox;
			if (ImGui::Checkbox("Show BoundingBox", &showBoundingBox)) {
				if (showBoundingBox) {
					scene.getDebugMode() = Bitwise::addFlag<DebugMode>(scene.getDebugMode(), DebugMode::BoundingBox);
				} else {
					scene.getDebugMode() = Bitwise::removeFlag<DebugMode>(scene.getDebugMode(), DebugMode::BoundingBox);
				}
			}

			ImGui::SeparatorText("Rendering Queue Sorting Settings");

			bool distanceSort = scene.getRenderingSettings().sortDistance;
			if (ImGui::Checkbox("Sort Distance", &distanceSort)) {
			}

			bool mergeInstance = scene.getRenderingSettings().mergeInstances;
			if (ImGui::Checkbox("Merge Instances", &mergeInstance)) {
			}

			bool bucketMaterial = scene.getRenderingSettings().sortSharedMaterials;
			if (ImGui::Checkbox("Material Bucket Sorting", &bucketMaterial)) {
			}

			ImGui::TreePop();
		}

		if (ImGui::CollapsingHeader("Debug Settings")) {
			if (scene.getDebugDrawer()) {
				ImGui::SeparatorText("Debug Drawer");
				/*	*/
				ImGui::TextUnformatted("Debug Drawer");
			}
		}

		/*	*/
		if (ImGui::CollapsingHeader("Global Rendering Settings")) {

			ImGui::ColorEdit4("Global Ambient Color", &scene.getRenderingSettings().lightSettings.ambientColor[0],
							  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
			ImGui::ColorEdit4("Global Specular Color", &scene.getRenderingSettings().lightSettings.specularColor[0],
							  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

			scene.getRenderingSettings().skybox.renderImGUI();
		}

		{
			std::function<void(Node *)> nodeTreeFunc;

			/*	*/
			nodeTreeFunc = [scene, &nodeTreeFunc](Node *currentNode) {
				ImGui::PushID(currentNode->getUID() + 1000);

				if (ImGui::TreeNode(currentNode->getName().c_str())) {
					nodeActive(*currentNode);

					glm::vec3 localPosition = currentNode->getLocalPosition();
					if (ImGui::DragFloat3("Position", &localPosition[0])) {
						currentNode->setLocalPosition(localPosition);
					}

					glm::quat quat_local_rotation = currentNode->getLocalRotation();
					if (ImGui::DragFloat4("Rotation (Quat)", &quat_local_rotation[0])) {
						currentNode->setLocalRotation(quat_local_rotation);
					}

					glm::vec3 eular_rotation = currentNode->getRotationEular() * (float)Math::Rad2Deg;
					if (ImGui::DragFloat3("Rotation (Eular)", &eular_rotation[0])) {
						currentNode->setRotationEular(eular_rotation * (float)Math::Deg2Rad);
					}

					glm::vec3 local_scale = currentNode->getLocalScale();
					if (ImGui::DragFloat3("Scale", &local_scale[0])) {
						currentNode->setLocalScale(local_scale);
					}

					ImGui::Text("Materials %zu", currentNode->materialIndex.size());
					ImGui::Text("Meshes %zu", currentNode->geometryObjectIndex.size());

					ImGui::Separator();

					for (size_t node_index = 0; node_index < currentNode->getNumChildren(); node_index++) {
						nodeTreeFunc(currentNode->getChild(node_index)->ptr());
					}

					ImGui::TreePop();
				}

				ImGui::PopID();
			};

			Node *rootNode = scene.getRootNode();
			if (ImGui::CollapsingHeader("Scene Graph") && rootNode) {
				ImGui::Text("Count %lu", scene.getNodes().size());

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);

				nodeTreeFunc(rootNode);
			}
		}

		if (ImGui::CollapsingHeader("Light Settings")) {
			size_t light_index = 0;

			ImGui::TextUnformatted("Light Sources");

			for (; light_index < scene.getLights().size(); light_index++) {
				ImGui::PushID(light_index + 1000);

				Light *light = scene.getLights()[light_index];

				/*	*/
				nodeActive(*light);

				ImGui::SameLine();
				ImGui::TextUnformatted(light->getName().c_str());
				const std::string lightType = std::string(magic_enum::enum_name(light->getLightType()));
				ImGui::TextUnformatted(lightType.c_str());

				ImGui::ColorEdit4("Color", &light->color[0], ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

				glm::vec3 position = light->getPosition();
				if (ImGui::DragFloat3("Position", &position[0])) {
					light->setPosition(position);
				}

				switch (light->lightType) {
				case Light::LightType::Directional: {
					DirectionalLight *dirLight = dynamic_cast<DirectionalLight *>(light);
					glm::vec3 rotation_eular = dirLight->getRotationEular();
					if (ImGui::DragFloat3("Rotation", &rotation_eular[0])) {
						dirLight->setRotationEular(rotation_eular);
					}
				} break;
				case Light::LightType::Point: {
					PointLight *pointLight = dynamic_cast<PointLight *>(light);

					ImGui::DragFloat("Range", &pointLight->range);
				} break;
				default:
					break;
				}

				glm::ivec3 size = light->getSize();
				if (ImGui::DragInt2("Size", &size[0], 1.0f, 0, 0, "%d")) {
					size = glm::max(size, glm::ivec3(128));
					light->setSize(size);
				}

				FrameBuffer *framebuffer = light->getFrameBuffer();
				if (framebuffer) {
					ImGui::Image(static_cast<ImTextureID>(framebuffer->attachments[framebuffer->depthIndex]),
								 ImVec2(512, 512), ImVec2(1, 1), ImVec2(0, 0));
				}

				/*	*/
				ImGui::DragFloat("Shadow Strength", &light->shadow, 1, 0.0f, 1.0f);
				ImGui::DragFloat("Shadow Bias", &light->bias, 1, 0.0f, 1.0f, "%.5f");
				ImGui::DragFloat("Shadow PCF Radius", &light->pcf_radius, 1, 0.0f, 10.0f, "%.5f");

				float shadowDistance = light->getShadowDistance();
				if (ImGui::DragFloat("Shadow Distance", &shadowDistance, 1, 0.0f, 10000000.0f, "%.5f")) {
					light->setShadowDistance(shadowDistance);
				}
				ImGui::PopID();
			}

			if (ImGui::Button("Add Direction Light")) {
				scene.getLights().push_back(new DirectionalLight());
			}
			ImGui::SameLine();
			if (ImGui::Button("Add Point Light")) {
				scene.getLights().push_back(new PointLight());
			}
		}

		if (ImGui::CollapsingHeader("Materials")) {
			ImGui::Text("Count %lu", scene.getMaterials().size());

			/*	*/
			size_t material_index = 0;
			for (; material_index < scene.getMaterials().size(); material_index++) {

				Material &mat = scene.getMaterials()[material_index];
				ImGui::PushID(material_index);
				ImGui::TextUnformatted(scene.getMaterials()[material_index].getName().c_str());

				ImGui::ColorEdit4("Ambient Color", &mat.ambient[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Diffuse Color", &mat.diffuse[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Transparent Color", &mat.transparent[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Emission Color", &mat.emission[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Reflective Color", &mat.reflectivity[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Specular Color", &mat.specular[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

				ImGui::DragFloat("Clipping", &mat.getGraphicSettings().clipping, 1, 0, 1);
				ImGui::DragFloat("Shinininess", &mat.shinininess, 1, 0, 0);
				ImGui::DragFloat("Bumpiness", &mat.bumpiness, 1, 0, 0);
				ImGui::SliderFloat("Metalic", &mat.metalic, 0, 1);

				ImGui::DragFloat("Displacement", &mat.getTessellationSettings().gDispFactor);

				/*	*/
				ImGui::Checkbox("Write Depth", &mat.getGraphicSettings().DepthWrite);
				enumComboBox<fragcore::BlendEqu>(
					"Blend Equation Mode", mat.getGraphicSettings().blend_equ, static_cast<size_t>(BlendEqu::Max) + 1,
					[&mat](const auto selected) { mat.getGraphicSettings().blend_equ = selected; });
				enumComboBox<fragcore::BlendFunc>(
					"Blend Function Mode", mat.getGraphicSettings().blend_color_func,
					static_cast<size_t>(BlendFunc::ConstantAlpha) + 1,
					[&mat](const auto selected) { mat.getGraphicSettings().blend_color_func = selected; });
				enumComboBox<fragcore::DepthFunc>(
					"Depth Function", mat.getGraphicSettings().DepthFunc, static_cast<size_t>(DepthFunc::Never) + 1,
					[&mat](const auto selected) { mat.getGraphicSettings().DepthFunc = selected; });
				enumComboBox<fragcore::CullingMode>(
					"Culling Mode", mat.getGraphicSettings().cullingMode,
					static_cast<size_t>(CullingMode::FrontAndBack) + 1,
					[&mat](const auto selected) { mat.getGraphicSettings().cullingMode = selected; });
				enumComboBox<fragcore::FillMode>(
					"Fill Mode", mat.getGraphicSettings().fillMode, static_cast<size_t>(fragcore::FillMode::Point) + 1,
					[&mat](const auto selected) { mat.getGraphicSettings().fillMode = selected; });

				ImGui::SetNextItemWidth(256);
				ImGui::DragInt("Render Queue", (int *)&mat.getGraphicSettings().queue);
				ImGui::SameLine();
				if (ImGui::Button("Set Domain")) {
					ImGui::OpenPopup("set_domain_popup");
				}
				if (ImGui::BeginPopup("set_domain_popup")) {
					ImGui::SeparatorText("Domains");
					for (size_t renderQueue_select_index = 0; renderQueue_select_index < getRenderQueueSymbols().size();
						 renderQueue_select_index++) {

						if (ImGui::Selectable(getRenderQueueSymbols()[renderQueue_select_index])) {
							mat.getGraphicSettings().queue = getQueueTypesOrdered()[renderQueue_select_index];
						}
					}
					ImGui::EndPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Update Domain")) {
					mat.getGraphicSettings().queue = Material::getDefaultQueueDomain(mat);
				}

				/*	Textures.	*/
				for (size_t mat_tex_index = 0; mat_tex_index < mat.texture_index.size(); mat_tex_index++) {

					/*	Validate Texture.	*/
					const uint32_t texture_index = mat.texture_index[mat_tex_index];
					if (mat.texture_index[mat_tex_index] == -1) {
						continue;
					}

					const unsigned int tex = scene.getTextures()[texture_index].texture;
					if (glIsTexture(tex)) {

						ImGui::PushID(mat_tex_index);

						/*	*/
						const std::string texType =
							std::string(magic_enum::enum_name((TextureTypeBinding)mat_tex_index));
						ImGui::Image(tex, ImVec2(96, 96), ImVec2(1, 1), ImVec2(0, 0));
						ImGui::SameLine();
						ImGui::Text("%s (%ld)", texType.c_str(), mat_tex_index);
						ImGui::SameLine();

						ImGui::BeginGroup();
						enumComboBox<fragcore::TextureWrappingMode>(
							"Texture Wrapping", mat.texture_sampling[mat_tex_index].wrapping,
							static_cast<size_t>(fragcore::TextureWrappingMode::ClampBorder) + 1,
							[&](auto selected) { mat.texture_sampling[mat_tex_index].wrapping = selected; });

						enumComboBox<fragcore::TextureFilterMode>(
							"Texture Filtering", mat.texture_sampling[mat_tex_index].filtering,
							static_cast<size_t>(fragcore::TextureFilterMode::Trilinear) + 1,
							[&](auto selected) { mat.texture_sampling[mat_tex_index].filtering = selected; });

						float min_lod = NAN;
						glGetSamplerParameterfv(scene.getSamplers()[mat_tex_index], GL_TEXTURE_MIN_LOD, &min_lod);
						ImGui::SetNextItemWidth(256);
						if (ImGui::DragFloat("Min LOD", &min_lod)) {
							glSamplerParameteri(scene.getSamplers()[mat_tex_index], GL_TEXTURE_MIN_LOD, min_lod);
						}
						float max_lod = NAN;
						glGetSamplerParameterfv(scene.getSamplers()[mat_tex_index], GL_TEXTURE_MAX_LOD, &max_lod);
						ImGui::SetNextItemWidth(256);
						if (ImGui::DragFloat("Max LOD", &max_lod)) {
							glSamplerParameteri(scene.getSamplers()[mat_tex_index], GL_TEXTURE_MAX_LOD, max_lod);
						}

						ImGui::EndGroup();

						ImGui::PopID();
					}
				}
				ImGui::Separator();
				ImGui::PopID();
			}
		}

		if (ImGui::CollapsingHeader("Cameras")) {
			size_t camera_index = 0;
			for (; camera_index < scene.getCameras().size(); camera_index++) {
				Camera *camera = scene.getCameras()[camera_index];

				nodeActive(*camera);
				float fov = camera->getFOVDegree();
				if (ImGui::SliderFloat("FOV", &fov, 0, 90)) {
					camera->setFOVDegree(fov);
				}
				float near = camera->getNear();
				if (ImGui::DragFloat("Near", &near, 1, 0.015f)) {
					camera->setNear(near);
				}

				float far = camera->getFar();
				if (ImGui::DragFloat("Far", &far, 1, camera->getNear())) {
					camera->setFar(far);
				}
			}
		}

		if (ImGui::CollapsingHeader("Textures")) {
			size_t material_index = 0;
			for (; material_index < scene.getMaterials().size(); material_index++) {
			}
		}

		if (ImGui::CollapsingHeader("Animation")) {

			size_t animation_index = 0;
			for (; animation_index < scene.getAnimation().size(); animation_index++) {
				AnimationPlayer *animation = scene.getAnimation()[animation_index];

				ImGui::PushID(animation_index + 1000);

				ImGui::TextUnformatted(animation->getName().c_str());
				ImGui::Text("Animation Channels: %zu", animation->getCurves().size());

				/*	Draw Each curves.	*/
				for (size_t i = 0; i < animation->getCurves().size(); i++) {
				}

				ImGui::Button("Play");
				ImGui::Button("Stop");

				ImGui::PopID();
			}
		}

		if (ImGui::CollapsingHeader("Meshes")) {
			size_t mesh_index = 0;
			for (; mesh_index < scene.getMeshes().size(); mesh_index++) {
				auto &ref = scene.getMeshes()[mesh_index];
				ImGui::PushID(mesh_index);
				ImGui::Text("Index %zu", mesh_index);
				ImGui::Text("Vertices %zu", ref.nrVertices);
				ImGui::Text("Indices Elements %zu", ref.nrIndicesElements);
				ImGui::Text("Vertex Stride %u", ref.stride);
				// ImGui::Text("Indices Elements Stride %zu", ref.stride);
				ImGui::PopID();
			}
		}
	}
}