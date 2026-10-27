/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	Verlihub is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see https://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#include "cworkerpool.h"
#include <system_error>

using namespace std;

namespace nVerliHub {
	namespace nThread {

cWorkerPool::~cWorkerPool()
{
	Stop();
}

bool cWorkerPool::Start(unsigned int threads, size_t max_jobs)
{
	if (mRunning)
		return false;

	if (!threads)
		threads = thread::hardware_concurrency();

	if (!threads) // note: hardware_concurrency can return 0 when unknown
		threads = 1;

	{
		lock_guard<mutex> lock(mJobLock);
		mStop = false;
		mMaxJobs = max_jobs;
	}

	mThreads.reserve(threads);

	try {
		for (unsigned int i = 0; i < threads; i++)
			mThreads.emplace_back(&cWorkerPool::WorkerLoop, this);

	} catch (const system_error&) { // note: thread creation failed, clean up those that were started
		mRunning = true;
		Stop();
		return false;
	}

	mRunning = true;
	return true;
}

void cWorkerPool::RequestStop()
{
	if (!mRunning)
		return;

	{
		lock_guard<mutex> lock(mJobLock);

		if (mStop) // note: already requested
			return;

		mStop = true;

		if (!mDrain) // note: in drain mode queued jobs are still done before threads exit
			mJobs.clear();
	}

	if (mOnStop) // note: abort blocking work of running jobs
		mOnStop();

	mJobCond.notify_all();
}

void cWorkerPool::Stop()
{
	if (!mRunning)
		return;

	RequestStop();

	for (auto &thr: mThreads) {
		if (thr.joinable())
			thr.join();
	}

	mThreads.clear();

	{
		lock_guard<mutex> lock(mResultLock);
		mResults.clear();
	}

	mRunning = false;
}

bool cWorkerPool::Post(tJob job)
{
	if (!job)
		return false;

	{
		lock_guard<mutex> lock(mJobLock);

		if (mStop || !mRunning || (mMaxJobs && (mJobs.size() >= mMaxJobs)))
			return false;

		mJobs.push_back(std::move(job));
	}

	mJobCond.notify_one();
	return true;
}

size_t cWorkerPool::RunResults(size_t max)
{
	size_t done = 0;

	while (!max || (done < max)) {
		tResult res;

		{
			lock_guard<mutex> lock(mResultLock);

			if (mResults.empty())
				break;

			res = std::move(mResults.front());
			mResults.pop_front();
		}

		res(); // note: run without lock, result is allowed to post new jobs
		done++;
	}

	return done;
}

void cWorkerPool::SetNotify(tNotify notify)
{
	if (!mRunning)
		mNotify = std::move(notify);
}

void cWorkerPool::SetOnStop(tNotify on_stop)
{
	if (!mRunning)
		mOnStop = std::move(on_stop);
}

void cWorkerPool::SetThreadHooks(tNotify start, tNotify stop)
{
	if (!mRunning) {
		mThreadStart = std::move(start);
		mThreadStop = std::move(stop);
	}
}

void cWorkerPool::SetDrain(bool drain)
{
	lock_guard<mutex> lock(mJobLock);
	mDrain = drain;
}

size_t cWorkerPool::JobsQueued() const
{
	lock_guard<mutex> lock(mJobLock);
	return mJobs.size();
}

size_t cWorkerPool::ResultsQueued() const
{
	lock_guard<mutex> lock(mResultLock);
	return mResults.size();
}

bool cWorkerPool::IsIdle() const
{
	lock_guard<mutex> lock(mJobLock); // note: job is taken from queue and marked busy under same lock
	return (mJobs.empty() && !mBusy);
}

void cWorkerPool::WorkerLoop()
{
	if (mThreadStart)
		mThreadStart();

	while (true) {
		tJob job;

		{
			unique_lock<mutex> lock(mJobLock);
			mJobCond.wait(lock, [this] { return (mStop || !mJobs.empty()); });

			if (mStop && (!mDrain || mJobs.empty())) // note: in drain mode thread exits only when all jobs are done
				break;

			job = std::move(mJobs.front());
			mJobs.pop_front();
			mBusy++;
		}

		tResult res;

		try {
			res = job();
		} catch (...) { // note: never let exception kill worker thread
			mFailed++;
		}

		job = nullptr; // note: release captured data on worker thread
		mBusy--;

		if (!res)
			continue;

		bool first = false;

		{
			lock_guard<mutex> lock(mResultLock);
			first = mResults.empty();
			mResults.push_back(std::move(res));
		}

		if (first && mNotify)
			mNotify();
	}

	if (mThreadStop)
		mThreadStop();
}

	}; // namespace nThread
}; // namespace nVerliHub
