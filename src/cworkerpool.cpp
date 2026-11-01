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
#include <chrono>
#include <system_error>

#ifdef _WIN32
	#include <windows.h>

	#if defined(__x86_64__) || defined(__i386__)
		#include <x86intrin.h>
		#define VH_THREAD_CYCLES
	#endif
#else
	#include <pthread.h>
	#include <time.h>

	#if defined(__FreeBSD__) || defined(__OpenBSD__)
		#include <pthread_np.h>
	#endif
#endif

using namespace std;

namespace nVerliHub {
	namespace nThread {

atomic<bool> cWorkerPool::sMeasure{false};

static unsigned long long SteadyNS()
{
	return (unsigned long long)chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now().time_since_epoch()).count();
}

static void SetThreadName(const string &name) // note: name of calling thread, seen in top, ps and debuggers, linux allows 15 characters
{
	const string full(("vh-" + name).substr(0, 15));

#ifdef _WIN32
	typedef HRESULT (WINAPI *tSetThreadDescription)(HANDLE, PCWSTR);
	const HMODULE kern = GetModuleHandleW(L"kernel32.dll");
	const tSetThreadDescription set_desc = (kern ? (tSetThreadDescription)(void*)GetProcAddress(kern, "SetThreadDescription") : NULL); // note: windows 10 1607 and later

	if (set_desc) {
		const wstring wide(full.begin(), full.end());
		set_desc(GetCurrentThread(), wide.c_str());
	}
#elif defined(__linux__)
	pthread_setname_np(pthread_self(), full.c_str());
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	pthread_set_name_np(pthread_self(), full.c_str());
#elif defined(__NetBSD__)
	pthread_setname_np(pthread_self(), "%s", (void*)full.c_str());
#elif defined(__APPLE__)
	pthread_setname_np(full.c_str());
#endif
}

unsigned long long cWorkerPool::ThreadCPU()
{
#if defined(VH_THREAD_CYCLES)
	ULONG64 cycles = 0;

	if (!QueryThreadCycleTime(GetCurrentThread(), &cycles))
		return 0;

	return cycles;
#elif defined(_WIN32)
	FILETIME create, leave, kern, user;

	if (!GetThreadTimes(GetCurrentThread(), &create, &leave, &kern, &user)) // note: counted in scheduler ticks, single job is not exact, sum of many is
		return 0;

	return (((((unsigned long long)kern.dwHighDateTime) << 32) | kern.dwLowDateTime) + ((((unsigned long long)user.dwHighDateTime) << 32) | user.dwLowDateTime)) * 100ULL;
#else
	struct timespec now;

	if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now))
		return 0;

	return (((unsigned long long)now.tv_sec * 1000000000ULL) + (unsigned long long)now.tv_nsec);
#endif
}

bool cWorkerPool::CPUCycles()
{
#if defined(VH_THREAD_CYCLES)
	return true;
#else
	return false;
#endif
}

unsigned long long cWorkerPool::CycleCounter()
{
#if defined(VH_THREAD_CYCLES)
	return __rdtsc(); // note: thread cycle time of windows is counted with same counter
#else
	return 0;
#endif
}

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

void cWorkerPool::SetName(const string &name)
{
	if (!mRunning)
		mName = name;
}

void cWorkerPool::TakeStats(unsigned long long &jobs, unsigned long long &time, unsigned long long &cpu, unsigned long long &max)
{
	jobs = mStatJobs.exchange(0);
	time = mStatTime.exchange(0);
	cpu = mStatCPU.exchange(0);
	max = mStatMax.exchange(0);
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
	if (mName.size())
		SetThreadName(mName);

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
		const bool measure = sMeasure;
		const unsigned long long start = (measure ? SteadyNS() : 0), cpu = (measure ? ThreadCPU() : 0);

		try {
			res = job();
		} catch (...) { // note: never let exception kill worker thread
			mFailed++;
		}

		job = nullptr; // note: release captured data on worker thread

		if (measure) {
			const unsigned long long now = SteadyNS(), took = ((now > start) ? (now - start) : 0), used = ThreadCPU();
			mStatJobs++;
			mStatTime += took;
			mStatCPU += ((used > cpu) ? (used - cpu) : 0);
			unsigned long long max = mStatMax;

			while (took > max) { // note: max changes only to higher value, failed exchange loads current value into max
				if (mStatMax.compare_exchange_weak(max, took))
					break;
			}
		}

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
