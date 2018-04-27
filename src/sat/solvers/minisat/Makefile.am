AM_CXXFLAGS = -Wall -Wno-parentheses -O3 
AM_CPPFLAGS = -I$(srcdir) -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS 

ACLOCAL_AMFLAGS = -I m4

noinst_LIBRARIES = libminisat.a

libminisat_a_SOURCES = \
  minisat/simp/SimpSolver.cc \
  minisat/simp/SimpSolver.h \
  minisat/core/Dimacs.h \
  minisat/core/Solver.cc \
  minisat/core/Solver.h \
  minisat/core/SolverTypes.h \
  minisat/mtl/Alg.h \
  minisat/mtl/Alloc.h \
  minisat/mtl/Heap.h \
  minisat/mtl/IntMap.h \
  minisat/mtl/IntTypes.h \
  minisat/mtl/Map.h \
  minisat/mtl/Queue.h \
  minisat/mtl/Rnd.h \
  minisat/mtl/Sort.h \
  minisat/mtl/Vec.h \
  minisat/mtl/XAlloc.h \
  minisat/utils/Options.cc \
  minisat/utils/Options.h \
  minisat/utils/ParseUtils.h \
  minisat/utils/System.h \
  minisat/utils/System.cc

