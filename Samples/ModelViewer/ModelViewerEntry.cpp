#include "ModelViewer.h"

using namespace glsample;

class ModelViewerGLSample : public GLSample<ModelViewer> {
  public:
	ModelViewerGLSample() : GLSample<ModelViewer>() {}

	void customOptions(cxxopts::OptionAdder &options) override {
		//			options("T,texture", "Texture Path",
		//					cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
	}
};

int main(int argc, const char **argv) {
	try {
		ModelViewerGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}