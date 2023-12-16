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

#ifndef NUTILSCPCRE_H
#define NUTILSCPCRE_H
#include <pcre.h>
#include <string>

using std::string;

namespace nVerliHub {
	namespace nUtils {
		/// @addtogroup Command
		/// @{
		/**
		 * Wrapper class for pcre library.
		 * @author Daniel Muller
		 */
		class cPCRE
		{
			public:
				/**
				* Class constructor.
				* @param offsetResultSize Size of array of ints that contains result offsets.
				*/
				cPCRE(int offsetResultSize=30);

				/**
				* Class constructor.
				* @param pattern The regular expression to compile.
				* @param options Option bit for pcre_compile.
				* @param offsetResultSize Size of array of ints that contains result offsets.
				*/
				cPCRE(const char *pattern, unsigned int options, int offsetResultSize=30);

				/**
				* Class constructor.
				* @param pattern The regular expression to compile.
				* @param options Option bit for pcre_compile.
				* @param offsetResultSize Size of array of ints that contains result offsets.
				*/
				cPCRE(const string&pattern, unsigned int options, int offsetResultSize=30);

				/**
				* Class destructor.
				*/
				~cPCRE();

				/**
				 * Compare the given string with the captured substring at given offset.
				 *
				 * in a compiled pattern.
				 * @param name Named substring.
				 * @param text
				 * @param text2
				 * @return 0 if the compared characters sequences are equal, otherwise a
				 * number different from 0 is returned, with its sign indicating whether the
				 * string is considered greater than the comparing string passed as
				 * parameter (positive sign), or smaller (negative sign).
				 */
				int Compare(const string &name, const string &text, const string &text2);

				/**
				 * Compare the given string with the captured substring at given offset.
				 *
				 * @param index The offset.
				 * @param text
				 * @param text2
				 * @return 0 if the compared characters sequences are equal, otherwise a
				 * number different from 0 is returned, with its sign indicating whether the
				 * string is considered greater than the comparing string passed as
				 * parameter (positive sign), or smaller (negative sign).
				 */

				int Compare(int index, const string &text, const string &text2);
				/**
				 * Compare the given string with the captured substring at given offset.
				 *
				 * @param index The offset.
				 * @param text
				 * @param text2
				 * @return 0 if the compared characters sequences are equal, otherwise a
				 * number different from 0 is returned, with its sign indicating whether the
				 * string is considered greater than the comparing string passed as
				 * parameter (positive sign), or smaller (negative sign).
				 */
				int Compare(int index, const string &text, const char *text2);

				/**
				* Compile the given regular expression.
				* @param pattern The regular expression to compile.
				* @param options Option bit for pcre_compile.
				* @return True if the regular expression is compiled or false otherwise.
				*/
				bool Compile(const char *pattern, unsigned int options = 0);

				/**
				* Match a compiled regular expression against a given subject
				* string, using a matching algorithm that is similar to Perl's.
				* @param subject The subject string.
				* @return Number of captured substrings.
				*/
				int Exec(const string &subject);

				void Extract( int index, const string &src, string &dst);

				/**
				 * Finds the number of a named substring capturing parenthesis
				 * in a compiled pattern.
				 * @param name Named substring.
				 * @return The number of named substirng.
				 */
				int GeStringNumber(const string &substring);

				bool PartFound(int index);

				int StartOf(int index)
				{
					if(index < 0 || index >= mResult)
						return -1;
					return mOffsetResults[index << 1];
				}

				void Replace(int index, string &subject, const string &replace);


			private:
				/// Compiled regular expression.
				pcre *mPattern;

				/// Number of captured substring of
				/// last call of Exec().
				/// @see Exec()
				int mResult;

				/// Pointer to an array of ints for result offsets.
				int *mOffsetResults;

				/// Number of elements in the array
				int mOffsetResultSize;
			private:
				void Clear();
		};
		/// @}
	}; // namespace nUtils
}; // namespace nVerliHub
#endif
