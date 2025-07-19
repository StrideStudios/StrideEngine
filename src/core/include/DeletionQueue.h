#pragma once

#include <deque>
#include <functional>
#include <ranges>

struct SDeletionQueue {
	//TODO: for thousands of objects this will be very slow
	std::deque<std::function<void()>> m_Deletors;

	void push(std::function<void()>&& inFunction) {
		m_Deletors.push_back(inFunction);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto & m_Deletor : std::ranges::reverse_view(m_Deletors)) {
			m_Deletor(); //call functors
		}

		m_Deletors.clear();
	}
};