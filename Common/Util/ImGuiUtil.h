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

#include "imgui.h"
#include "magic_enum.hpp"
#include <functional>
#include <string>

namespace glsample {

	template <typename T>
	void enumComboBox(const char *label, const T currentSelected, const size_t maxEnums,
					  std::function<void(const T selected)> onSelected) {

		const int item_selected_idx = (int)currentSelected;

		const std::string combo_preview_value = std::string(magic_enum::enum_name(currentSelected));

		const ImGuiComboFlags flags = 0;
		ImGui::SetNextItemWidth(256);
		if (ImGui::BeginCombo(label, combo_preview_value.c_str(), flags)) {
			for (size_t nth_enum = 0; nth_enum < maxEnums; nth_enum++) {
				const bool is_selected = (item_selected_idx == nth_enum);

				if (ImGui::Selectable(magic_enum::enum_name((T)nth_enum).data(), is_selected)) {
					onSelected((T)nth_enum);
				}

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	/*	*/
	template <typename T, size_t n> struct plot_graph_t {
		/*	*/
		std::array<T, n> data{};
		size_t offset = 0;
	};
	using PlotGraph = plot_graph_t<float, 512>;

	template <typename T> extern void setNext(PlotGraph &plot, const T &value) {
		plot.data[plot.offset] = value;
		plot.offset = Math::mod<size_t>(plot.offset + 1, plot.data.size());
	}

	template <typename T> extern T getLatest(const PlotGraph &plot) {
		return plot.data[Math::mod<size_t>(plot.offset - 1, plot.data.size())];
	}

} // namespace glsample