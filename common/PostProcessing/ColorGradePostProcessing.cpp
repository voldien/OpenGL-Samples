#include "PostProcessing/ColorGradePostProcessing.h"
using namespace glsample;

ColorGradePostProcessing::ColorGradePostProcessing() { this->setName("ColorGrade"); }
ColorGradePostProcessing::~ColorGradePostProcessing() {
	if (this->hue_color_grade_program >= 0) {
		glDeleteProgram(this->hue_color_grade_program);
	}
}

void ColorGradePostProcessing::initialize(fragcore::IFileSystem *filesystem) {}
