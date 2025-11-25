/*
	Copyright (C) 2024-2025 FearDC, webmaster at feardc dot net

	This is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	It is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see https://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef FEARTLS_H
#define FEARTLS_H

struct VH_FearConf
{
	const char *FAddr;
	int FPort;
	const char *FHost;
	const char *FCert;
	const char *FKey;
	bool FLog;
	int FWait;
	int FVer;
	bool FSend;
};

extern "C"
{
	extern bool VH_FearStart(VH_FearConf *);
	extern void VH_FearStop(int);
}

#endif // FEARTLS_H
