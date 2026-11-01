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

#ifndef NUTILSCPROFILER_H
#define NUTILSCPROFILER_H

#include <cstddef>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace nVerliHub {
	namespace nThread {
		class cWorkerPool;
	};

	namespace nUtils {

/*
	measures time of hub sections on main thread and work of worker threads
	data is kept in one minute parts for last hour, one hour parts for last day and total since start or reset

	section time is own time of section, time of nested section is counted only in nested one, for example lua script called during protocol command
	main thread time is clock time, so blocking call shows its whole wait, thread time is cpu time

	note: everything is used only from main thread, worker pools count their own work and profiler takes it once per second
*/

class cProfiler
{
	public:
		cProfiler();

		cProfiler(const cProfiler&) = delete;
		cProfiler& operator=(const cProfiler&) = delete;

		bool mEnabled = false; // note: configuration adv_enable_profiler

		// id of section with given name, same name always gives same id
		unsigned int Id(const std::string &name);

		// start and end of section, use cProfileScope instead, without count only time is added, for example second part of same call
		void Begin(unsigned int id, bool count = true);
		void End();

		// section of database query, named after section that runs it and mQueryDetail
		unsigned int QueryId();

		// what runs next database queries inside current section, for example calling Lua function, set and cleared by caller, empty = nothing
		std::string mQueryDetail;

		// change id of current section, for example when protocol command is known only after parsing
		void SetId(unsigned int id);

		// called every main loop step, moves to next minute and takes work of threads once per second
		void Tick();

		// worker pool whose threads are shown, same order as server pool list
		void AddPool(nThread::cWorkerPool *pool);
		void ClearPools();

		// section of results of pool at given position
		unsigned int PoolResults(std::size_t pos) const
		{
			return ((pos < mPools.size()) ? mPools[pos].mResults : mOther);
		}

		// clear all collected data
		void Reset();

		// show data of last period in seconds, 0 = all time
		void Show(std::ostream &os, unsigned int period) const;

		// longest period that Show() accepts, in seconds
		static constexpr unsigned int sMaxPeriod = 24 * 60 * 60;

		static unsigned long long SteadyNS();

	private:
		struct sPart // note: times are nanoseconds
		{
			unsigned long long mCount = 0;
			unsigned long long mTime = 0;
			unsigned long long mCPU = 0;
			unsigned long long mMax = 0;
			long long mStamp = -1; // note: number of minute or hour of this part

			void Add(const sPart &part);
		};

		struct sSection
		{
			std::string mName;
			bool mThread = false; // note: worker pool or main thread, not section
			unsigned int mThreads = 0;
			sPart mMinute[60];
			sPart mHour[24];
			sPart mTotal;
		};

		struct sFrame
		{
			unsigned int mId;
			unsigned long long mStart;
			unsigned long long mChild; // note: time of nested sections
			bool mCount;
		};

		struct sPool
		{
			nThread::cWorkerPool *mPool;
			unsigned int mThread;
			unsigned int mResults;
		};

		unsigned int Add(const std::string &name, bool thread);
		void Record(unsigned int id, unsigned long long count, unsigned long long time, unsigned long long cpu, unsigned long long max);
		void TakeThreads(bool keep, unsigned long long now);
		double CPUScale(unsigned long long now) const;
		void CloseMinute();
		sPart Sum(const sSection &sec, unsigned int period, long long minute) const;
		static std::string Time(unsigned long long ns);
		static std::string Share(unsigned long long part, unsigned long long all);

		std::vector<sSection> mSections;
		std::unordered_map<std::string, unsigned int> mIds;
		std::vector<sFrame> mStack;
		std::vector<sPool> mPools;
		std::vector<unsigned int> mQueryIds; // note: query section for each section id, 0 = not made yet
		long long mMinute; // note: minute of steady clock that is collected now
		long long mSecond = -1; // note: last second when threads were taken
		bool mWasEnabled = false;
		unsigned long long mMainCPU = 0;
		unsigned long long mCycleStart = 0; // note: cycle counter and time when profiler was enabled, to convert thread cpu cycles to time
		unsigned long long mCycleTime = 0;
		unsigned long long mSteps = 0;
		unsigned int mMain;
		unsigned int mOther;
};

/*
	measures section until end of scope, does nothing when profiler is disabled
*/

class cProfileScope
{
	public:
		cProfileScope(cProfiler *prof, unsigned int id, bool count = true):
			mProf((prof && prof->mEnabled) ? prof : nullptr)
		{
			if (mProf)
				mProf->Begin(id, count);
		}

		~cProfileScope()
		{
			if (mProf)
				mProf->End();
		}

		cProfileScope(const cProfileScope&) = delete;
		cProfileScope& operator=(const cProfileScope&) = delete;

		void SetId(unsigned int id)
		{
			if (mProf)
				mProf->SetId(id);
		}

	private:
		cProfiler *mProf;
};

	}; // namespace nUtils
}; // namespace nVerliHub

#endif
