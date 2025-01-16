// #include "GLUIComponent.h"

// using namespace glsample;

// template <typename T>
// void GLUIComponent<T>::drawCamera(Camera<float> &camera) {

// 	float fov = camera.getFOV();
// 	if (ImGui::SliderFloat("Field of View", &fov, 0.01f, 360.0f)) {
// 		camera.setFOV(fov);
// 	}
// 	float near = camera.getNear();
// 	if (ImGui::DragFloat("Near Distance", &near, 1, 0.001f)) {
// 		camera.setNear(near);
// 	}
// 	float far = camera.getFar();
// 	if (ImGui::DragFloat("Far Distance", &far)) {
// 		camera.setFar(far);
// 	}
// }

// template <typename T>
// void GLUIComponent<T>::drawCameraController(CameraController &cameraController) {

// 	ImGui::TextUnformatted("Camera Controller");
// 	glm::vec3 position = cameraController.getPosition();
// 	if (ImGui::DragFloat3("Position", &position[0])) {
// 		cameraController.setPosition(position);
// 	}
// 	glm::vec3 rotation = cameraController.getRotation();
// 	if (ImGui::DragFloat3("Rotation", &rotation[0])) {
// 		cameraController.setRotation(rotation);
// 	}
// 	drawCamera(cameraController);
// }