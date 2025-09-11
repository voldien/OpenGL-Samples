#include "SceneSettingComponentUI.h"
#include "GLUIComponent.h"
#include "Scene.h"
#include "imgui.h"
#include "magic_enum.hpp"

using namespace glsample;

SceneSettingsUI::SceneSettingsUI(Scene &scene) : GLUIComponent<Scene>(scene, "Scene Settings") {}

void SceneSettingsUI::draw() {

	Scene &scene = getScene();

	/*	*/
	if (ImGui::CollapsingHeader("Scene Settings")) {

		if (ImGui::CollapsingHeader("Rendering Settings")) {

			/*	*/
			if (ImGui::Checkbox("Use Frustum Culling", &scene.getRenderingSettings().frustumSettings.useFrustum)) {
			}

			bool showWireFrame =
				(scene.getRenderingSettings().debugMode & DebugMode::Wireframe) == DebugMode::Wireframe;
			if (ImGui::Checkbox("Show Wireframe", &showWireFrame)) {
				if (showWireFrame) {
					scene.getRenderingSettings().debugMode =
						Math::addFlag<unsigned int>(scene.getRenderingSettings().debugMode, DebugMode::Wireframe);
				} else {
					scene.getRenderingSettings().debugMode =
						Math::removeFlag<unsigned int>(scene.getRenderingSettings().debugMode, DebugMode::Wireframe);
				}
			}

			bool showBoundingBox =
				(scene.getRenderingSettings().debugMode & DebugMode::BoundingBox) == DebugMode::BoundingBox;
			if (ImGui::Checkbox("Show BoundingBox", &showBoundingBox)) {
				if (showBoundingBox) {
					scene.getRenderingSettings().debugMode =
						Math::addFlag<unsigned int>(scene.getRenderingSettings().debugMode, DebugMode::BoundingBox);
				} else {
					scene.getRenderingSettings().debugMode =
						Math::removeFlag<unsigned int>(scene.getRenderingSettings().debugMode, DebugMode::BoundingBox);
				}
			}

			if (scene.getDebugDrawer()) {
				/*	*/
				ImGui::TextUnformatted("Debug Drawer");
			}
		}

		if (ImGui::TreeNode("Advanced, with Selectable nodes")) {
			ImGui::TreePop();
		}

		/*	*/
		if (ImGui::CollapsingHeader("Global Rendering Settings")) {

			ImGui::ColorEdit4("Global Ambient Color", &scene.getRenderingSettings().ambientColor[0],
							  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

			{
				ImGui::TextUnformatted("Global Fog");

				// ImGui::Checkbox("Use Fog", &scene.getRenderingSettings().fogSettings.fogType);
				ImGui::DragInt("Fog Type", (int *)&scene.getRenderingSettings().fogSettings.fogType);
				ImGui::ColorEdit4("Fog Color", &scene.getRenderingSettings().fogSettings.fogColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat("Fog Density", &scene.getRenderingSettings().fogSettings.fogDensity);
				ImGui::DragFloat("Fog Intensity", &scene.getRenderingSettings().fogSettings.fogIntensity);
				ImGui::DragFloat("Fog Start", &scene.getRenderingSettings().fogSettings.fogStart);
				ImGui::DragFloat("Fog End", &scene.getRenderingSettings().fogSettings.fogEnd);
			}
		}

		/*	*/
		// TODO: add tree structure
		if (ImGui::TreeNode("Nodes")) {
			ImGui::Text("Count %lu", scene.getNodes().size());

			for (size_t node_index = 0; node_index < scene.getNodes().size(); node_index++) {

				Node *currentNode = scene.getNodes()[node_index];

				if (node_index == 0) {
					ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				}

				ImGui::PushID(node_index);

				if (currentNode->getParent()) {
					ImGui::TextUnformatted("Has Parent");
					ImGui::SameLine();
					ImGui::TextUnformatted(currentNode->getParent()->ptr()->getName().c_str());
				}

				ImGui::TextUnformatted(currentNode->getName().c_str());

				glm::vec3 localPosition = currentNode->getLocalPosition();
				if (ImGui::DragFloat3("Position", &localPosition[0])) {
					currentNode->setLocalPosition(localPosition);
				}

				glm::quat quat_local_rotation = currentNode->getLocalRotation();
				if (ImGui::DragFloat4("Rotation (Quat)", &quat_local_rotation[0])) {
					currentNode->setLocalRotation(quat_local_rotation);
				}

				glm::vec3 eular_rotation = currentNode->getRotationEular();
				if (ImGui::DragFloat3("Rotation (Eular)", &eular_rotation[0])) {
					currentNode->setRotationEular(eular_rotation);
				}

				glm::vec3 local_scale = currentNode->getLocalScale();
				if (ImGui::DragFloat3("Scale", &local_scale[0])) {
					currentNode->setLocalScale(local_scale);
				}
				ImGui::Separator();

				ImGui::PopID();
			}

			/*	*/
			ImGui::TreePop();
		}

		if (ImGui::CollapsingHeader("Light Settings")) {
			size_t light_index = 0;

			ImGui::TextUnformatted("Light Sources");

			for (; light_index < scene.getLights().size(); light_index++) {
				ImGui::PushID(light_index + 1000);

				Light *light = scene.getLights()[light_index];

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

				ImGui::DragFloat("Shadow Strength", &light->shadow, 1, 0.0f, 1.0f);
				ImGui::DragFloat("Shadow Bias", &light->bias, 1, 0.0f, 1.0f, "%.5f");

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
			size_t material_index = 0;
			for (; material_index < scene.getMaterials().size(); material_index++) {

				MaterialObject &mat = scene.getMaterials()[material_index];
				ImGui::PushID(material_index);
				ImGui::TextUnformatted(scene.getMaterials()[material_index].name.c_str());

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

				ImGui::DragFloat("Clipping", &mat.clipping, 1, 0, 1);
				ImGui::DragFloat("Shinininess", &mat.shinininess, 1, 0, 128);
				ImGui::DragFloat("Bumpiness", &mat.bumpiness, 1, 0, 128);
				ImGui::DragFloat("Metalic", &mat.metalic, 1, 0, 1);

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

						{
							const int texture_wrapping_item_selected_idx =
								(int)mat.texture_sampling[mat_tex_index].wrapping;

							/*	*/
							const std::string combo_preview_value = std::string(magic_enum::enum_name(
								(fragcore::TextureWrappingMode)texture_wrapping_item_selected_idx));
							ImGuiComboFlags flags = 0;
							if (ImGui::BeginCombo("Texture Wrapping", combo_preview_value.c_str(), flags)) {
								for (int n = 0; n <= (int)fragcore::TextureWrappingMode::ClampBorder; n++) {
									const bool is_selected = (texture_wrapping_item_selected_idx == n);

									if (ImGui::Selectable(
											magic_enum::enum_name((fragcore::TextureWrappingMode)n).data(),
											is_selected)) {
										mat.texture_sampling[mat_tex_index].wrapping = (fragcore::TextureWrappingMode)n;
									}

									if (is_selected) {
										ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::EndCombo();
							}
						}

						ImGui::SameLine();

						{
							const int texture_filtering_item_selected_idx =
								(int)mat.texture_sampling[mat_tex_index].filtering;

							/*	*/
							const std::string combo_preview_filter_value = std::string(magic_enum::enum_name(
								(fragcore::TextureFilterMode)texture_filtering_item_selected_idx));
							ImGuiComboFlags flags = 0;
							if (ImGui::BeginCombo("Texture Filtering", combo_preview_filter_value.c_str(), flags)) {
								for (int n = 0; n <= (int)fragcore::TextureFilterMode::Trilinear; n++) {
									const bool is_selected = (texture_filtering_item_selected_idx == n);

									if (ImGui::Selectable(magic_enum::enum_name((fragcore::TextureFilterMode)n).data(),
														  is_selected)) {
										mat.texture_sampling[mat_tex_index].filtering = (fragcore::TextureFilterMode)n;
									}

									if (is_selected) {
										ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::EndCombo();
							}
						}

						ImGui::PopID();
					}
				}
				ImGui::PopID();
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