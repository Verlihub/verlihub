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

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef _I18N_
#define _I18N_

#ifndef USE_CUSTOM_AUTOSPRINTF
#include <autosprintf.h>
using gnu::autosprintf;

#else
#include <string>
std::string my_autosprintf(const char *format, ...);
#define autosprintf my_autosprintf
#endif

#include <libintl.h>
#include <locale.h>
#include <cstring>
#include <string>
#include <vector>

namespace nVerliHub {
	namespace nI18n {

/*
	translations come from translators that nobody reviews, wrong format in translation, for example %s instead of %d, crashes the hub in autosprintf
	so translation is used only when its format arguments are same as in original text, else original text is used
*/

inline bool FormatArgs(const char *text, std::vector<std::string> &args) // note: printf argument types in order, positional %2$s are put on their place, flags, width and precision are skipped
{
	args.clear();
	size_t next = 0;
	const char *pos = text;

	while ((pos = strchr(pos, '%')) != NULL) {
		pos++;

		if (*pos == '%') {
			pos++;
			continue;
		}

		const char *num = pos;
		size_t index = 0;

		while ((*num >= '0') && (*num <= '9'))
			index = (index * 10) + size_t(*num++ - '0');

		const bool place = ((num > pos) && (*num == '$') && index);

		if (place)
			pos = num + 1;
		else
			index = ++next;

		while (*pos && strchr("-+ #0'", *pos))
			pos++;

		while (((*pos >= '0') && (*pos <= '9')) || (*pos == '.') || (*pos == '*')) {
			if (*pos == '*') { // note: width or precision from argument, it comes before value
				if (place)
					return false;

				if (args.size() < index)
					args.resize(index);

				args[index - 1] = "*";
				index = ++next;
			}

			pos++;
		}

		std::string type;

		while (*pos && strchr("hlLqjzt", *pos))
			type += *pos++;

		switch (*pos) {
			case 'd': case 'i':
				type += 'd';
				break;
			case 'o': case 'u': case 'x': case 'X':
				type += 'u';
				break;
			case 'e': case 'E': case 'f': case 'F': case 'g': case 'G': case 'a': case 'A':
				type += 'f';
				break;
			case 's': case 'c': case 'p': case 'n':
				type += *pos;
				break;
			default: // note: unknown conversion or end of text
				return false;
		}

		pos++;

		if (args.size() < index)
			args.resize(index);

		args[index - 1] = type;
	}

	for (const std::string &arg: args) {
		if (arg.empty()) // note: missing position
			return false;
	}

	return true;
}

inline bool FormatSafe(const char *orig, const char *tran) // note: translation can leave out arguments from end, but not change their types or order
{
	if (!strchr(orig, '%') && !strchr(tran, '%'))
		return true;

	std::vector<std::string> list_orig, list_tran;

	if (!FormatArgs(orig, list_orig)) // note: original is not a format, translation is not used with arguments either
		return true;

	if (!FormatArgs(tran, list_tran) || (list_tran.size() > list_orig.size()))
		return false;

	for (size_t i = 0; i < list_tran.size(); i++) {
		if (list_tran[i] != list_orig[i])
			return false;
	}

	return true;
}

inline char *Translate(const char *orig)
{
	char *res = gettext(orig);
	return (((res == orig) || FormatSafe(orig, res)) ? res : const_cast<char*>(orig));
}

inline char *TranslatePlural(const char *one, const char *more, unsigned long count)
{
	char *res = ngettext(one, more, count);
	const char *orig = ((count == 1) ? one : more);
	return (((res == one) || (res == more) || FormatSafe(orig, res)) ? res : const_cast<char*>(orig));
}

	}; // namespace nI18n
}; // namespace nVerliHub

#define _(string) nVerliHub::nI18n::Translate(string)

#ifdef ngettext
#undef ngettext
#endif

#define ngettext(one, more, count) nVerliHub::nI18n::TranslatePlural(one, more, count) // note: same check for plural forms, name stays for xgettext

#endif
