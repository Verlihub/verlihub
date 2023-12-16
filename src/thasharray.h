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

#ifndef NUTILSTHASHARRAY_H
#define NUTILSTHASHARRAY_H

#include <string.h>
#include "cobj.h"

namespace nVerliHub {
	namespace nUtils {
		/// @addtogroup Core
		/// @{
		/**
		 * Non-persisten hash container.
		 *
		 * Please note that the dimension of the hash container is fixed and
		 * two elements may collide because they have the same hash. In
		 * this case the old element is removed and kept the last one.
		 *
		 * If you do not want this behaviour, please use tHashArray that
		 * deals with collision resolution.
		 * @author Daniel Muller
		 */
		template <class DataType> class tUniqueHashArray : public cObj
		{
			public:
				/**
				 * Class constructor
				 * @param initialSize Initial size of the hash container.
				 */
				tUniqueHashArray(unsigned initialSize=1024); // todo: why so big size? can we make it smaller?

				/**
				 * Class destructor.
				 */
				virtual ~tUniqueHashArray();

				/**
				 * Return the reserved capacity for the container.
				 * @return The size of the container.
				 */
				virtual unsigned Capacity() const;

				/**
				 * Check if an item exists with the given hash.
				 * @param hash The hash.
				 * @return True if the item exists or false otherwise.
				 */
				bool ContainsHash(const unsigned &hash)
				{
					return (DataType)NULL != Get(hash);
				}

				/**
				 * Return the data with the given hash.
				 * @param hash The hash.
				 * @return The data or 0 if no data can be found.
				 */
				virtual DataType Get(unsigned hash);

				/**
				 * Return the data with the given hash.
				 * @param hash The hash.
				 * @return The data or 0 if no data can be found.
				 * @see Get()
				 */
				DataType GetByHash(const unsigned &hash)
				{
					return Get(hash);
				}

				/**
				 * Insert a new data in the hash array with the given hash.
				 * @param data The data to add.
				 * @param hash The hash.
				 * @return 0 on success or the old data with the same hash.
				 */
				virtual DataType Insert(DataType const data, unsigned hash);

				/**
				 * Calculate the hash for the given key.
				 * @param key The key.
				 * @return The hash.
				 * @todo ShortHashString does the same thing!
				 */
				unsigned Key2ShortHash(const string &key)
				{
					return ShortHashString(key);
				}

				/**
				 * Remove data with the given hash.
				 * @param hash The hash.
				 * @return The data removed.
				 */
				virtual DataType Remove(unsigned hash);

				/**
				 * Calculate the hash for the given pointer.
				 * @param pointer The pointer.
				 * @return The hash of the pointer.
				 */
				unsigned ShortHashPointer(const void *pointer);

				/**
				 * Calculate the hash for the given string.
				 * @param string The string.
				 * @return The hash of the string.
				 */
				unsigned ShortHashString(const string &string);

				/**
				 * Return the number of elements in the container.
				 * @return The number of elements.
				 * @todo why virtual?
				 */
				virtual unsigned Size() const;

				/**
				 * Return the number of elements in the container.
				 * @return The number of elements.
				 */
				unsigned size() const
				{
					return mSize;
				}

				/**
				 * Update the data with the given hash.
				 *
				 * If no data exists with the given hash, a new item is added
				 * otherwise the old data are overwritten.
				 * @param data The data to add.
				 * @param hash The hash.
				 * @return 0 on success or the old data.
				 */
				virtual DataType Update(DataType const data, unsigned hash);

				/// Iterator to iterate throught the items of the container.
				class iterator
				{
				public:
					/**
					 * Class constructor.
					 */
					iterator() : currentElement(0), lastElement(0), mData(NULL)
					{}

					/**
					 * Class constructor.
					 * @param data Pointer to array data to iterate.
					 * @param _currentElement Current element of the data.
					 * @param _lastElement Last element of the data.
					 */
					iterator(DataType *data, unsigned _currentElement, unsigned _lastElement) : currentElement(_currentElement), lastElement(_lastElement), mData(data)
					{}

					/**
					 * Copy constructor.
					 * @param it Iterator instance.
					 */
					iterator(const iterator &it)
					{
						(*this) = it;
					}

					/**
					* Overload operator =.
					* @param it The value to assign.
					* @return A reference to the current instance.
					*/
					iterator &operator=(const iterator &it)
					{
						mData = it.mData;
						currentElement = it.currentElement;
						lastElement = it.lastElement;
						return *this;
					}

					/**
					* Overload operator ==.
					* @param it The value to check.
					* @return Return true if the two instance are equal or false otherwise.
					*/
					bool operator==(const iterator &it)
					{
						return currentElement == it.currentElement;
					}

					/**
					 * Overload operator !=.
					 * @param it The value to check.
					 * @return Return true if the two instance are different or false otherwise.
					 */
					bool operator!=(const iterator &it)
					{
						return currentElement != it.currentElement;
					}

					/**
					 * Overload operator ++.
					 * @return Iterator to the next element.
					 */
					iterator &operator++()
					{
						while((++currentElement != lastElement) && (mData[currentElement] == (DataType)NULL));
						return *this;
					}

					/**
					 * Overload operator *.
					 * @return Return the element pointed by the iterator.
					 */
					DataType operator*()
					{
						return mData[currentElement];
					}

					/**
					 * Test if it is at the end of array data.
					 * @return True if it is at the end or false otherwise.
					 */
					bool IsEnd()
					{
						return currentElement >= lastElement;
					}

					/// Current element pointed by the iterator.
					unsigned currentElement;

					/// Last element in the array data.
					unsigned lastElement;

					/// Array data to iterate.
					DataType *mData;
				};

				/**
				 * Return the iterator that points to the first element in the container.
				 */
				iterator begin()
				{
					iterator Begin(mData, 0,mCapacity);
					if(mData[0] == (DataType)NULL)
						++Begin;
					return Begin;
				}

				/**
				 * Return the iterator that points to the last element in the container.
				 */
				iterator end()
				{
					return iterator(mData, mCapacity , mCapacity);
				}

				/// Array pointer to data.
				DataType *mData;

				/// Number of elements in the container.
				unsigned mSize;

				/// Size of the container.
				unsigned mCapacity;
		};

