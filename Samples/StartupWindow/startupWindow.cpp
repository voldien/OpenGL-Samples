#include "GLSampleWindow.h"
#include "GLWindow.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <iostream>

class SampleComponent : public MIMIIMGUI::UIComponent {
  private:
  public:
	SampleComponent() { this->setName("Sample Window"); }
	virtual void draw() override {

		ImGui::ColorEdit4("color 1", color);
		if (ImGui::Button("Press me")) {
		}
	}
	float color[4];
};

class StartUpWindow : public GLSampleWindow {
  public:
	std::shared_ptr<SampleComponent> com;
	StartUpWindow() : GLSampleWindow() {
		com = std::make_shared<SampleComponent>();
		this->addUIComponent(com);
	}
	virtual void Release() override {}

	virtual void Initialize() override {

		int w, h;
		getSize(&w, &h);
		onResize(w, h);
	}

	virtual void draw() override {

		int w, h;
		getSize(&w, &h);
		glClearColor(com->color[0], com->color[1], com->color[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	virtual void onResize(int width, int height) { glViewport(0, 0, width, height); }

  private:
	float color[4];
};

int main(int argc, const char **argv) {
	try {
		GLSample<StartUpWindow> sample(argc, argv);
		sample.run();
	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
