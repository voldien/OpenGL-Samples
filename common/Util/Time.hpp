#pragma once
#include <SDL2/SDL_timer.h>

namespace glsample {

	class Time {
	  public:
		Time() {
			this->_private_level_startup = SDL_GetPerformanceCounter();

			this->timeResolution = SDL_GetPerformanceFrequency();
		}

		void start() { this->ticks = SDL_GetPerformanceCounter(); }

		float getElapsed() const noexcept {
			return static_cast<float>(SDL_GetPerformanceCounter() - this->_private_level_startup) /
				   static_cast<float>(this->timeResolution);
		}
		float deltaTime() const noexcept {
			return static_cast<float>(this->delta_data) / static_cast<float>(this->timeResolution);
		}
		void update() {
			this->delta_data = SDL_GetPerformanceCounter() - this->ticks;
			this->ticks = SDL_GetPerformanceCounter();
		}

	  private:
		/*  */
		unsigned long int ticks;
		float scale;
		float fixed;

		/*	TODO clean up later by relocating it to the time class.*/
		float gc_fdelta;
		float delta_data;
		unsigned int idelta;

		/*  */

		unsigned long timeResolution;
		unsigned long _private_level_startup;
	};

} // namespace glsample
