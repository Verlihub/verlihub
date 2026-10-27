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

#ifndef NTHREADCWORKERPOOL_H
#define NTHREADCWORKERPOOL_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace nVerliHub {
	namespace nThread {

/*
	pool of worker threads for blocking or heavy work, keeps main thread free

	job runs on any worker thread and returns result function, or empty function when there is nothing to report
	result runs on main thread when it calls RunResults(), this is the only place where hub state may be touched
	with single thread, jobs run in same order as they were posted

	note: job must not touch hub state, copy everything it needs into lambda capture
	note: result must not trust raw pointers captured before job started, connection might be gone by then
*/

class cWorkerPool
{
	public:
		using tResult = std::function<void()>;
		using tJob = std::function<tResult()>;
		using tNotify = std::function<void()>;

		cWorkerPool() = default;
		~cWorkerPool();

		cWorkerPool(const cWorkerPool&) = delete;
		cWorkerPool& operator=(const cWorkerPool&) = delete;

		// start given number of threads, 0 = number of cpu cores, max_jobs limits job queue, 0 = unlimited
		bool Start(unsigned int threads, std::size_t max_jobs = 0);

		// running jobs finish, queued jobs and unprocessed results are dropped, must be called from main thread
		void Stop();

		// same as Stop() but without waiting for threads, call Stop() later to wait, used to stop many pools at once
		void RequestStop();

		// queue job, returns false when pool is not running or job queue is full
		bool Post(tJob job);

		// run up to max ready results on calling thread, 0 = all, returns number of results run
		std::size_t RunResults(std::size_t max = 0);

		/*
			called on worker thread when result queue changes from empty to non empty, used to wake up main loop
			must be set before Start(), must be thread safe and must not block
			note: when RunResults() leaves results in queue, caller is responsible to call it again
		*/
		void SetNotify(tNotify notify);

		/*
			called on main thread when stop is requested, before waiting for threads, used to abort blocking work of running jobs
			must be set before Start(), must not block
		*/
		void SetOnStop(tNotify on_stop);

		bool IsRunning() const
		{
			return mRunning;
		}

		std::size_t Threads() const
		{
			return mThreads.size();
		}

		std::size_t JobsQueued() const;
		std::size_t ResultsQueued() const;

		// no job is queued or running, note: result of finished job can still be waiting
		bool IsIdle() const;

		unsigned int JobsBusy() const
		{
			return mBusy;
		}

		// number of jobs that threw an exception, they are dropped without result
		unsigned long JobsFailed() const
		{
			return mFailed;
		}

	private:
		void WorkerLoop();

		std::vector<std::thread> mThreads;
		std::deque<tJob> mJobs;
		std::deque<tResult> mResults;
		mutable std::mutex mJobLock;
		mutable std::mutex mResultLock;
		std::condition_variable mJobCond;
		tNotify mNotify;
		tNotify mOnStop;
		std::size_t mMaxJobs = 0;
		bool mStop = false; // note: guarded by mJobLock
		std::atomic<bool> mRunning{false};
		std::atomic<unsigned int> mBusy{0};
		std::atomic<unsigned long> mFailed{0};
};

	}; // namespace nThread
}; // namespace nVerliHub

#endif
