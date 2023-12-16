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

#ifndef NCONFIGTCACHE_H
#define NCONFIGTCACHE_H
#include "thasharray.h"
#include "cconfmysql.h"
#include "ctime.h"

namespace nVerliHub {
	namespace nConfig {
		using namespace nUtils;

		/// @addtogroup Core
		/// @{
		/**
		 * Provide cache support for MySQL table content.
		 * Data of the table are loaded in memory and kept syncronized
		 * to improve performance.
		 * @author Daniel Muller
		 */
		template <class IndexType> class tCache : public cConfMySQL
		{
			public:
				/// Define an hash container of void pointers.
				typedef tHashArray<void*> tHashTab;

				/// Define the hash type.
				typedef tHashTab::tHashType tCacheHash;

				/**
				 * Class constructor.
				 * @param mysql cMySQL instance.
				 * @param tableName The name of the table.
				 * @param indexName The index of the table.
				 * @param dataName The column name used to update data.
				 */
				tCache(nMySQL::cMySQL &mysql, const char* tableName, const char* indexName, const char* dataName = NULL) :
					cConfMySQL(mysql), mDateName(dataName)
				{
					SetClassName("tCache");
					mMySQLTable.mName = tableName;
					cConfMySQL::Add(indexName,mCurIdx);
					SetBaseTo(this);
					mIsLoaded = false;
				}

				/**
				 * Class destructor.
				 */
				~tCache()
				{
					Clear();
				}

				/**
				 * Add new data to the cache.
				 * @param key The key.
				 */
				void Add(IndexType const &key)
				{
					tCacheHash hash = CalcHash(key);
					mHashTab.AddWithHash(this,hash);
				}

				/**
				* Calculate the hash for the given key.
				* @param key They key.
				* @return The hash of the key.
				*/
				tCacheHash CalcHash(string const &key)
				{
					return mHashTab.HashLowerString(key);
				}

				/**
				 * Clear the cache by removing all items.
				 */
				void Clear()
				{
					mHashTab.Clear();
					mIsLoaded = false;
				}

				/**
				 * Find the data in the cache with the given
				 * key.
				 * @param key The key.
				 * @return True if the data exists or false otherwise.
				 */
				bool Find(IndexType const &key)
				{
					tCacheHash hash = CalcHash(key);
					return mHashTab.ContainsHash(hash);
				}

				/**
				 * Return the time in seconds of last update operation.
				 * @return The last update.
				 */
				unsigned GetLastUpdate() const
				{
					return mLastUpdate.Sec();
				}

				/**
				* Return the time in seconds of last sync operation.
				* @return The last sync.
				*/
				unsigned GetLastSync()
				{
					return mLastSync.Sec();
				}

				/**
				 * Check if data has been loaded from the table.
				 * @return True if data are loaded or false otherwise.
				 */
				bool IsLoaded()
				{
					mIsLoaded = (mHashTab.Size() > 0);
					return mIsLoaded;
				}

				/**
				 * Load data from table to cache.
				 * @return The number of loaded items.
				 */
				int LoadAll()
				{
					SelectFields(mQuery.OStream());
					db_iterator it;
					for(it = db_begin(); it != db_end(); ++it)
						Add(mCurIdx);
					mQuery.Clear();

					if(Log(0))
						LogStream() << mHashTab.Size() << " items preloaded." << endl;
					mIsLoaded = (mHashTab.Size() > 0);
					mLastUpdate.Get();
					return mHashTab.Size();
				}

				/**
				 * Update the syn time.
				 */
				void Sync()
				{
					mLastSync.Get();
				}

				/**
				 * Update the cache.
				 *
				 * Please note that just new data since last
				 * update opeartion are fetched from database.
				 * The old one are not updated.
				 * @return The number of new items.
				 */
				int Update()
				{
					int n = 0;

					SelectFields(mQuery.OStream());
					if(mDateName)
						mQuery.OStream() << " where " << mDateName << " > " << mLastUpdate.Sec();
					for(db_iterator it = db_begin(); it != db_end(); ++it) {
						if(!Find(mCurIdx))
							Add(mCurIdx);
						n++;
					}
					if(n && Log(0))
						LogStream() << "Updated cache for table " << mMySQLTable.mName << " [Total items: " << mHashTab.Size() << " | New items: " << n << ']' << endl;
					mQuery.Clear();
					mLastUpdate.Get();
					return n;
				}
			protected:
				/// Cache container.
				tHashTab mHashTab;

				/// True if data are loaded.
				bool mIsLoaded;

				/// Time of last update operation.
				/// @see Update()
				cTime mLastUpdate;

				/// Time of last sync operation.
				/// @see Sync()
				cTime mLastSync;

				/// The column name that contains
				/// a time value used to update data
				/// and keep them sync with cache.
				const char *mDateName;
			private:
				/// The index of the table.
				IndexType mCurIdx;
		};
		/// @}
	}; // namespace nConfig
}; // namespace nVerliHub

#endif
