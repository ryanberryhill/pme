AM_CXXFLAGS = -Wall -Wno-parentheses -O3 
AM_CPPFLAGS = -I$(srcdir)/src -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS 

ACLOCAL_AMFLAGS = -I m4

noinst_LIBRARIES = libglucose.a

libglucose_a_SOURCES = \
  src/simp/SimpSolver.cc \
  src/simp/SimpSolver.h \
  src/core/BoundedQueue.h \
  src/core/Constants.h \
  src/core/Dimacs.h \
  src/core/Solver.cc \
  src/core/Solver.h \
  src/core/SolverStats.h \
  src/core/SolverTypes.h \
  src/mtl/Alg.h \
  src/mtl/Alloc.h \
  src/mtl/Clone.h \
  src/mtl/Heap.h \
  src/mtl/IntTypes.h \
  src/mtl/Map.h \
  src/mtl/Queue.h \
  src/mtl/Sort.h \
  src/mtl/VecThreads.h \
  src/mtl/Vec.h \
  src/mtl/XAlloc.h \
  src/utils/Options.cc \
  src/utils/Options.h \
  src/utils/ParseUtils.h \
  src/utils/System.h \
  src/utils/System.cc

