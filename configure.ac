AC_INIT([mines], [0.1])
AM_INIT_AUTOMAKE([subdir-objects])
AC_PROG_CXX

# AC_CANONICAL_HOST is needed to access the 'host_os' variable
AC_CANONICAL_HOST

build_linux=no
build_mac=no

# Detect the target system
case "${host_os}" in
    linux*)
        build_linux=yes
        ;;
    darwin*)
        build_mac=yes
        ;;
    *)
        AC_MSG_ERROR(["OS $host_os is not supported"])
        ;;
esac

# Pass the conditionals to automake
AM_CONDITIONAL([LINUX], [test "$build_linux" = "yes"])
AM_CONDITIONAL([OSX], [test "$build_mac" = "yes"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
