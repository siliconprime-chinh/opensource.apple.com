# $Id: Makefile.am,v 1.2 2003/07/25 18:37:24 jkh Exp $
# Makefile.am for texinfo/makeinfo.
# Run automake in .. to produce Makefile.in from this.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = makeinfo

localedir = $(datadir)/locale
INCLUDES = -I$(top_srcdir)/lib -I../intl -DLOCALEDIR=\"$(localedir)\"
LDADD = ../lib/libtxi.a $(LIBINTL)

makeinfo_SOURCES = \
  cmds.c cmds.h defun.c defun.h \
  files.c files.h footnote.c footnote.h \
  html.c html.h index.c index.h insertion.c insertion.h lang.c lang.h \
  macro.c macro.h makeinfo.c makeinfo.h multi.c node.c node.h \
  sectioning.c sectioning.h toc.c toc.h xml.c xml.h

pkgdata_DATA = texinfo.dtd texinfo.xsl

EXTRA_DIST = README $(pkgdata_DATA)

SUBDIRS = tests
