/*
 * langinfo.c - parameter interface to langinfo via curses
 *
 * This file is part of zsh, the Z shell.
 *
 * Copyright (c) 2002 Peter Stephenson, Clint Adams
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and to distribute modified versions of this software for any
 * purpose, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * In no event shall Peter Stephenson, Clint Adams or the Zsh Development Group
 * be liable to any party for direct, indirect, special, incidental, or
 * consequential damages arising out of the use of this software and its
 * documentation, even if Sven Wishnowsky, Clint Adams and the Zsh
 * Development Group have been advised of the possibility of such damage.
 *
 * Peter Stephenson, Clint Adams and the Zsh Development Group specifically
 * disclaim any warranties, including, but not limited to, the implied
 * warranties of merchantability and fitness for a particular purpose.
 * The software provided hereunder is on an "as is" basis, and Peter
 * Stephenson, Clint Adams and the Zsh Development Group have no obligation
 * to provide maintenance, support, updates, enhancements, or modifications.
 *
 */

#include "langinfo.mdh"
#include "langinfo.pro"

static char langinfo_nam[] = "langinfo";

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

static Param langinfo_pm;

/**/
#ifdef HAVE_NL_LANGINFO

static char *nl_names[] = {
#ifdef CODESET
    "CODESET",
#endif /* CODESET */
#ifdef D_T_FMT
    "D_T_FMT",
#endif /* D_T_FMT */
#ifdef D_FMT
    "D_FMT",
#endif /* D_FMT */
#ifdef T_FMT
    "T_FMT",
#endif /* T_FMT */
#ifdef RADIXCHAR
    "RADIXCHAR",
#endif /* RADIXCHAR */
#ifdef THOUSEP
    "THOUSEP",
#endif /* THOUSEP */
#ifdef YESEXPR
    "YESEXPR",
#endif /* YESEXPR */
#ifdef NOEXPR
    "NOEXPR",
#endif /* NOEXPR */
#ifdef CRNCYSTR
    "CRNCYSTR",
#endif /* CRNCYSTR */
#ifdef ABDAY_1
    "ABDAY_1",
#endif /* ABDAY_1 */
#ifdef ABDAY_2
    "ABDAY_2",
#endif /* ABDAY_2 */
#ifdef ABDAY_3
    "ABDAY_3",
#endif /* ABDAY_3 */
#ifdef ABDAY_4
    "ABDAY_4",
#endif /* ABDAY_4 */
#ifdef ABDAY_5
    "ABDAY_5",
#endif /* ABDAY_5 */
#ifdef ABDAY_6
    "ABDAY_6",
#endif /* ABDAY_6 */
#ifdef ABDAY_7
    "ABDAY_7",
#endif /* ABDAY_7 */
#ifdef DAY_1
    "DAY_1",
#endif /* DAY_1 */
#ifdef DAY_2
    "DAY_2",
#endif /* DAY_2 */
#ifdef DAY_3
    "DAY_3",
#endif /* DAY_3 */
#ifdef DAY_4
    "DAY_4",
#endif /* DAY_4 */
#ifdef DAY_5
    "DAY_5",
#endif /* DAY_5 */
#ifdef DAY_6
    "DAY_6",
#endif /* DAY_6 */
#ifdef DAY_7
    "DAY_7",
#endif /* DAY_7 */
#ifdef ABMON_1
    "ABMON_1",
#endif /* ABMON_1 */
#ifdef ABMON_2
    "ABMON_2",
#endif /* ABMON_2 */
#ifdef ABMON_3
    "ABMON_3",
#endif /* ABMON_3 */
#ifdef ABMON_4
    "ABMON_4",
#endif /* ABMON_4 */
#ifdef ABMON_5
    "ABMON_5",
#endif /* ABMON_5 */
#ifdef ABMON_6
    "ABMON_6",
#endif /* ABMON_6 */
#ifdef ABMON_7
    "ABMON_7",
#endif /* ABMON_7 */
#ifdef ABMON_8
    "ABMON_8",
#endif /* ABMON_8 */
#ifdef ABMON_9
    "ABMON_9",
#endif /* ABMON_9 */
#ifdef ABMON_10
    "ABMON_10",
#endif /* ABMON_10 */
#ifdef ABMON_11
    "ABMON_11",
#endif /* ABMON_11 */
#ifdef ABMON_12
    "ABMON_12",
#endif /* ABMON_12 */
#ifdef MON_1
    "MON_1",
#endif /* MON_1 */
#ifdef MON_2
    "MON_2",
#endif /* MON_2 */
#ifdef MON_3
    "MON_3",
#endif /* MON_3 */
#ifdef MON_4
    "MON_4",
#endif /* MON_4 */
#ifdef MON_5
    "MON_5",
#endif /* MON_5 */
#ifdef MON_6
    "MON_6",
#endif /* MON_6 */
#ifdef MON_7
    "MON_7",
#endif /* MON_7 */
#ifdef MON_8
    "MON_8",
#endif /* MON_8 */
#ifdef MON_9
    "MON_9",
#endif /* MON_9 */
#ifdef MON_10
    "MON_10",
#endif /* MON_10 */
#ifdef MON_11
    "MON_11",
#endif /* MON_11 */
#ifdef MON_12
    "MON_12",
#endif /* MON_12 */
#ifdef T_FMT_AMPM
    "T_FMT_AMPM",
#endif /* T_FMT_AMPM */
#ifdef AM_STR
    "AM_STR",
#endif /* AM_STR */
#ifdef PM_STR
    "PM_STR",
#endif /* PM_STR */
#ifdef ERA
    "ERA",
#endif /* ERA */
#ifdef ERA_D_FMT
    "ERA_D_FMT",
#endif /* ERA_D_FMT */
#ifdef ERA_D_T_FMT
    "ERA_D_T_FMT",
#endif /* ERA_D_T_FMT */
#ifdef ERA_T_FMT
    "ERA_T_FMT",
#endif /* ERA_T_FMT */
#ifdef ALT_DIGITS
    "ALT_DIGITS",
#endif /* ALT_DIGITS */
    NULL
};

