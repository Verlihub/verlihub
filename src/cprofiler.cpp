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

#include "cprofiler.h"
#include "cworkerpool.h"
#include "ctime.h"
#include "i18n.h"

#include <algorithm>
#include <chrono>
#include <cstdio>

using namespace std;

namespace nVerliHub {
	namespace nUtils {

static long long SteadyMinute()
{
	return chrono::duration_cast<chrono::minutes>(chrono::steady_clock::now().time_since_epoch()).count();
}

unsigned long long cProfiler::SteadyNS()
{
	return (unsigned long long)chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now().time_since_epoch()).count();
}

void cProfiler::sPart::Add(const sPart &part)
{
	mCount += part.mCount;
	mTime += part.mTime;
	mCPU += part.mCPU;

	if (part.mMax > mMax)
		mMax = part.mMax;
}

cProfiler::cProfiler():
	mMinute(SteadyMinute())
{
	mStack.reserve(16);
	mMain = Add("main", true);
	mOther = Add("Results", false);
}

unsigned int cProfiler::Add(const string &name, bool thread)
{
	const string key((thread ? "thread " : "") + name); // note: thread and section can have same name
	const auto it = mIds.find(key);

	if (it != mIds.end())
		return it->second;

	const unsigned int id = (unsigned int)mSections.size();
	mSections.emplace_back();
	mSections.back().mName = name;
	mSections.back().mThread = thread;
	mIds.emplace(key, id);
	return id;
}

unsigned int cProfiler::Id(const string &name)
{
	return Add(name, false);
}

void cProfiler::Begin(unsigned int id, bool count)
{
	mStack.push_back({id, SteadyNS(), 0, count});
}

void cProfiler::End()
{
	if (mStack.empty())
		return;

	const sFrame frame = mStack.back();
	mStack.pop_back();
	const unsigned long long now = SteadyNS(), all = ((now > frame.mStart) ? (now - frame.mStart) : 0), own = ((all > frame.mChild) ? (all - frame.mChild) : 0);

	if (mStack.size())
		mStack.back().mChild += all;

	Record(frame.mId, (frame.mCount ? 1 : 0), own, 0, own);
}

unsigned int cProfiler::QueryId()
{
	if (mStack.empty()) // note: query outside of measured section
		return Id(mQueryDetail.size() ? ("MySQL query: " + mQueryDetail) : string("MySQL query"));

	const unsigned int parent = mStack.back().mId;

	if (mQueryDetail.size()) {
		const string name("MySQL in " + mSections[parent].mName + ": " + mQueryDetail); // note: copy before new section is added
		return Id(name);
	}

	if (parent >= mQueryIds.size())
		mQueryIds.resize(parent + 1, 0);

	if (!mQueryIds[parent]) {
		const string name("MySQL in " + mSections[parent].mName); // note: copy before new section is added
		mQueryIds[parent] = Id(name);
	}

	return mQueryIds[parent];
}

void cProfiler::SetId(unsigned int id)
{
	if (mStack.size())
		mStack.back().mId = id;
}

void cProfiler::Record(unsigned int id, unsigned long long count, unsigned long long time, unsigned long long cpu, unsigned long long max)
{
	if (id >= mSections.size())
		return;

	sPart &part = mSections[id].mMinute[mMinute % 60];

	if (part.mStamp != mMinute) { // note: part of older minute was already added to its hour and total when that minute was closed
		part = sPart();
		part.mStamp = mMinute;
	}

	part.mCount += count;
	part.mTime += time;
	part.mCPU += cpu;

	if (max > part.mMax)
		part.mMax = max;
}

void cProfiler::CloseMinute()
{
	const long long hour = (mMinute / 60);

	for (auto &sec: mSections) {
		const sPart &part = sec.mMinute[mMinute % 60];

		if (part.mStamp != mMinute)
			continue;

		sPart &sum = sec.mHour[hour % 24];

		if (sum.mStamp != hour) {
			sum = sPart();
			sum.mStamp = hour;
		}

		sum.Add(part);
		sec.mTotal.Add(part);
	}
}

void cProfiler::AddPool(nThread::cWorkerPool *pool)
{
	if (!pool)
		return;

	sPool item;
	item.mPool = pool;
	item.mThread = Add(pool->Name(), true);
	item.mResults = Id("Results " + pool->Name());
	mPools.push_back(item);
}

void cProfiler::ClearPools()
{
	mPools.clear();
}

double cProfiler::CPUScale(unsigned long long now) const
{
	if (!nThread::cWorkerPool::CPUCycles()) // note: thread cpu is already nanoseconds
		return 1.;

	const unsigned long long cycles = nThread::cWorkerPool::CycleCounter();

	if ((cycles <= mCycleStart) || (now <= mCycleTime))
		return 0.;

	return ((double)(now - mCycleTime) / (double)(cycles - mCycleStart)); // note: nanoseconds per cycle, measured over whole time since profiler was enabled
}

void cProfiler::TakeThreads(bool keep, unsigned long long now)
{
	const unsigned long long cpu = nThread::cWorkerPool::ThreadCPU();
	const double scale = CPUScale(now);

	if (keep)
		Record(mMain, mSteps, 0, (unsigned long long)(((cpu > mMainCPU) ? (cpu - mMainCPU) : 0) * scale), 0);

	mMainCPU = cpu;
	mSteps = 0;
	mSections[mMain].mThreads = 1;

	for (const auto &item: mPools) {
		unsigned long long jobs = 0, time = 0, work = 0, max = 0;
		item.mPool->TakeStats(jobs, time, work, max);
		mSections[item.mThread].mThreads = (unsigned int)item.mPool->Threads();

		if (keep && jobs)
			Record(item.mThread, jobs, time, (unsigned long long)(work * scale), max);
	}
}

void cProfiler::Tick()
{
	if (!mEnabled) {
		if (mWasEnabled) {
			nThread::cWorkerPool::sMeasure = false;
			mWasEnabled = false;
		}

		return;
	}

	mSteps++;
	const unsigned long long now = SteadyNS();
	const long long second = (long long)(now / 1000000000ULL);

	if (second == mSecond)
		return;

	mSecond = second;
	const long long minute = (second / 60);

	if (minute != mMinute) {
		CloseMinute();
		mMinute = minute;
	}

	if (!mWasEnabled) { // note: work done while profiler was disabled is not counted
		nThread::cWorkerPool::sMeasure = true;
		mWasEnabled = true;
		mCycleStart = nThread::cWorkerPool::CycleCounter();
		mCycleTime = now;
		TakeThreads(false, now);
		return;
	}

	TakeThreads(true, now);
}

void cProfiler::Reset()
{
	for (auto &sec: mSections) {
		for (auto &part: sec.mMinute)
			part = sPart();

		for (auto &part: sec.mHour)
			part = sPart();

		sec.mTotal = sPart();
	}
}

cProfiler::sPart cProfiler::Sum(const sSection &sec, unsigned int period, long long minute) const
{
	sPart res;
	const sPart &open = sec.mMinute[mMinute % 60]; // note: part of minute that is not closed yet, it is not in hours and total

	if (!period) {
		res = sec.mTotal;

		if (open.mStamp == mMinute)
			res.Add(open);

		return res;
	}

	if (period <= 3600) {
		const long long from = (minute - (long long)((period + 59) / 60) + 1);

		for (const auto &part: sec.mMinute) {
			if ((part.mStamp >= from) && (part.mStamp <= minute))
				res.Add(part);
		}

		return res;
	}

	const long long hour = (minute / 60), from = (hour - (long long)((period + 3599) / 3600) + 1);

	for (const auto &part: sec.mHour) {
		if ((part.mStamp >= from) && (part.mStamp <= hour))
			res.Add(part);
	}

	if ((open.mStamp == mMinute) && ((mMinute / 60) >= from) && ((mMinute / 60) <= hour))
		res.Add(open);

	return res;
}

string cProfiler::Time(unsigned long long ns)
{
	char buf[32];

	if (ns >= 1000000000ULL)
		snprintf(buf, sizeof(buf), "%.2f s", (ns / 1000000000.));
	else if (ns >= 1000000ULL)
		snprintf(buf, sizeof(buf), "%.2f ms", (ns / 1000000.));
	else
		snprintf(buf, sizeof(buf), "%.0f us", (ns / 1000.));

	return buf;
}

string cProfiler::Share(unsigned long long part, unsigned long long all)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%.1f%%", (all ? ((part * 100.) / all) : 0.));
	return buf;
}