		/**
		 * Hash table.
		 *
		 * This container is different from tUniqueHashArray because each slot
		 * of the bucket container is a pointer to a linked list that contains
		 * the key-value pairs that hashed to the same location.
		 * @author Daniel Muller
		 */
		template <class DataType> class tHashArray : public cObj
		{
			public:
				/// Define the type of the hash.
				typedef unsigned long tHashType;
				class iterator;
			private:

				/**
				 * Linked list of elements with same hash.
				 */
				struct sItem
				{
					friend class iterator;
					/// The data of the item.
					DataType mData;

					/// The hash of the item.
					tHashType mHash;

					public:
						/**
						 * Class constructor.
						 */
						sItem(DataType Data= (DataType)NULL, tHashType Hash = 0, sItem *Next = NULL) :
						mData(Data), mHash(Hash), mNext(Next) {}

						/**
						 * Class destructor.
						 * The data in the container are deleted by calling
						 * the delete operator.
						 */
						~sItem()
						{
							if (mNext != NULL) {
								delete mNext;
								mNext =  NULL;
							}
						}

						/**
						 * Return the number of elements in the list.
						 */
						unsigned Size()
						{
							unsigned i = 1;
							sItem *it = mNext;
							while(it != NULL) {
								it = it->mNext;
								i++;
							}
							return i;
						}

						/**
						 * Find an element with the given hash in the list.
						 * @param hash The hash.
						 * @return The found element or NULL.
						 */
						DataType Find(tHashType hash)
						{
							if(mHash == hash)
								return mData;
							sItem *it = mNext;
							while((it != NULL ) && (it->mHash != hash))
								it = it->mNext;
							if(it != NULL)
								return it->mData;
							else
								return (DataType)NULL;
						}

						/**
						 * Add a new element to the list.
						 * If an element with the same hash already exists,
						 * the element is overwritten.
						 * @param hash The hash.
						 * @param element The element to add.
						 * @return The added element if no element exists with the same hash
						 * or the old one.
						 */
						DataType Set(tHashType hash, const DataType &element)
						{
							DataType _result = 0;
							if(mHash == hash) {
								_result = mData;
								mData = element;
								return _result;
							}
							sItem *it = mNext;
							while((it != NULL ) && (it->mHash != hash))
								it = it->mNext;
							if(it != NULL) {
								_result = it->mData;
								it->mData = element;
								return _result;
							}
							else
								return _result;
						}

						/**
						 * Add a new element to the container.
						 * If an element with the same hash already exists,
						 * the element is NOT added.
						 *
						 * To overwrite existing elements use Set().
						 * @param element The element to add.
						 * @param hash The hash.
						 * @return The added element if no element exists with the same hash
						 * or the existing one.
						 */
						DataType AddData(DataType element, tHashType hash)
						{
							if(mHash == hash)
								return mData;
							sItem *it = mNext;
							sItem *prev = this;

							while((it != NULL ) && (it->mHash != hash)) {
								prev = it;
								it = it->mNext;
							}

							if(it != NULL) {
								return it->mData;
							}
							prev->mNext = new sItem(element , hash);
							return (DataType)NULL;
						}

						/**
						 * Delete an element with the given hash in the container.
						 * @param hash The hash.
						 * @param start
						 * @return The deleted element.
						 * @todo What is the meaning of start param?
						 */
						DataType DeleteHash(tHashType hash, sItem *&start)
						{
							DataType Data = (DataType)NULL;

							if (mHash == hash) {
								start = mNext;
								mNext = NULL;
								Data = mData;
								return Data;
							}

							sItem *it = mNext;
							sItem *prev = this;

							while ((it != NULL ) && (it->mHash != hash)) {
								prev = it;
								it = it->mNext;
							}

							if (it != NULL) {
								Data = it->mData;
								prev->mNext = it->mNext;
								it->mNext = NULL;
								delete it;
								it = NULL;
							}

							return Data;
						}

					private:
						/// Next item in the list.
						sItem *mNext;
				};
			public:
				/**
				 * Class constructor.
				 * @param initialSize Initial size of the hash array.
				 */
				tHashArray(unsigned initialSize=1024): cObj("tHashArray")
				{
					mData = new tData(initialSize);
					mSize = 0;
					mIsResizing = false;
				}

				/**
				 * Class destructor
				 */
				~tHashArray()
				{
					Clear();
					delete mData;
					mData = NULL;
				}

				/**
				 * Add an element with the given hash to the container.
				 * @param element The element to add.
				 * @param hash The hash.
				 * @return True if the element is added or false otherwise.
				 */
				bool AddWithHash(DataType element, const tHashType &hash)
				{
					if(element == (DataType) NULL)
						return false;
					unsigned HashShort = hash % mData->mCapacity;
					sItem * Items, *Item = NULL;
					Items = mData->Get(HashShort);
					if(Items == NULL) {
						Item = new sItem(element, hash);
						mData->Insert(Item, HashShort);
						if(!mIsResizing) {
							OnAdd(element);
							mSize++;
						}
						return true;
					}

					if((DataType)NULL == Items->AddData(element, hash)) {
						if(!mIsResizing) {
							OnAdd(element);
							mSize++;
						}
						return true;
					} else {
						return false;
					}
				}

				/**
				 * Autoresize the container.
				 * A complete resizing can be automatically triggered when the load factor exceeds some threshold.
				 */
				void AutoResize()
				{
					if((this->mSize > this->mData->mCapacity * 2 ) || ((this->mSize * 2 + 1) < this->mData->mCapacity)) {
						if(Log(3))
							LogStream() << "Autoresizing capacity: " << this->mData->mCapacity << " size: " << this->mSize << " >> " << (this->mSize + this->mSize/2 + 1) << endl;
						this->Resize(this->mSize + this->mSize/2 + 1);
					}
				}

				/**
				 * Clear the container.
				 */
				void Clear()
				{
					sItem *Item = NULL;

					for (unsigned it = 0; it < mData->Capacity(); it++) {
						Item = mData->Get(it);
						mData->Update(NULL, it);

						if (Item != NULL) {
							delete Item;
							Item = NULL;
						}
					}
				}

				/**
				 * Check if an element with the given hash exists
				 * in the container.
				 * @param hash The hash.
				 * @return True if the element exists or false otherwise.
				 */
				bool ContainsHash(const tHashType &hash)
				{
					unsigned HashShort = hash % mData->mCapacity ;
					sItem * Items = mData->Get(HashShort);
					if(Items == NULL)
						return false;
					return ((DataType)NULL != Items->Find(hash));
				}

				/**
				 * Dumpe the content of the container to
				 * the given stream.
				 * @param os The stream where to store the result.
				 */
				void DumpProfile(ostream &os)
				{
					os << "Size = " << mSize << " Capacity = " << mData->mCapacity << endl;
					unsigned cumulative = 0;
					for(unsigned i=0;i < mData->mCapacity; ++i) {
						if(mData->mData[i] != NULL) {
							os << "Bucket #" << i << " [Cumulative NULL elements = " << cumulative << " Total = " << ((sItem*)mData->mData[i])->Size() << ']' << endl;
							cumulative = 0;
						} else {
							cumulative++;
						}
					}
				}

				/**
				 * Get the element with the given hash from the container.
				 * @param hash The hash.
				 * @return The element or NULL if the element does not exists.
				 */
				DataType GetByHash(const tHashType &hash)
				{
					unsigned HashShort = hash % mData->mCapacity ;
					sItem * Items = mData->Get(HashShort);
					if (Items == NULL)
						return (DataType)NULL;
					return Items->Find(hash);
				}

				/**
				 * Calculate the hash for the given string
				 * but convert it to lower case.
				 * @param string The string.
				 * @return The hash of the string.
				 */
				static tHashType HashLowerString(const string &string);

				/**
				* Calculate the hash for the given string.
				* @param string The string.
				* @return The hash of the string.
				*/
				static tHashType HashString(const string &string);

				/**
				 * Event handler function called when an element is added.
				 * @param element The added element.
				 */
				virtual void OnAdd(DataType element)
				{};

				/**
				 * Event handler function called when an element is removed.
				 * @param element The removed element.
				 */
				virtual void OnRemove(DataType element)
				{};

				/**
				 * Calculate the hash for the given key.
				 * @param key The key.
				 * @return The hash of the key.
				 * @see HashString()
				 */
				tHashType Key2Hash(const string &key)
				{
					return HashString(key);
				}

				/*
				tHashType Key2HashLower(const string &key)
				{
					return HashLowerString(key);
				}
				*/

				/**
				* Remove an element with the given hash from the container.
				* @param hash The hash.
				* @return True if the element is removed or false otherwise.
				*/
				bool RemoveByHash(const tHashType &hash)
				{
					unsigned HashShort = hash % mData->mCapacity;
					sItem *Items, *Item = NULL;
					Items = mData->Get(HashShort);

					if (Items == NULL)
						return false;

					Item = Items;
					DataType Data = Items->DeleteHash(hash, Item);

					if (Item != Items) {
						mData->Update(Item, HashShort);
						delete Items;
						Items = NULL;
					}

					if ((DataType)NULL != Data) {
						OnRemove(Data);
						mSize--;
						return true;
					}

					return false;
				}

				/*
					resize the container to the given size
					newSize - the new size of the container
					return - always 0
				*/
				int Resize(int newSize)
				{
					tData *NewData = new tData(newSize);
					tData *OldData = this->mData;
					iterator it = this->begin();
					mIsResizing = true;
					this->mData = NewData;

					while (!it.IsEnd()) { // copy and recalculate all old data
						this->AddWithHash(it.mItem->mData, it.mItem->mHash);
						++it;
					}

					sItem *Item = NULL; // delete old container and all its items

					for (unsigned it = 0; it < OldData->Capacity(); it++) {
						Item = OldData->Get(it);

						if (Item) {
							delete Item;
							Item = NULL;
						}
					}

					delete OldData;
					OldData = NULL;
					mIsResizing = false;
					return 0;
				}

				/**
				 * @todo Document me!
				 */
				bool SetByHash(const tHashType &hash, const DataType &value)
				{
					unsigned HashShort = hash % mData->mCapacity ;
					sItem * Items = mData->Get(HashShort);
					if(Items == NULL)
						return false;
					return Items->Set(hash, value);
				}

				/**
				 * Return the number of elements in the container.
				 * @return The number of elements.
				 */
				unsigned Size() const
				{
					return mSize;
				}

				unsigned Capacity() const
				{
					return mData->mCapacity;
				}

				/// Define a cointer of pointer of sItem instance.
				typedef tUniqueHashArray<sItem *> tData;


				/**
				 * Iterator class to iterate throught the items of the container.
				 */
				class iterator
				{
					public:
						/**
						 * Class constructor.
						 */
						iterator() : mItem(NULL)
						{}

						/**
						 * Class constructor.
						 * @param it Iterate starting from the given element.
						 */
						iterator(typename tData::iterator it) : i(it)
						{
							if(!i.IsEnd())
								mItem = *i;
							else
								mItem = NULL;
						}

						/**
						* Copy constructor.
						* @param it Iterator instance.
						*/
						iterator(const iterator &it)
						{
							 (*this) = it;
						}

						/**
						 * Test if the iterator is at the end of the container.
						 * @return True if it is at the end or false otherwise.
						 */
						bool IsEnd()
						{
							return mItem == NULL;
						}

						/**
						 * Overload operator =.
						 * @param it The value to assign.
						 * @return A reference to the current instance.
						 */
						iterator &operator=(const iterator &it)
						{
							mItem = it.mItem;
							i = it.i;
							return *this;
						}

						/**
						* Overload operator ==.
						* @param it The value to check.
						* @return Return true if the two instance are equal or false otherwise.
						*/
						bool operator==(const iterator &it)
						{
							return mItem == it.mItem;
						}

						/**
						 * Overload operator !=.
						 * @param it The value to check.
						 * @return Return true if the two instance are different or false otherwise.
						 */
						bool operator!=(const iterator &it)
						{
							return mItem != it.mItem;
						}

						/**
						* Overload operator ++.
						* @return Iterator to the next element.
						*/
						iterator &operator++()
						{
							if(mItem->mNext != NULL) {
								mItem = mItem->mNext;
							} else {
								++i;
								if(!i.IsEnd())
									mItem = (*i);
								else
									mItem = NULL;
							}
							return *this;
						}

						/**
						* Overload operator *.
						* @return Return the element pointed by the iterator.
						*/
						DataType operator*()
						{
							return mItem->mData;
						}

						/// Iterator element.
						typename tData::iterator i;

						/// Element pointed by the iterator.
						sItem * mItem;
				};

				/**
				 * Return the iterator that points to the first element in the container.
				 */
				iterator begin()
				{
					return iterator(mData->begin());
				}
				/**
				 * Return the iterator that points to the last element in the container.
				 */
				iterator end()
				{
					return iterator();
				}
			protected:
				/// Pointer to container instance.
				tData * mData;

				/// Container size.
				unsigned mSize;

				/// True if the container is going to
				/// be resized.
				bool mIsResizing;
		};
		/// @}

template <class DataType> tUniqueHashArray<DataType>::tUniqueHashArray(unsigned initialSize) : mCapacity(initialSize)
{
	mSize = 0;
	mData = new DataType[mCapacity];
	memset(mData, 0, sizeof(DataType)*mCapacity);
}

template <class DataType> tUniqueHashArray<DataType>::~tUniqueHashArray()
{
	if (mData)
		delete [] mData;

	mData = NULL;
}

template <class DataType> DataType tUniqueHashArray<DataType>::Insert(DataType const Data, unsigned hash)
{
	if (hash > mCapacity) hash %= mCapacity;
	DataType OldData = mData[hash];
	if (!OldData)
	{
		mData[hash] = Data;
		if( Data ) mSize ++;
	}
	return OldData;
}

template <class DataType> DataType tUniqueHashArray<DataType>::Update(DataType const Data, unsigned hash)
{
	if (hash > mCapacity)
		hash %= mCapacity;

	DataType OldData = mData[hash];
	mData[hash] = Data;

	if (!OldData && Data)
		mSize++;
	else if (OldData && !Data)
		mSize--;

	return OldData;
}

template <class DataType> DataType tUniqueHashArray<DataType>::Get(unsigned hash)
{
	if(hash > mCapacity)
		hash %= mCapacity;
	return mData[hash];
}

template <class DataType> DataType tUniqueHashArray<DataType>::Remove(unsigned hash)
{
	if(hash > mCapacity)
		hash %= mCapacity;
	DataType OldData = mData[hash];
	mData[hash] = 0;
	if(OldData)
		mSize --;
	return OldData;
}

template <class DataType> unsigned tUniqueHashArray<DataType>::ShortHashString(const string &str)
{
	unsigned __h = 0;
	const char *__s = str.c_str();
	for ( ; *__s; ++__s)
	__h = 33*__h + *__s;

	return __h % mCapacity;
}

template <class DataType> typename tHashArray<DataType>::tHashType tHashArray<DataType>::HashString(const string &str)
{
	tHashType __h = 0;
	const char *__s = str.c_str();
	for ( ; *__s; ++__s)
	__h = 33*__h + *__s;

	return __h;
}

template <class DataType> typename tHashArray<DataType>::tHashType tHashArray<DataType>::HashLowerString(const string &str)
{
	tHashType __h = 0;
	const char *__s = str.c_str();
	for ( ; *__s; ++__s)
	__h = 33*__h + ::tolower(*__s);

	return __h;
}

template <class DataType> unsigned tUniqueHashArray<DataType>::ShortHashPointer(const void *)
{
	return 0;
}

template <class DataType> unsigned tUniqueHashArray<DataType>::Size() const
{
	return mSize;
}

template <class DataType> unsigned tUniqueHashArray<DataType>::Capacity() const
{
	return mCapacity;
}
	}; // namespace nUtils
}; // namespace nVerliHub
#endif