static nl_item nl_vals[] = {
#ifdef CODESET
    CODESET,
#endif /* CODESET */
#ifdef D_T_FMT
    D_T_FMT,
#endif /* D_T_FMT */
#ifdef D_FMT
    D_FMT,
#endif /* D_FMT */
#ifdef T_FMT
    T_FMT,
#endif /* T_FMT */
#ifdef RADIXCHAR
    RADIXCHAR,
#endif /* RADIXCHAR */
#ifdef THOUSEP
    THOUSEP,
#endif /* THOUSEP */
#ifdef YESEXPR
    YESEXPR,
#endif /* YESEXPR */
#ifdef NOEXPR
    NOEXPR,
#endif /* NOEXPR */
#ifdef CRNCYSTR
    CRNCYSTR,
#endif /* CRNCYSTR */
#ifdef ABDAY_1
    ABDAY_1,
#endif /* ABDAY_1 */
#ifdef ABDAY_2
    ABDAY_2,
#endif /* ABDAY_2 */
#ifdef ABDAY_3
    ABDAY_3,
#endif /* ABDAY_3 */
#ifdef ABDAY_4
    ABDAY_4,
#endif /* ABDAY_4 */
#ifdef ABDAY_5
    ABDAY_5,
#endif /* ABDAY_5 */
#ifdef ABDAY_6
    ABDAY_6,
#endif /* ABDAY_6 */
#ifdef ABDAY_7
    ABDAY_7,
#endif /* ABDAY_7 */
#ifdef DAY_1
    DAY_1,
#endif /* DAY_1 */
#ifdef DAY_2
    DAY_2,
#endif /* DAY_2 */
#ifdef DAY_3
    DAY_3,
#endif /* DAY_3 */
#ifdef DAY_4
    DAY_4,
#endif /* DAY_4 */
#ifdef DAY_5
    DAY_5,
#endif /* DAY_5 */
#ifdef DAY_6
    DAY_6,
#endif /* DAY_6 */
#ifdef DAY_7
    DAY_7,
#endif /* DAY_7 */
#ifdef ABMON_1
    ABMON_1,
#endif /* ABMON_1 */
#ifdef ABMON_2
    ABMON_2,
#endif /* ABMON_2 */
#ifdef ABMON_3
    ABMON_3,
#endif /* ABMON_3 */
#ifdef ABMON_4
    ABMON_4,
#endif /* ABMON_4 */
#ifdef ABMON_5
    ABMON_5,
#endif /* ABMON_5 */
#ifdef ABMON_6
    ABMON_6,
#endif /* ABMON_6 */
#ifdef ABMON_7
    ABMON_7,
#endif /* ABMON_7 */
#ifdef ABMON_8
    ABMON_8,
#endif /* ABMON_8 */
#ifdef ABMON_9
    ABMON_9,
#endif /* ABMON_9 */
#ifdef ABMON_10
    ABMON_10,
#endif /* ABMON_10 */
#ifdef ABMON_11
    ABMON_11,
#endif /* ABMON_11 */
#ifdef ABMON_12
    ABMON_12,
#endif /* ABMON_12 */
#ifdef MON_1
    MON_1,
#endif /* MON_1 */
#ifdef MON_2
    MON_2,
#endif /* MON_2 */
#ifdef MON_3
    MON_3,
#endif /* MON_3 */
#ifdef MON_4
    MON_4,
#endif /* MON_4 */
#ifdef MON_5
    MON_5,
#endif /* MON_5 */
#ifdef MON_6
    MON_6,
#endif /* MON_6 */
#ifdef MON_7
    MON_7,
#endif /* MON_7 */
#ifdef MON_8
    MON_8,
#endif /* MON_8 */
#ifdef MON_9
    MON_9,
#endif /* MON_9 */
#ifdef MON_10
    MON_10,
#endif /* MON_10 */
#ifdef MON_11
    MON_11,
#endif /* MON_11 */
#ifdef MON_12
    MON_12,
#endif /* MON_12 */
#ifdef T_FMT_AMPM
    T_FMT_AMPM,
#endif /* T_FMT_AMPM */
#ifdef AM_STR
    AM_STR,
#endif /* AM_STR */
#ifdef PM_STR
    PM_STR,
#endif /* PM_STR */
#ifdef ERA
    ERA,
#endif /* ERA */
#ifdef ERA_D_FMT
    ERA_D_FMT,
#endif /* ERA_D_FMT */
#ifdef ERA_D_T_FMT
    ERA_D_T_FMT,
#endif /* ERA_D_T_FMT */
#ifdef ERA_T_FMT
    ERA_T_FMT,
#endif /* ERA_T_FMT */
#ifdef ALT_DIGITS
    ALT_DIGITS,
#endif /* ALT_DIGITS */
    0
};

