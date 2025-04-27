/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net

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

/* Convenience header for conditional use of GNU <libintl.h>.
   Copyright (C) 1995-1998, 2000-2002, 2004-2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.  */

#ifndef _LIBGETTEXT_H
#define _LIBGETTEXT_H 1

#define __(String) gettext (String)
#define _(String) gettext_noop (String)

/* NLS can be disabled through the configure --disable-nls option.  */
#if ENABLE_NLS

/* Get declarations of GNU message catalog functions.  */
# include <libintl.h>

#  ifdef HAVE_LOCALE_H
#    include <locale.h>
#  endif

#else

/* Solaris /usr/include/locale.h includes /usr/include/libintl.h, which
   chokes if dcgettext is defined as a macro.  So include it now, to make
   later inclusions of <locale.h> a NOP.  We don't include <libintl.h>
   as well because people using "gettext.h" will not include <libintl.h>,
   and also including <libintl.h> would fail on SunOS 4, whereas <locale.h>
   is OK.  */
#if defined(__sun)
# include <locale.h>
#endif

/* Many header files from the libstdc++ coming with g++ 3.3 or newer include
   <libintl.h>, which chokes if dcgettext is defined as a macro.  So include
   it now, to make later inclusions of <libintl.h> a NOP.  */
#if defined(__cplusplus) && defined(__GNUG__) && (__GNUC__ >= 3)
# include <cstdlib>
# if (__GLIBC__ >= 2) || _GLIBCXX_HAVE_LIBINTL_H
#  include <libintl.h>
# endif
#endif

/* Disabled NLS.
   The casts to 'const char *' serve the purpose of producing warnings
   for invalid uses of the value returned from these functions.
   On pre-ANSI systems without 'const', the config.h file is supposed to
   contain "#define const".  */
# define gettext(Msgid) ((char *) (Msgid))
# define dgettext(Domainname, Msgid) ((char *) (Msgid))
# define dcgettext(Domainname, Msgid, Category) ((char *) (Msgid))
# define ngettext(Msgid1, Msgid2, N) \
    ((N) == 1 ? (char *) (Msgid1) : (char *) (Msgid2))
# define dngettext(Domainname, Msgid1, Msgid2, N) \
    ((N) == 1 ? (char *) (Msgid1) : (char *) (Msgid2))
# define dcngettext(Domainname, Msgid1, Msgid2, N, Category) \
    ((N) == 1 ? (char *) (Msgid1) : (char *) (Msgid2))
# define textdomain(Domainname) ((char *) (Domainname))
# define bindtextdomain(Domainname, Dirname) ((char *) (Dirname))
# define bind_textdomain_codeset(Domainname, Codeset) ((char *) (Codeset))

#endif

/* A pseudo function call that serves as a marker for the automated
   extraction of messages, but does not call gettext().  The run-time
   translation is done at a different place in the code.
   The argument, String, should be a literal string.  Concatenated strings
   and other string expressions won't work.
   The macro's expansion is not parenthesized, so that it is suitable as
   initializer for static 'char[]' or 'const char[]' variables.  */
#define gettext_noop(String) String

/* The separator between msgctxt and msgid in a .mo file.  */
#define GETTEXT_CONTEXT_GLUE "\004"

/* Pseudo function calls, taking a MSGCTXT and a MSGID instead of just a
   MSGID.  MSGCTXT and MSGID must be string literals.  MSGCTXT should be
   short and rarely need to change.
   The letter 'p' stands for 'particular' or 'special'.  */
#define pgettext(Msgctxt, Msgid) \
  pgettext_aux (NULL, Msgctxt GETTEXT_CONTEXT_GLUE Msgid, Msgid, LC_MESSAGES)
#define dpgettext(Domainname, Msgctxt, Msgid) \
  pgettext_aux (Domainname, Msgctxt GETTEXT_CONTEXT_GLUE Msgid, Msgid, LC_MESSAGES)
#define dcpgettext(Domainname, Msgctxt, Msgid, Category) \
  pgettext_aux (Domainname, Msgctxt GETTEXT_CONTEXT_GLUE Msgid, Msgid, Category)
#define npgettext(Msgctxt, Msgid, MsgidPlural, N) \
  npgettext_aux (NULL, Msgctxt GETTEXT_CONTEXT_GLUE Msgid, Msgid, MsgidPlural, N, LC_MESSAGES)
#define dnpgettext(Domainname, Msgctxt, Msgid, MsgidPlural, N) \
  npgettext_aux (Domainname, Msgctxt GETTEXT_CONTEXT_GLUE Msgid, Msgid, MsgidPlural, N, LC_MESSAGES)
