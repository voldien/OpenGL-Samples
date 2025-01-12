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
#include "Util/Camera.h"
#include "Util/CameraController.h"
#include "imgui.h"
#include <UIComponent.h>

namespace glsample {

	/**
	 * @brief
	 *
	 * @tparam T
	 */
	template <typename T> class GLUIComponent : public nekomimi::UIComponent {
	  public:
		GLUIComponent(T &base, const std::string &name = "Sample Settings") : base(base) { this->setName(name); }

		void draw() override = 0;

	  protected:
		// TODO: move to cpp
		void drawCamera(Camera<float> &camera) {

			float fov = camera.getFOV();
			if (ImGui::SliderFloat("Field of View", &fov, 0.01f, 360.0f)) {
				camera.setFOV(fov);
			}
			float near = camera.getNear();
			if (ImGui::DragFloat("Near Distance", &near, 1, 0.001f)) {
				camera.setNear(near);
			}
			float far = camera.getFar();
			if (ImGui::DragFloat("Far Distance", &far)) {
				camera.setFar(far);
			}
		}

		void drawCameraController(CameraController &cameraController) {

			ImGui::TextUnformatted("Camera Controller");
			glm::vec3 position = cameraController.getPosition();
			if (ImGui::DragFloat3("Position", &position[0])) {
				cameraController.setPosition(position);
			}
			glm::vec3 rotation = cameraController.getRotation();
			if (ImGui::DragFloat3("Rotation", &rotation[0])) {
				cameraController.setRotation(rotation);
			}
			drawCamera(cameraController);
		}

	  protected:
		T &getRefSample() const { return this->base; }

	  private:
		T &base;
	};
} // namespace glsample