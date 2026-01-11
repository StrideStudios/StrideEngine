#include "basic/core/Threading.h"

CThreading gThreading;

CWorker& CThreading::getMainThread() { return gThreading.mMainThread; }

void CThreading::runOnBackgroundThread(const std::function<void()>& inFunc) {
	gThreading.mThreadPool.run(inFunc);
}

void CThreading::wait() {
	gThreading.mThreadPool.wait();
}

void CThreading::stop() {
	gThreading.mThreadPool.stop();
}