#define dcnpgettext(Domainname, Msgctxt, Msgid, MsgidPlural, N, Category) \
  npgettext_aux (Domainname, Msgctxt GETTEXT_CONTEXT_GLUE Msgid, Msgid, MsgidPlural, N, Category)

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static const char *
pgettext_aux (const char *domain,
	      const char *msg_ctxt_id, const char *msgid,
	      int category)
{
  const char *translation = dcgettext (domain, msg_ctxt_id, category);
  if (translation == msg_ctxt_id)
    return msgid;
  else
    return translation;
}

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static const char *
npgettext_aux (const char *domain,
	       const char *msg_ctxt_id, const char *msgid,
	       const char *msgid_plural, unsigned long int n,
	       int category)
{
  const char *translation =
    dcngettext (domain, msg_ctxt_id, msgid_plural, n, category);
  if (translation == msg_ctxt_id || translation == msgid_plural)
    return (n == 1 ? msgid : msgid_plural);
  else
    return translation;
}

/* The same thing extended for non-constant arguments.  Here MSGCTXT and MSGID
   can be arbitrary expressions.  But for string literals these macros are
   less efficient than those above.  */

#include <string.h>

#define _LIBGETTEXT_HAVE_VARIABLE_SIZE_ARRAYS \
  (__GNUC__ >= 3 || defined __cplusplus)

#if !_LIBGETTEXT_HAVE_VARIABLE_SIZE_ARRAYS
#include <stdlib.h>
#endif

#define pgettext_expr(Msgctxt, Msgid) \
  dcpgettext_expr (NULL, Msgctxt, Msgid, LC_MESSAGES)
#define dpgettext_expr(Domainname, Msgctxt, Msgid) \
  dcpgettext_expr (Domainname, Msgctxt, Msgid, LC_MESSAGES)

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static const char *
dcpgettext_expr (const char *domain,
		 const char *msgctxt, const char *msgid,
		 int category)
{
  size_t msgctxt_len = strlen (msgctxt) + 1;
  size_t msgid_len = strlen (msgid) + 1;
  const char *translation;
#if _LIBGETTEXT_HAVE_VARIABLE_SIZE_ARRAYS
  char msg_ctxt_id[msgctxt_len + msgid_len];
#else
  char buf[1024];
  char *msg_ctxt_id =
    (msgctxt_len + msgid_len <= sizeof (buf)
     ? buf
     : (char *) malloc (msgctxt_len + msgid_len));
  if (msg_ctxt_id != NULL)
#endif
    {
      memcpy (msg_ctxt_id, msgctxt, msgctxt_len - 1);
      msg_ctxt_id[msgctxt_len - 1] = '\004';
      memcpy (msg_ctxt_id + msgctxt_len, msgid, msgid_len);
      translation = dcgettext (domain, msg_ctxt_id, category);
#if !_LIBGETTEXT_HAVE_VARIABLE_SIZE_ARRAYS
      if (msg_ctxt_id != buf)
	free (msg_ctxt_id);
#endif
      if (translation != msg_ctxt_id)
	return translation;
    }
  return msgid;
}

#define npgettext_expr(Msgctxt, Msgid, MsgidPlural, N) \
  dcnpgettext_expr (NULL, Msgctxt, Msgid, MsgidPlural, N, LC_MESSAGES)
#define dnpgettext_expr(Domainname, Msgctxt, Msgid, MsgidPlural, N) \
  dcnpgettext_expr (Domainname, Msgctxt, Msgid, MsgidPlural, N, LC_MESSAGES)

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static const char *
dcnpgettext_expr (const char *domain,
		  const char *msgctxt, const char *msgid,
		  const char *msgid_plural, unsigned long int n,
		  int category)
{
  size_t msgctxt_len = strlen (msgctxt) + 1;
  size_t msgid_len = strlen (msgid) + 1;
  const char *translation;
#if _LIBGETTEXT_HAVE_VARIABLE_SIZE_ARRAYS
  char msg_ctxt_id[msgctxt_len + msgid_len];
#else
  char buf[1024];
  char *msg_ctxt_id =
    (msgctxt_len + msgid_len <= sizeof (buf)
     ? buf
     : (char *) malloc (msgctxt_len + msgid_len));
  if (msg_ctxt_id != NULL)
#endif
    {
      memcpy (msg_ctxt_id, msgctxt, msgctxt_len - 1);
      msg_ctxt_id[msgctxt_len - 1] = '\004';
      memcpy (msg_ctxt_id + msgctxt_len, msgid, msgid_len);
      translation = dcngettext (domain, msg_ctxt_id, msgid_plural, n, category);
#if !_LIBGETTEXT_HAVE_VARIABLE_SIZE_ARRAYS
      if (msg_ctxt_id != buf)
	free (msg_ctxt_id);
#endif
      if (!(translation == msg_ctxt_id || translation == msgid_plural))
	return translation;
    }
  return (n == 1 ? msgid : msgid_plural);
}

#endif /* _LIBGETTEXT_H */