void cProfiler::Show(ostream &os, unsigned int period) const
{
	if (period > sMaxPeriod)
		period = sMaxPeriod;

	if (period)
		os << autosprintf(_("Profiler information for last %s"), cTimePrint((long)period).AsPeriod().AsString().c_str()) << ":\r\n\r\n";
	else
		os << _("Profiler information since hub start or reset") << ":\r\n\r\n";

	if (!mEnabled)
		os << " [*] " << _("Profiler is disabled, see adv_enable_profiler.") << "\r\n\r\n";

	const long long minute = SteadyMinute();
	vector<pair<const sSection*, sPart>> threads, sections;
	unsigned long long all_cpu = 0, all_time = 0;

	for (const auto &sec: mSections) {
		const sPart sum = Sum(sec, period, minute);

		if (!sum.mCount && !sum.mCPU)
			continue;

		if (sec.mThread) {
			threads.emplace_back(&sec, sum);
			all_cpu += sum.mCPU;
		} else {
			sections.emplace_back(&sec, sum);
			all_time += sum.mTime;
		}
	}

	if (threads.empty() && sections.empty()) {
		os << " [*] " << _("No data collected.");
		return;
	}

	sort(threads.begin(), threads.end(), [](const pair<const sSection*, sPart> &a, const pair<const sSection*, sPart> &b) { return (a.second.mCPU > b.second.mCPU); });
	sort(sections.begin(), sections.end(), [](const pair<const sSection*, sPart> &a, const pair<const sSection*, sPart> &b) { return (a.second.mTime > b.second.mTime); });

	if (threads.size()) {
		os << _("Threads") << ":\r\n\r\n";

		for (const auto &item: threads) {
			const sPart &sum = item.second;

			if (item.first == &mSections[mMain])
				os << " [*] " << autosprintf(_("%s: CPU %s [%s], main loop steps %s"), item.first->mName.c_str(), Time(sum.mCPU).c_str(), Share(sum.mCPU, all_cpu).c_str(), to_string(sum.mCount).c_str()) << "\r\n";
			else
				os << " [*] " << autosprintf(_("%s x %u: CPU %s [%s], jobs %s, busy %s, longest %s"), item.first->mName.c_str(), item.first->mThreads, Time(sum.mCPU).c_str(), Share(sum.mCPU, all_cpu).c_str(), to_string(sum.mCount).c_str(), Time(sum.mTime).c_str(), Time(sum.mMax).c_str()) << "\r\n";
		}
	}

	if (sections.size()) {
		if (threads.size())
			os << "\r\n";

		os << _("Main thread sections") << ":\r\n\r\n";

		for (const auto &item: sections) {
			const sPart &sum = item.second;
			os << " [*] " << autosprintf(_("%s: %s [%s], calls %s, average %s, longest %s"), item.first->mName.c_str(), Time(sum.mTime).c_str(), Share(sum.mTime, all_time).c_str(), to_string(sum.mCount).c_str(), Time(sum.mCount ? (sum.mTime / sum.mCount) : 0).c_str(), Time(sum.mMax).c_str()) << "\r\n";
		}
	}
}

	}; // namespace nUtils
}; // namespace nVerliHub
