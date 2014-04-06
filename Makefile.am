AUTOMAKE_OPTIONS=dist-xz

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src tests

EXTRA_DIST = \
	AUTHORS \
	COPYING.MPL \
	NEWS \
	README \
	autogen.sh

check:
	astyle --style=allman -n -s4 -r -y -S -Y -k1 -W1 -c -xC79 --formatted \*.cpp \*.h | grep -q Formatted

distclean-local:
	rm -rf *.cache *~ *.out *.pc

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

dist-hook:
	git log --date=short --pretty="format:@%cd  %an  <%ae>  [%H]%n%n%s%n%n%e%b" | sed -e "s|^\([^@]\)|\t\1|" -e "s|^@||" >$(distdir)/ChangeLog

.PHONY: distclean-local rpm dist-hook check
