#include "core/Threading.h"

CThreading gThreading;

CWorker& CThreading::getMainThread() { return gThreading.mMainThread; }

CThread& CThreading::getRenderingThread() { return gThreading.mRenderingThread; }

CThread& CThreading::getGameThread() { return gThreading.mGameThread; }

void CThreading::runOnBackgroundThread(const std::function<void()>& inFunc) {
	gThreading.mThreadPool.run(inFunc);
}

void CThreading::wait() {
	gThreading.mRenderingThread.wait();
	gThreading.mGameThread.wait();
	gThreading.mThreadPool.wait();
}

void CThreading::stop() {
	gThreading.mRenderingThread.stop();
	gThreading.mGameThread.stop();
	gThreading.mThreadPool.stop();
}