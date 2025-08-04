#pragma once

#include <thread>
#include <mutex>
#include <queue>

#include "Common.h"

class CThread {

	std::thread m_Thread;

	std::mutex m_Mutex;

	std::condition_variable m_Cv;

	std::queue<std::function<void()>> m_Tasks;

	bool m_Stop = false;

public:

	CThread(): m_Thread(
		[this] {
			while (true) {
				std::function<void()> task;
				std::unique_lock lock(m_Mutex);

				m_Cv.wait(lock, [this] {
					return !m_Tasks.empty() || m_Stop;
				});

				if (m_Stop && m_Tasks.empty()) return;

				task = std::move(m_Tasks.front());
				task();

				// Remove the task, also tells the main thread that execution has finished
				m_Tasks.pop();
			}
		}
	) {}

	~CThread() {
		{
			std::unique_lock lock(m_Mutex);
			m_Stop = true;
		}

		m_Cv.notify_all();

		m_Thread.join();
	}

	no_discard bool isThreadRunning() const {
		return m_Thread.joinable();
	}

	void run(const std::function<void()>& inFunc) {
		{
			std::unique_lock lock(m_Mutex);
			m_Tasks.emplace(inFunc);
		}
		m_Cv.notify_one();
	}

	void wait() {
		while (!m_Tasks.empty()) {
			std::unique_lock lock(m_Mutex);
		}
	}
};

class CThreading {

	static CThreading& get() {
		static CThreading threading;
		return threading;
	}

public:

	static CThread& getRenderingThread() { return get().mRenderingThread; }

	static CThread& getGameThread() { return get().mRenderingThread; }

	CThread mRenderingThread;

	CThread mGameThread;
};
