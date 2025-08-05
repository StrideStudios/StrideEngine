#pragma once

#include <thread>
#include <mutex>
#include <deque>

#include "Common.h"
#include "tracy/Tracy.hpp"
#include "tracy/TracyC.h"

class CWorker {
public:
	std::mutex m_QueueMutex;

	std::condition_variable m_QueueCV;

	std::condition_variable m_FinishedCV;

	std::deque<std::function<void()>> m_Tasks;

	bool m_Stop = false;

public:

	CWorker() = default;

	bool execute() {
		std::function<void()> task;
		{
			std::unique_lock lock(m_QueueMutex);
			m_QueueCV.wait(lock, [this] {
				return !m_Tasks.empty() || m_Stop;
			});

			if (m_Stop && m_Tasks.empty()) return true;

			// Get task and remove from tasks
			task = std::move(m_Tasks.front());
			m_Tasks.pop_front();
		}

		task();

		if (m_Tasks.empty()) {
			m_FinishedCV.notify_all();
		}

		return false;
	}

	void executeAll() {
		while (getNumberOfTasks() > 0) {
			std::function<void()> task;
			{
				std::unique_lock lock(m_QueueMutex);
				/*m_QueueCV.wait(lock, [this] {
					return !m_Tasks.empty() || m_Stop;
				});*/

				// Get task and remove from tasks
				task = std::move(m_Tasks.front());
				m_Tasks.pop_front();
			}

			task();

			if (m_Tasks.empty()) {
				m_FinishedCV.notify_all();
			}
		}
	}

	no_discard int32 getNumberOfTasks() const {
		return m_Tasks.size();
	}

	no_discard bool isWorkerRunning() const {
		return !m_Tasks.empty();
	}

	void stop() {
		{
			std::lock_guard lock(m_QueueMutex);
			m_Tasks.clear();
			m_Stop = true;
		}
		m_QueueCV.notify_all();
	}

	void add(const std::function<void()>& inFunc) {
		{
			std::lock_guard lock(m_QueueMutex);
			m_Tasks.push_back(std::move(inFunc));
		}
		m_QueueCV.notify_all();
	}

	void wait() {
		std::unique_lock lock(m_QueueMutex);
		m_FinishedCV.wait(lock, [this] {return m_Tasks.empty();});
	}
};

class CThread {

	std::thread m_Thread;

	CWorker m_Worker;

public:

	explicit CThread(const std::function<void(std::function<bool()>)>& inFunc): m_Thread([this, inFunc] {
		const std::function<bool()> loop = [this] {
			return m_Worker.execute();
		};
		inFunc(loop);
	}) {}

	~CThread() {
		m_Worker.stop();
		m_Thread.join();
	}

	no_discard const CWorker& getWorker() const {
		return m_Worker;
	}

	void run(const std::function<void()>& inFunc) {
		m_Worker.add(inFunc);
	}

	void wait() {
		m_Worker.wait();
	}
};

#define THREAD_LOOP(name, color) \
	TracyCSetThreadName(name.c_str()) \
	while (true) { \
		TracyCZone(ctx, 1); \
		std::string threadName = name; \
		TracyCZoneName(ctx, threadName.c_str(), threadName.size()); \
		TracyCZoneColor(ctx, color); \
		if (loop()) return; \
		TracyCZoneEnd(ctx); \
	}

class CThreadPool {

	std::vector<std::shared_ptr<CThread>> m_Threads;

public:

	explicit CThreadPool(const uint32 inNumThreads = std::thread::hardware_concurrency()) {
		for (uint32 i = 0; i < inNumThreads; ++i) {
			uint32 currentColor = i * (255u / inNumThreads);
			m_Threads.push_back(std::make_shared<CThread>([i, currentColor](const std::function<bool()>& loop) {
				THREAD_LOOP(fmts("Background Thread {}", i), currentColor)
			}));
		}
	}

	// Run on least busy thread
	void run(const std::function<void()>& inFunc) const {
		std::shared_ptr<CThread> lowestThread = nullptr;
		for (const auto& thread : m_Threads) {
			if (!lowestThread || thread->getWorker().getNumberOfTasks() < lowestThread->getWorker().getNumberOfTasks()) {
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

	static CWorker& getMainThread() { return get().mMainThread; }

	static CThread& getRenderingThread() { return get().mRenderingThread; }

	static CThread& getGameThread() { return get().mGameThread; }

	static void runOnBackgroundThread(const std::function<void()>& inFunc) {
		get().mThreadPool.run(inFunc);
	}

	CWorker mMainThread;

	CThread mRenderingThread{[](const std::function<bool()>& loop) {
		THREAD_LOOP(std::string("Render Thread"), 0xff0000)
	}};

	CThread mGameThread{[](const std::function<bool()>& loop) {
		THREAD_LOOP(std::string("Game Thread"), 0x00ff00)
	}};

	// Modern computers can reliably have > 4 cores ( 8 threads)
	CThreadPool mThreadPool{6};
};
