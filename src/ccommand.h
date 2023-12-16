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

#ifndef NCMDRCCOMMAND_H
#define NCMDRCCOMMAND_H
#include "cpcre.h"
#include "stringutils.h"
#include <string>
#include <ostream>
#include <iostream>

using std::string;
using std::ostream;
using std::cout;

namespace nVerliHub {
	namespace nCmdr {
		class cCommandCollection;
		/// @addtogroup Command
		/// @{
		/**
		 * Base command pattern class.
		 * A command is defined with a regular expression
		 * pattern and the pattern can define different command
		 * alias.
		 *
		 * For example suppose to show a list of last 10 banned users and
		 * you would use two command: !banlist 10 and !banls 10.
		 * The regular expression is something like this: !ban(list|ls).
		 *
		 * A command is described with:
		 * - a unique integer ID
		 * - a regular expression to identify the command
		 * - a regular expression to parse the command options
		 * - an instance of sCmdFunc class
		 *
		 * For the example above the interger ID may be 1,
		 * the string ID is 'ban' and the regular expression
		 * for the command options is '[0-9]+'.
		 *
		 * These are the steps usually done to execute the command:
		 * -# parse the command line with ParseCommandLine() call
		 * -# check if the options of the command are valid with
		 * TestParams() call
		 * -# execute the command with Execute() call
		 *
		 * @author Daniel Muller
		 */
		class cCommand
		{
			friend class cCommandCollection;
			public:
				/**
				* Command class function to parse the command options and run the command.
				* It is possibile to check value of a single command option by its type,
				* for example if it is a string, an integer or a boolean value.
				*
				* A value for a single option is indexed by matched offset in the regular expression.
				*
				* For example suppose to have the following regluar expression to
				* parse two options: "( -lc ?(-?[0-9.]+))? ( -LC ?(-?[0-9.]+))?".
				*
				* The index for the value of -lc option is 2 and the the value for
				* -LC is 4.
				* @author Daniel Muller
				*/
				class sCmdFunc
				{
					public:
						/**
						* Class constructor.
						*/
						sCmdFunc():mIdRex(NULL), mParRex(NULL), mOS(NULL), mCommand(NULL), mExtra(NULL)
						{}

						/**
						* Class destructor.
						*/
						virtual ~sCmdFunc()
						{};

						/**
						* Execute the command.
						* @return The result of execution.
						*/
						virtual bool operator() (void) = 0;

						/**
						 * Execute the command.
						 * The command execution is delegated to void operator()
						 * that must be implemented in the subclass.
						 * @param regexId The regular expression to parse command ID.
						 * @param paramRegex The regular expression to parse options of the command.
						 * @param os The stream where to store command result.
						 * @param data Extra data to the command.
						 * @return The result of the excution.
						 */
						virtual bool operator() (nUtils::cPCRE &idRegex, nUtils::cPCRE &paramRegex, ostream &os, void *data)
						{
							mIdRex = &idRegex;
							mParRex = &paramRegex;
							mExtra = data;
							mOS = &os;
							return operator()();
						}

						/**
						 * Return the value of the given option as boolean.
						 * Boolean values are "1", "on", "true" or "yes".
						 * @param index The index of the option.
						 * @param dest The result.
						 * @return True if the option is found or false otherwise.
						 */
						virtual bool GetParBool(int index, bool &dest)
						{
							string tmp;
							if(!GetParStr(index, tmp))
								return false;
							dest = (tmp == "1") || (tmp == "on") || (tmp == "true") || (tmp == "yes");
							return true;
						}

						/**
						 * Return the value of the given option as double.
						 * @param index The index of the option.
						 * @param dest The result.
						 * @return True if the option is found or false otherwise.
						 */
						virtual bool GetParDouble(int index, double &dest)
						{
							string tmp;
							if(!GetParStr(index, tmp))
								return false;
							dest = atof(tmp.c_str());
							return true;
						}

						/**
						* Return the value of the given option as integer.
						* @param index The index of the option.
						* @param dest The result.
						* @return True if the option is found or false otherwise.
						*/
						virtual bool GetParInt(int index, int &dest)
						{
							string tmp;

							if (!GetParStr(index, tmp) || !nUtils::IsNumber(tmp.c_str()))
								return false;

							dest = atoi(tmp.c_str());
							return true;
						}

						/**
						* Return the value of the given option as long integer.
						* @param index The index of the option.
						* @param dest The result.
						* @return True if the option is found or false otherwise.
						*/
						virtual bool GetParLong(int index, long &dest)
						{
							string tmp;

							if (!GetParStr(index, tmp) || !nUtils::IsNumber(tmp.c_str()))
								return false;

							dest = atoi(tmp.c_str());
							return true;
						}

						/**
						 * Return the value of the given option as string.
						 * @param index The index of the option.
						 * @param dest The result.
						 * @return True if the option is found or false otherwise.
						 */
						virtual bool GetParStr(int index, string &dest)
						{
							if(!this->mParRex->PartFound(index))
								return false;
							this->mParRex->Extract(index, this->mParStr, dest);
							return true;
						}

