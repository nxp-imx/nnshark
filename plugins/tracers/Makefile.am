lib_LTLIBRARIES = libgstshark.la

libgstshark_la_SOURCES = \
	gstdot.c \
	gstcpuusagecompute.c \
	gstgpuusagecompute.c \
	gstddrusagecompute.c \
	gstpwrusagecompute.c \
	gstproctimecompute.c \
	gstctf.c \
	gstparser.c \
	gstliveunit.c \
	gstliveprofiler.c \
	visualizeutil.c

libgstshark_la_CFLAGS = \
	$(GST_SHARK_OBJ_CFLAGS) \
	$(GST_CFLAGS) \
	$(GVC_CFLAGS)  \
	$(GIO_CFLAGS) \
	-DGST_USE_UNSTABLE_API \
	-Wno-undef

libgstshark_la_LIBADD = \
	$(GST_SHARK_OBJ_LIBS) \
	$(GST_LIBS) \
	$(GVC_LIBS) \
	$(GIO_LIBS) \
	-lgpuperfcnt \
	-lpthread \
	-lncurses

plugin_LTLIBRARIES = libgstsharktracers.la

libgstsharktracers_la_SOURCES = \
	gstplugin.c \
	gstsharktracer.c \
	gstgraphic.c \
	gstcpuusage.c \
	gstgpuusage.c \
	gstddrusage.c \
	gstpwrusage.c \
	gstproctime.c \
	gstinterlatency.c \
	gstscheduletime.c \
	gstframerate.c \
	gstqueuelevel.c \
	gstbitrate.c \
	gstbuffer.c \
	gstperiodictracer.c \
	gstlive.c

libgstsharktracers_la_CFLAGS = \
	$(GST_SHARK_OBJ_CFLAGS) \
	$(GST_CFLAGS) \
	-DGST_USE_UNSTABLE_API

libgstsharktracers_la_LIBADD = \
	$(GST_SHARK_OBJ_LIBS) \
	$(GST_LIBS) \
	libgstshark.la

libgstsharktracers_la_LDFLAGS = \
	$(GST_SHARK_PLUGIN_LDFLAGS)

libgstsharktracers_la_LIBTOOLFLAGS = \
	$(GST_SHARK_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS = \
	gstdot.h \
	gstcpuusage.h \
	gstcpuusagecompute.h \
	gstgpuusage.h \
	gstgpuusagecompute.h \
	gstddrusage.h \
	gstddrusagecompute.h \
	gstpwrusage.h \
	gstpwrusagecompute.h \
	gstgraphic.h \
	gstproctime.h \
	gstproctimecompute.h \
	gstinterlatency.h \
	gstscheduletime.h \
	gstframerate.h \
	gstctf.h \
	gstparser.h \
	gstqueuelevel.h \
	gstbitrate.h \
	gstbuffer.h \
	gstsharktracer.h \
	gstperiodictracer.h \
	gstliveunit.h \
	gstliveprofiler.h \
	gstlive.h \
	visualizeutil.h

CLEANFILES = *.gcno *.gcda *.gcov *.gcov.out

%.c.gcov: .libs/libgstshark_la-%.gcda %.c
	$(GCOV) -b -f -o $^ > $@.out

gcov: $(libgstshark_la_SOURCES:=.gcov)

