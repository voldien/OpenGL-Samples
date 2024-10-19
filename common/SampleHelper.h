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
#include <Eigen/Eigen>
#include <glm/matrix.hpp>

namespace glsample {

	template <typename T, int m, int n>
	inline glm::mat<m, n, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, n> &em) noexcept {
		glm::mat<m, n, float, glm::precision::highp> mat;
		for (int i = 0; i < m; ++i) {
			for (int j = 0; j < n; ++j) {
				mat[j][i] = em(i, j);
			}
		}
		return mat;
	}

	template <typename T, int m>
	inline glm::vec<m, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, 1> &em) noexcept {
		glm::vec<m, float, glm::precision::highp> v;
		for (int i = 0; i < m; ++i) {
			v[i] = em(i);
		}
		return v;
	}

	template <typename T, int m> inline Eigen::Matrix<T, m, 1> GLM2E(const glm::vec<m, T> &em) noexcept {
		Eigen::Matrix<T, m, 1> v;
		for (int i = 0; i < m; ++i) {
			v(i) = em[i];
		}
		return v;
	}

	template <typename T, int m, int n> inline Eigen::Matrix<T, m, n> GLM2E(const glm::mat<m, n, T> &em) noexcept {
		Eigen::Matrix<T, m, n> mat;
		for (int i = 0; i < m; ++i) {
			for (int j = 0; j < n; ++j) {
				mat(j, i) = em[i][j];
			}
		}
		return mat;
	}
} // namespace glsample