/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net

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

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef NPLUGINCCALLBACKLIST_H
#define NPLUGINCCALLBACKLIST_H
#include <list>
#include <algorithm>
#include <functional>
#include <string>
#include <iostream>

using namespace std;

namespace nVerliHub {
	namespace nPlugin {
		class cPluginBase;
		class cPluginManager;
		/// @addtogroup Plugin
		/// @{
		/**
		 * This class represents a callback function and
		 * it keeps a list of plugin on which the callback should be called.
		 *
		 * A callback is is uniquely identified with a name.
		 *
		 * Callbacks are handled by the VerliHub Plugin Manager.
		 *
		 * A plugin must inherits from cPluginBase class and can registered
		 * itself with Register() method. This is usually done when the plugin
		 * is loaded.
		 * Plugin can also unregistered at any time with Unregister() method.
		 * @author Daniel Muller
		 * @todo Documentation not done!
		 * @todo Rename to cCallBack
		 */
		class cCallBackList
		{
			public:
				/// Define a list of plugins
				typedef list<cPluginBase*> tPICont;

				/**
				 * Class constructor.
				 * @param mgr Pointer to plugin manager
				 * @param id The identifier of the list.
				 */
				cCallBackList(cPluginManager *mgr, const string &id);

				/**
				 * Class destructor.
				 */
				virtual ~cCallBackList();

				/**
				 * Call the callback for each registerd plugin.
				 * @return The result of the calls.
				 */
				virtual bool CallAll();

				/**
				 * Call the callback for the given plugin.
				 * @param plugin The plugin.
				 * * @return The result of the call.
				 */
				virtual bool CallOne(cPluginBase *plugin) = 0;

				/**
				 * Store in the given stream the list of registered plugins.
				 * @param os The stream.
				 * @param indent The identation string to use for each element in the list.
				 */
				virtual void ListRegs(ostream &os, const string &sep);

				/**
				 * Return the identifier of the callback list.
				 * @return The name of the list.
				 */
				virtual const string &Name() const;

				/**
				 * Register a plugin so callback can be invoked.
				 * @param plugin The plugin to register.
				 * @return False if the plugin is not valid or it cannot be registered
				 * or true otherwise.
				 */
				bool Register(cPluginBase *plugin);

				/**
				 * Unregister a plugin.
				 * @param plugin The plugin to unregister.
				 * @return False if the plugin is not valid or it is not registered
				 * or true otherwise.
				 */
				bool Unregister(cPluginBase *plugin);

				/**
				 * This structure represents a callback function that is called by
				 * CallOne() and CallAll() methods.
				 */
				//struct ufCallOne: public unary_function<void, tPICont::iterator>
				struct ufCallOne
				{
					/// Pointer to plugin manager instance.
					cPluginManager *mMgr;
					/// Pointer to callbacklist instance.
					cCallBackList *mCBL;
					bool mCall;

					ufCallOne(cPluginManager *mgr, cCallBackList *cbl):
						mMgr(mgr),
						mCBL(cbl),
						mCall(true)
					{}

					void operator()(cPluginBase *pi);
				};
			private:
				/// List of registered plugins.
				tPICont mPlugList;

				/// Pointer to plugin manager instance.
				cPluginManager *mMgr;

				ufCallOne mCallOne;

				/// Identifier of the list.
				string mName;
		};
		/// @}
	}; // namespace nPlugin
}; // namespace nVerliHub

#endif
