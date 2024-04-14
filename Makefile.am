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
  mvtx_standalone_cluster.h

lib_LTLIBRARIES = \
  libmvtxstandalonecluster.la

libmvtxstandalonecluster_la_SOURCES = \
  mvtx_standalone_cluster.cc

libmvtxstandalonecluster_la_LIBADD = \
  -lffaobjects \
  -lphool \
  -ltrack_io \
  -lg4detectors \
  -lmvtx \
  -lSubsysReco

BUILT_SOURCES = testexternals.cc

noinst_PROGRAMS = \
  testexternals

testexternals_SOURCES = testexternals.cc
testexternals_LDADD   = libmvtxstandalonecluster.la

testexternals.cc:
	echo "//*** this is a generated file. Do not commit, do not edit" > $@
	echo "int main()" >> $@
	echo "{" >> $@
	echo "  return 0;" >> $@
	echo "}" >> $@

clean-local:
	rm -f $(BUILT_SOURCES)
