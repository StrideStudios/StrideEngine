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

	CThread(): m_Thread([this] {
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
	}) {}

	// no copying
	//CThread(const CThread&) = delete;

	~CThread() {
		{
			std::unique_lock lock(m_Mutex);
			m_Stop = true;
		}

		m_Cv.notify_all();

		m_Thread.join();
	}

	no_discard int32 getNumberOfTasks() const {
		return m_Tasks.size();
	}

	no_discard bool isThreadRunning() const {
		return !m_Tasks.empty();
	}

	void run(const std::function<void()>& inFunc) {
		{
			std::unique_lock lock(m_Mutex);
			m_Tasks.emplace(inFunc);
		}
		m_Cv.notify_one();
	}

	void wait() {
		while (isThreadRunning()) {
			std::unique_lock lock(m_Mutex);
		}
	}
};

class CThreadPool {

	std::vector<std::shared_ptr<CThread>> m_Threads;

public:

	explicit CThreadPool(const uint32 inNumThreads = std::thread::hardware_concurrency()) {
		for (uint32 i = 0; i < inNumThreads; ++i)
			m_Threads.push_back(std::make_shared<CThread>());
	}

	// Run on least busy thread
	void run(const std::function<void()>& inFunc) const {
		std::shared_ptr<CThread> lowestThread = nullptr;
		for (const auto& thread : m_Threads) {
			if (!lowestThread || thread->getNumberOfTasks() < lowestThread->getNumberOfTasks()) {
				lowestThread = thread;
			}
		}
		asts(lowestThread, "Could not find thread to run task on!");
		lowestThread->run(inFunc);
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

	static void runOnBackgroundThread(const std::function<void()>& inFunc) {
		get().mThreadPool.run(inFunc);
	}

	CThread mRenderingThread;

	CThread mGameThread;

	// Modern computers can reliably have > 4 cores ( 8 threads)
	CThreadPool mThreadPool{6};
};
