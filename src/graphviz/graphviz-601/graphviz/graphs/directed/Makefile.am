## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.4

GRAPHS = KW91.dot Latin1.dot NaN.dot abstract.dot alf.dot arrows.dot \
	awilliams.dot clust.dot clust1.dot clust2.dot clust3.dot clust4.dot \
	clust5.dot crazy.dot ctext.dot dfa.dot fig6.dot fsm.dot \
	grammar.dot hashtable.dot honda-tokoro.dot jcctree.dot jsort.dot \
	ldbxtried.dot longflat.dot mike.dot nhg.dot oldarrows.dot \
	pgram.dot pm2way.dot pmpipe.dot polypoly.dot proc3d.dot \
	record2.dot records.dot rowe.dot sdh.dot shells.dot states.dot \
	structs.dot table.dot train11.dot trapeziumlr.dot tree.dot triedds.dot \
	try.dot unix.dot unix2.dot unix2k.dot viewfile.dot world.dot

graphdir = $(datadir)/@PACKAGE@/graphs
directeddir = $(graphdir)/directed
directed_DATA = $(GRAPHS)

EXTRA_DIST = $(GRAPHS)

CLEANFILES = core *.dot.* *.png .xvpics/* 

test:
	for i in $(GRAPHS); do \
		echo "dot $$i"; \
		$(top_builddir)/dotneato/dot \
			-Tcanon -o$$i.canon \
			-Tcmap -o$$i.cmap \
			-Tcmapx -o$$i.cmapx \
			-Tdia -o$$i.dia \
			-Tdot -o$$i.dot \
			-Tfig -o$$i.fig \
			-Tgd -o$$i.gd \
			-Tgd2 -o$$i.gd2 \
			-Tgif -o$$i.gif \
			-Thpgl -o$$i.hpgl \
			-Tismap -o$$i.ismap \
			-Timap -o$$i.imap \
			-Tjpg -o$$i.jpg \
			-Tmif -o$$i.mif \
			-Tmp -o$$i.mp \
			-Tpcl -o$$i.pcl \
			-Tpic -o$$i.pic \
			-Tplain -o$$i.plain \
			-Tplain-ext -o$$i.plain-ext \
			-Tpng -o$$i.png \
			-Tps -o$$i.ps \
			-Tps2 -o$$i.ps2 \
			-Tsvg -o$$i.svg \
			-Tsvgz -o$$i.svgz \
			-Twbmp -o$$i.wbmp \
			-Tvrml -o$$i.vrml \
			-Tvtx -o$$i.vtx \
			-Txdot -o$$i.xdot \
				$(top_srcdir)/graphs/directed/$$i; \
	done

canon:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tcanon -o$$i.canon $(top_srcdir)/graphs/directed/$$i; done

cmap:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tcmap -o$$i.cmap $(top_srcdir)/graphs/directed/$$i; done

cmapx:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tcmapx -o$$i.cmapx $(top_srcdir)/graphs/directed/$$i; done

dia:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tdia -o$$i.dia $(top_srcdir)/graphs/directed/$$i; done

dot:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tdot -o$$i.dot $(top_srcdir)/graphs/directed/$$i; done

fig:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tfig -o$$i.fig $(top_srcdir)/graphs/directed/$$i; done

gd:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tgd -o$$i.gd $(top_srcdir)/graphs/directed/$$i; done

gd2:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tgd2 -o$$i.gd2 $(top_srcdir)/graphs/directed/$$i; done

gif:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tgif -o$$i.gif $(top_srcdir)/graphs/directed/$$i; done

hpgl:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Thpgl -o$$i.hpgl $(top_srcdir)/graphs/directed/$$i; done

ismap:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tismap -o$$i.ismap $(top_srcdir)/graphs/directed/$$i; done

imap:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Timap -o$$i.imap $(top_srcdir)/graphs/directed/$$i; done

jpg:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tjpg -o$$i.jpg $(top_srcdir)/graphs/directed/$$i; done

mif:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tmif -o$$i.mif $(top_srcdir)/graphs/directed/$$i; done

mp:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tmp -o$$i.mp $(top_srcdir)/graphs/directed/$$i; done

pcl:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tpcl -o$$i.pcl $(top_srcdir)/graphs/directed/$$i; done

pic:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tpic -o$$i.pic $(top_srcdir)/graphs/directed/$$i; done

plain:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tplain -o$$i.plain $(top_srcdir)/graphs/directed/$$i; done

plain-ext:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tplain-ext -o$$i.plain-ext $(top_srcdir)/graphs/directed/$$i; done

png:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tpng -o$$i.png $(top_srcdir)/graphs/directed/$$i; done

ps:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tps -o$$i.ps $(top_srcdir)/graphs/directed/$$i; done

ps2:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tps2 -o$$i.ps2 $(top_srcdir)/graphs/directed/$$i; done

svg:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tsvg -o$$i.svg $(top_srcdir)/graphs/directed/$$i; done

svgz:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tsvgz -o$$i.svgz $(top_srcdir)/graphs/directed/$$i; done

wbmp:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Twbmp -o$$i.wbmp $(top_srcdir)/graphs/directed/$$i; done

vrml:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tvrml -o$$i.vrml $(top_srcdir)/graphs/directed/$$i; done

vtx:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Tvtx -o$$i.vtx $(top_srcdir)/graphs/directed/$$i; done

xdot:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(top_builddir)/dotneato/dot -Txdot -o$$i.xdot $(top_srcdir)/graphs/directed/$$i; done
