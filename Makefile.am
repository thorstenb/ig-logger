ACLOCAL_AMFLAGS = -I m4
SUBDIRS =

bin_PROGRAMS = iglogger
iglogger_SOURCES = logig_main.cpp
iglogger_LDADD = $(LIBOBJS)

EXTRA_DIST = \
	AUTHORS \
	COPYING.MPL \
	NEWS \
	README \
	autogen.sh

distclean-local:
	rm -rf *.cache *~ *.out *.pc

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

dist-hook:
	git log --date=short --pretty="format:@%cd  %an  <%ae>  [%H]%n%n%s%n%n%e%b" | sed -e "s|^\([^@]\)|\t\1|" -e "s|^@||" >$(distdir)/ChangeLog

