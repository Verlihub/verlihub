/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCPLUGINHOST_H
#define CADCPLUGINHOST_H

#include "casyncsocketserver.h"

namespace nVerliHub {
	namespace nPlugin {
		class cPluginManager;

/**
 * Minimal bridge exposed by an ADC server to ADC connection objects.
 *
 * Keeping this interface independent from cServerDC prevents cConnADC from
 * acquiring a dependency on the legacy NMDC server just to dispatch native
 * ADC plugin lifecycle events.
 */
class cADCPluginHost
{
	public:
		virtual ~cADCPluginHost() {}
		virtual cPluginManager *ADCPluginManager() = 0;
};

	}; // namespace nPlugin
}; // namespace nVerliHub

#endif
