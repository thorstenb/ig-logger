# Process this file with autoconf to create configure.

AC_PREREQ([2.65])

# ====================
# Version informations
# ====================
m4_define([iglogger_version_major],[0])
m4_define([iglogger_version_minor],[0])
m4_define([iglogger_version_micro],[14])
m4_define([iglogger_version],[iglogger_version_major.iglogger_version_minor.iglogger_version_micro])

# =============
# Automake init
# =============
AC_INIT([iglogger],[iglogger_version])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([1.11 foreign dist-xz dist-bzip2])
AM_SILENT_RULES([yes])
AC_LANG([C++])

# ===========================
# Find required base packages
# ===========================
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT([win32-dll disable-static pic-only])
AC_CANONICAL_HOST

PKG_PROG_PKG_CONFIG([0.20])

# ===========================
# Find required boost headers
# ===========================

AC_CHECK_HEADERS(
	boost/optional.hpp \
	boost/property_tree/ptree.hpp \
	boost/spirit/include/classic.hpp \
	boost/property_tree/json_parser.hpp,
	[],
	[AC_MSG_ERROR(Required boost headers not found. Install boost >= 1.41.0)],
	[]
)

# ================
# Check for cflags
# ================
AC_ARG_ENABLE([werror],
	[AS_HELP_STRING([--disable-werror], [Treat all warnings as errors, usefull for development])],
	[enable_werror="$enableval"],
	[enable_werror=yes]
)
AS_IF([test x"$enable_werror" != "xno"], [
	CFLAGS="$CFLAGS -Werror"
	CXXFLAGS="$CXXFLAGS -Werror"
])
AC_ARG_ENABLE([weffc],
	[AS_HELP_STRING([--disable-weffc], [ Disable -Weffc++ warnings, usefull when using an old version of gcc or of boost])],
	[enable_weffc="$enableval"],
	[enable_weffc=yes]
)
AC_ARG_ENABLE([wparanoic],
	[AS_HELP_STRING([--enable-wparanoic], [Enable a lot of warnings...])],
	[enable_wparanoic="$enableval"],
	[enable_wparanoic=no]
)
# Courtesy of Glib: Ensure MSVC-compatible struct packing convention
# is used when compiling for Win32 with gcc.
AS_IF([test x"$GCC" = xyes], [
	# Be tough with warnings and produce less careless code
	CFLAGS="$CFLAGS -Wall -Wextra -pedantic"
	CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic -Wshadow -Wunused-variable"
	AS_IF([test x"$enable_weffc" != "xno"], [
		CXXFLAGS="$CXXFLAGS -Weffc++"
	])
	AS_IF([test x"$enable_wparanoic" != "xno"], [
		CXXFLAGS="$CXXFLAGS -Wcast-align -Wcast-qual -Wchar-subscripts"
		CXXFLAGS="$CXXFLAGS -Wcomment -Wconversion -Wdisabled-optimization"
		CXXFLAGS="$CXXFLAGS -Wfloat-equal -Wformat -Wformat=2"
		CXXFLAGS="$CXXFLAGS -Wformat-nonliteral -Wformat-security"
		CXXFLAGS="$CXXFLAGS -Wformat-y2k -Wimport -Winit-self -Winvalid-pch"
		CXXFLAGS="$CXXFLAGS -Wmissing-braces -Wmissing-field-initializers"
		CXXFLAGS="$CXXFLAGS -Wmissing-format-attribute -Wmissing-include-dirs"
		CXXFLAGS="$CXXFLAGS -Wmissing-noreturn -Wpacked -Wparentheses"
		CXXFLAGS="$CXXFLAGS -Wpointer-arith -Wredundant-decls -Wreturn-type"
		CXXFLAGS="$CXXFLAGS -Wsequence-point -Wsign-compare -Wstrict-aliasing"
		CXXFLAGS="$CXXFLAGS -Wstrict-aliasing=2 -Wswitch -Wswitch-default"
		CXXFLAGS="$CXXFLAGS -Wswitch-enum -Wtrigraphs -Wunknown-pragmas -Wunused"
		CXXFLAGS="$CXXFLAGS -Wunused-function -Wunused-label -Wunused-parameter"
		CXXFLAGS="$CXXFLAGS -Wunused-value -Wvariadic-macros"
		CXXFLAGS="$CXXFLAGS -Wvolatile-register-var -Wwrite-strings"
	])
])
AC_SUBST(DEBUG_CXXFLAGS)

# ============
# Debug switch
# ============
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug], [Turn on debugging])],
	[enable_debug="$enableval"],
	[enable_debug=no]
)
AS_IF([test "x$enable_debug" = "xyes"], [
	DEBUG_CXXFLAGS="-DDEBUG -g"
	CXXFLAGS="$CXXFLAGS -O0"
	CFLAGS="$CFLAGS -O0"
], [
	DEBUG_CXXFLAGS="-DNDEBUG"
])
AC_SUBST(DEBUG_CXXFLAGS)

# =============
# Documentation
# =============
AC_ARG_WITH(docs,
	[AS_HELP_STRING([--without-docs], [Do not build documentation])],
	[with_docs="$withval"],
	[AS_IF([test "x$native_win32" = "xyes"], [with_docs=no], [with_docs=yes])]
)
AS_IF([test "x$with_docs" != "xno"], [
	AC_PATH_PROG(DOXYGEN, [doxygen])
	AS_IF([test -z "$DOXYGEN"], [
		AC_MSG_WARN([*** Could not find doxygen in your PATH.])
		AC_MSG_WARN([*** The documentation will not be built.])
		build_docs=no
	], [build_docs=yes])
], [build_docs=no])
AM_CONDITIONAL([WITH_LIBCDR_DOCS], [test "x$build_docs" != "xno"])

# =====================
# Prepare all .in files
# =====================
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    tests/Makefile
])
AC_OUTPUT

# ==============================================
# Display final informations about configuration
# ==============================================
AC_MSG_NOTICE([
==============================================================================
Build configuration:
	debug:           ${enable_debug}
	docs:            ${build_docs}
	static-tools:    ${enable_static_tools}
	werror:          ${enable_werror}
==============================================================================
])
