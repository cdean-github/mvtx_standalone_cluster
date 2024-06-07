AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = \
  -I$(includedir) \
  -I$(OFFLINE_MAIN)/include \
  -isystem$(ROOTSYS)/include

AM_LDFLAGS = \
  -L$(libdir) \
  -L$(OFFLINE_MAIN)/lib \
  -L$(OFFLINE_MAIN)/lib64

pkginclude_HEADERS = \
  mvtx_standalone_cluster.h \
  event_display_maker.h \
  silicon_detector_analyser.h \
  mvtx_file_finder.h \
  mvtx_hit_map.h 
 
lib_LTLIBRARIES = \
  libeventdisplaymaker.la

libeventdisplaymaker_la_SOURCES = \
  mvtx_standalone_cluster.cc \
  event_display_maker.cc \
  silicon_detector_analyser.cc \
  mvtx_file_finder.cc \
  mvtx_hit_map.cc

libeventdisplaymaker_la_LIBADD = \
  -lphool \
  -ltrack_io \
  -lg4detectors \
  -lintt \
  -lmvtx \
  -lmicromegas \
  -lglobalvertex \
  -ltrackbase_historic_io \
  -lSubsysReco

BUILT_SOURCES = testexternals.cc

noinst_PROGRAMS = \
  testexternals

testexternals_SOURCES = testexternals.cc
testexternals_LDADD   = libeventdisplaymaker.la

testexternals.cc:
	echo "//*** this is a generated file. Do not commit, do not edit" > $@
	echo "int main()" >> $@
	echo "{" >> $@
	echo "  return 0;" >> $@
	echo "}" >> $@

clean-local:
	rm -f $(BUILT_SOURCES)
