bin_PROGRAMS = \
		serve \
		serve-supervise \
		serve-kickstart

AM_CFLAGS = \
		-DSERVE_SUPERVISE="\"@bindir@/serve-supervise\"" \
		-DSERVE_DIR="\"@sysconfdir@/serve\""

bin_SCRIPTS = serve-list

serve_SOURCES = serve.c

serve_supervise_SOURCES = supervise.c sig.c

serve_kickstart_SOURCES = kickstart.c sig.c

noinst_HEADERS = \
		serve.h \
		sig.h

EXTRA_DIST = $(bin_SCRIPTS)