static nl_item *
liitem(char *name)
{
    char **element;
    nl_item *nlcode;

    nlcode = &nl_vals[0];

    for (element = (char **)nl_names; *element; element++, nlcode++) {
	if ((!strcmp(*element, name)))
	    return nlcode;
    }

    return NULL;
}

/**/
static void
shempty(void)
{
}

/* Create a simple special hash parameter. */

/**/
static Param
createlihash()
{
    Param pm;
    HashTable ht;

    unsetparam(langinfo_nam);

    if (!(pm = createparam(langinfo_nam, PM_SPECIAL|PM_HIDE|PM_HIDEVAL|
			   PM_REMOVABLE|PM_HASHED)))
	return NULL;

    pm->level = pm->old ? locallevel : 0;
    pm->gets.hfn = hashgetfn;
    pm->sets.hfn = hashsetfn;
    pm->unsetfn = stdunsetfn;
    pm->u.hash = ht = newhashtable(7, langinfo_nam, NULL);

    ht->hash        = hasher;
    ht->emptytable  = (TableFunc) shempty;
    ht->filltable   = NULL;
    ht->addnode     = (AddNodeFunc) shempty;
    ht->getnode     = ht->getnode2 = getlanginfo;
    ht->removenode  = (RemoveNodeFunc) shempty;
    ht->disablenode = NULL;
    ht->enablenode  = NULL;
    ht->freenode    = (FreeNodeFunc) shempty;
    ht->printnode   = printparamnode;
    ht->scantab     = scanlanginfo;

    return (langinfo_pm = pm);
}

/**/
static HashNode
getlanginfo(HashTable ht, char *name)
{
    int len, *elem;
    char *listr;
    Param pm = NULL;

    unmetafy(name, &len);

    pm = (Param) zhalloc(sizeof(struct param));
    pm->nam = dupstring(name);
    pm->flags = PM_READONLY | PM_SCALAR;
    pm->sets.cfn = NULL;
    pm->gets.cfn = strgetfn;
    pm->sets.ifn = NULL;
    pm->gets.ifn = intgetfn;
    pm->unsetfn = NULL;
    pm->ct = 0;
    pm->env = NULL;
    pm->ename = NULL;
    pm->old = NULL;
    pm->level = 0;

    if(name)
	elem = liitem(name);
    else
	elem = NULL;

    if (elem && (listr = nl_langinfo(*elem))) {
	pm->u.str = dupstring(listr);
    }
    else
    {
	/* zwarn("no such lang info: %s", name, 0); */
	pm->u.str = dupstring("");
	pm->flags |= PM_UNSET;
    }
    return (HashNode) pm;
}

/**/
static void
scanlanginfo(HashTable ht, ScanFunc func, int flags)
{
    Param pm = NULL;
    char **element, *langstr;
    nl_item *nlcode;

    pm = (Param) zhalloc(sizeof(struct param));
    pm->sets.cfn = NULL;
    pm->gets.cfn = strgetfn;
    pm->sets.ifn = NULL;
    pm->gets.ifn = intgetfn;
    pm->unsetfn = NULL;
    pm->ct = 0;
    pm->env = NULL;
    pm->ename = NULL;
    pm->old = NULL;
    
    pm->flags = PM_READONLY | PM_SCALAR;

    nlcode = &nl_vals[0];
    for (element = (char **)nl_names; *element; element++, nlcode++) {
	if ((langstr = nl_langinfo(*nlcode)) != NULL) {
	    pm->u.str = dupstring(langstr);
	    pm->nam = dupstring(*element);
	    func((HashNode) pm, flags);
	}
    }
    
}

/**/
#endif /* HAVE_NL_LANGINFO */

/**/
int
setup_(Module m)
{
    return 0;
}

/**/
int
boot_(Module m)
{
#ifdef HAVE_NL_LANGINFO
    if (!createlihash())
    	return 1;
#else
    unsetparam(langinfo_nam);
#endif
    return 0;
}

/**/
int
cleanup_(Module m)
{
#ifdef HAVE_NL_LANGINFO
    Param pm;

    if ((pm = (Param) paramtab->getnode(paramtab, langinfo_nam)) &&
	pm == langinfo_pm) {
	pm->flags &= ~PM_READONLY;
	unsetparam_pm(pm, 0, 1);
    }
#endif
    return 0;
}

/**/
int
finish_(Module m)
{
    return 0;
}