						/**
						* Return the syntax for using the command.
						* @param os The stream where to store the result.
						* @see sCmdFunc::GetSyntaxHelp()
						*/
						virtual void GetSyntaxHelp(ostream &os, cCommand *){};

						/**
						 * Return the ID used for the command.
						 * @param index The matched offset in mIdRex regular expression.
						 * @param dest The result.
						 * @return True if the ID is found or false otherwise.
						 */
						virtual bool GetIDStr(int index, string &dest)
						{
							if(!this->mIdRex->PartFound(index))
								return false;
							this->mIdRex->Extract(index, this->mIdStr, dest);
							return true;
						}

						/**
						 * Check if the option can be found in the command options.
						 * @param index The index of the option.
						 * @return True if the option is found or false otherwise.
						 */
						virtual bool PartFound(int index)
						{
							return this->mParRex->PartFound(index);
						}

						/**
						 * @todo Document me!
						 */
						int StringToIntFromList(const string &str,const char *stringlist[], const int intlist[], int item_count);

						/// The extracted ID from command line.
						string mIdStr;

						/// The extracted options from command line.
						string mParStr;

						/// cPCRE instance to match the command by its
						/// id from command line.
						/// @see mIdStr
						nUtils::cPCRE * mIdRex;

						/// cPCRE instance to parse the options of the command
						/// from command line.
						/// @see mParStr
						nUtils::cPCRE * mParRex;

						/// Stream pointer where to store output execution.
						ostream *mOS;

						/// Pointer to cCommand instance
						cCommand *mCommand;

						/// Extra data for the command.
						void *mExtra;
				};

			/**
			 * Class constructor.
			* @param id The integer ID.
			* @param regexId The regular expression to parse command ID.
			* @param paramRegex The regular expression to parse options of the command.
			* @param function An instance of sCmdFunc class.
			* @see sCmdFunc
			*/
			cCommand(int id, const char *idRegex, const char *paramRegex, sCmdFunc *function);

			/**
			 * Class constructor.
			 */
			cCommand();

			/**
			 * Class destructor.
			 */
			virtual ~cCommand();

			/**
			 * Execute the command with the given options.
			 * This method does not really execute the command but it
			 * delegates to sCmdFunc::operator().
			 * @param output A stream where to store command output.
			 * @param options The options of the command.
			 * @return True if the command can be executed or false otherwise.
			 */
			bool Execute(ostream &output, void *options);

			/**
			 * Return the ID of the command
			 * @return The ID of the command.
			 */
			int GetID() const
			{
				return mID;
			}

			/**
			* Return the syntax for using the command.
			* @param os The stream where to store the result.
			* @see sCmdFunc::GetSyntaxHelp()
			*/
			void GetSyntaxHelp(ostream &os);

			/**
			 * Initialize the command.
			 * @param id The integer ID.
			 * @param regexId The string ID in the regular expression.
			 * @param paramRegex The regular expression for options of the command.
			 * @param function An instance of sCmdFunc class.
			 * @see sCmdFunc
			 */
			virtual void Init(int id, const char *regexId, const char *paramRegex, sCmdFunc *function);

			/**
			 * Initialize the command with the given data.
			 * @param data The data to initialize the command.
			 */
			virtual void Init(void *data)
			{};

			/**
			 * Describe the command.
			 * @param os The stream where to store output.
			 */
			virtual void Describe(ostream &os);

			/**
			 * Extract the command ID and command options from the
			 * given command line.
			 * Command ID is stored in mIdStr and the options is mParStr.
			 * @param commandLine The command line.
			 * @return True if the extract operation is performed or false otherwise.
			 */
			bool ParseCommandLine(const string &commandLine);

			/**
			 * Check if the command options parsed from command line with
			 * ParseCommandLine() call are valid.
			 * @return True if the options are valid or false otherwise.
			 * @see ParseCommandLine()
			 */
			bool TestParams();

			/// Pointer to command collection that store this command.
			cCommandCollection *mCmdr;
		protected:

			/// Integer identifier.
			int mID;

			/// cPCRE instance to match the command by its
			/// id from command line.
			/// @see mIdStr
			nUtils::cPCRE mIdentificator;

			/// cPCRE instance to parse the options of the command
			/// from command line.
			/// @see mParStr
			nUtils::cPCRE mParamsParser;

			/// The command function.
			sCmdFunc *mCmdFunc;

			/// The extracted ID from command line.
			string mIdStr;

			/// The extracted options from command line.
			/// This options will be parsed by sCmdFunc class
			/// with class methods.
			string mParStr;

			/// The regular expression to identify the command.
			string mIdRegexStr;

			/// The regular expression to parse the options of the command.
			string mParRegexStr;
		};
		/// @}
	}; // namespace nCmdr
}; // namespace nVerliHub
#endif
