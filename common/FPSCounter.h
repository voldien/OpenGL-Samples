/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Valdemar Lindberg
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
#include <stddef.h>

/**
 * @brief
 *
 * @tparam T
 */
template <typename T = double> class FPSCounter {
  public:
	FPSCounter(const int nrFPSSample = 50, const long int timeResolution = 1000000000) {
		this->fpsSample = nrFPSSample;
		this->timeResolution = timeResolution;

		this->averageFPS = 0;
		this->totalFPS = 0;
	}

	void enabled(const bool status) noexcept {}

	void incrementFPS(const long int timeSample) noexcept {

		if (totalFPS % fpsSample == 0) {
			/*  Compute number average FPS.  */
			long int deltaTime = timeSample - prevTimeSample;
			T sec = static_cast<T>(deltaTime) / static_cast<T>(timeResolution);
			averageFPS = static_cast<T>(fpsSample) / sec;
			prevTimeSample = timeSample;
		}
		totalFPS++;
	}

	void update(const float elapsedTime) noexcept {
		if (totalFPS % fpsSample == 0) {
		}
		totalFPS++;
	}

	unsigned int getFPS() const noexcept { return averageFPS; }

  protected:
	void internal_update(long int timeSample) noexcept {
		if (totalFPS % fpsSample == 0) {
			/*  Compute number average FPS.  */
			long int deltaTime = timeSample - prevTimeSample;
			T sec = static_cast<T>(deltaTime) / static_cast<T>(timeResolution);
			averageFPS = static_cast<T>(fpsSample) / sec;
			prevTimeSample = timeSample;

			this->totalFPS = 0;
		}
	}

  private:
	size_t totalFPS;
	int fpsSample;
	unsigned int averageFPS;
	long int prevTimeSample;
	long int timeResolution;
};
