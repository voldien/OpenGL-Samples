/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Valdemar Lindberg
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
#include <cstddef>

namespace glsample {

	/**
	 * @brief
	 *
	 * @tparam T
	 */
	template <typename T = float> class FPSCounter {
	  public:
		FPSCounter(const unsigned int nrFPSSample = 50, const size_t timeResolution = 1000000000) {
			this->fpsSample = nrFPSSample;
			this->timeResolution = timeResolution;

			this->averageFPS = 0;
			this->totalFPS = 0;
		}

		void update(const float elapsedTime) noexcept { this->incrementFPS(elapsedTime); }

		unsigned int getFPS() const noexcept { return this->averageFPS; }

	  protected:
		void incrementFPS(const float timeSample) noexcept {

			if (this->totalFPS % fpsSample == 0) {
				/*  Compute number average FPS.  */
				const float deltaTime = timeSample - prevTimeSample;
				this->averageFPS = static_cast<T>(fpsSample) / deltaTime;

				this->prevTimeSample = timeSample;
			}

			this->totalFPS++;
		}

	  private:
		size_t totalFPS{};
		unsigned int fpsSample{};
		unsigned int averageFPS{};
		float prevTimeSample{};
		long int timeResolution{};
	};

} // namespace glsample