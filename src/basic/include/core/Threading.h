#pragma once

#include <thread>
#include <mutex>
#include <deque>

#include "Profiling.h"

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

	//TODO: shouldnt be necessary, but 'main thread' needs it for now
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
		stop();
	}

	no_discard const CWorker& getWorker() const {
		return m_Worker;
	}

	void run(const std::function<void()>& inFunc) {
		m_Worker.add(inFunc);
	}

	void stop() {
		m_Worker.stop();
		wait();
		if (m_Thread.joinable()) m_Thread.join();
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

	std::vector<std::thread> m_Threads{};

	CWorker m_Worker;

public:

	explicit CThreadPool(const uint32 inNumThreads = std::thread::hardware_concurrency()) {
		for (uint32 i = 0; i < inNumThreads; ++i) {
			const std::string name = fmts("Background Thread {}", i);
			uint32 color = i * (255u / inNumThreads);
			m_Threads.emplace_back([this, name, color] {
				TracyCSetThreadName(name.c_str())
				while (true) {
					TracyCZone(ctx, 1);
					TracyCZoneName(ctx, name.c_str(), name.size());
					TracyCZoneColor(ctx, color);
					if (m_Worker.execute()) return;
					TracyCZoneEnd(ctx);
				}
			});
		}
	}

	~CThreadPool() {
		stop();
	}

	// Whichever thread gets to the task first will run it
	void run(const std::function<void()>& inFunc) {
		m_Worker.add(inFunc);
	}

	void wait() {
		m_Worker.wait();
	}

	void stop() {
		m_Worker.stop();
		wait();
		for (auto& thread : m_Threads) {
			if (thread.joinable()) thread.join();
		}
	}

};

class CThreading {

public:

	EXPORT static CWorker& getMainThread();

	EXPORT static CThread& getRenderingThread();

	EXPORT static CThread& getGameThread();

	EXPORT static void runOnBackgroundThread(const std::function<void()>& inFunc);

	// Wait for threads to finish operations
	EXPORT static void wait();

	// Not guaranteed to finish all operations (some may stop prematurely)
	EXPORT static void stop();

	CWorker mMainThread;

	CThread mRenderingThread{[](const std::function<bool()>& loop) {
		THREAD_LOOP(std::string("Render Thread"), 0xff0000)
	}};

	CThread mGameThread{[](const std::function<bool()>& loop) {
		THREAD_LOOP(std::string("Game Thread"), 0x00ff00)
	}};

	// Modern computers can reliably have > 4 cores ( 8 threads)
	CThreadPool mThreadPool{std::thread::hardware_concurrency()};
};
