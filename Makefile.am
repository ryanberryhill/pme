AM_CPPFLAGS = -I$(srcdir)/src 
AM_CXXFLAGS = -Wall -Wpedantic -Werror
# No pedantic for CFLAGS because aiger has some mixed code and declarations
# that gcc4 errors about
AM_CFLAGS = -Wall -Werror

ACLOCAL_AMFLAGS = -I m4

noinst_LIBRARIES = libpme.a libsat.a

libpme_a_SOURCES = src/aiger/aiger.h \
                   src/aiger/aiger.c \
                   src/pme/pme.h \
                   src/pme/pme.cpp \
                   src/pme/id.h \
                   src/pme/id.cpp \
                   src/pme/cpme.cpp \
                   src/pme/engine/engine.h \
                   src/pme/engine/engine.cpp \
                   src/pme/engine/transition_relation.h \
                   src/pme/engine/transition_relation.cpp \
                   src/pme/engine/sat_adaptor.h \
                   src/pme/engine/sat_adaptor.cpp \
                   src/pme/engine/variable_manager.h \
                   src/pme/engine/variable_manager.cpp \
                   src/pme/util/cardinality_constraint.h \
                   src/pme/util/cardinality_constraint.cpp \
                   src/pme/util/proof_checker.h \
                   src/pme/util/proof_checker.cpp 

libpme_a_LIBADD = $(SAT_LIBS)

libsat_a_SOURCES = src/sat/sat.h \
                   src/sat/sat.cpp \
                   src/sat/minisat_style_solver.cpp \
                   src/sat/glucose.cpp \
                   src/sat/minisat.cpp

libsat_a_LIBADD = src/sat/solvers/glucose/libglucose.a \
                  src/sat/solvers/minisat/libminisat.a

SAT_LIBS = libsat.a \
           src/sat/solvers/glucose/libglucose.a \
           src/sat/solvers/minisat/libminisat.a 

# No Werror because glucose and minisat headers cause several warnings
libsat_a_CPPFLAGS = -I$(srcdir)/src \
                    -I$(srcdir)/src/sat/solvers/glucose/src \
                    -I$(srcdir)/src/sat/solvers/minisat/
libsat_a_CXXFLAGS = -Wall -Wpedantic 

bin_PROGRAMS = pme parseproof

pme_SOURCES = src/main/main.c \
              src/aiger/aiger.h \
              src/aiger/aiger.c

pme_LDADD = libpme.a \
            $(SAT_LIBS) \
            -lstdc++ -lm

parseproof_SOURCES = src/parseproof/main.cpp \
                     src/aiger/aiger.h \
                     src/aiger/aiger.c

TESTS = test_sat_adaptor \
        test_sat \
        test_transition_relation \
        test_variable_manager \
        test_cardinality_constraint \
        test_proof_checker

check_PROGRAMS = $(TESTS)

test_variable_manager_SOURCES = tests/test_variable_manager.cpp
test_variable_manager_LDADD = libpme.a \
                                -lboost_unit_test_framework


test_sat_adaptor_SOURCES = tests/test_sat_adaptor.cpp  
test_sat_adaptor_LDADD = libpme.a \
                         $(SAT_LIBS) \
                          -lboost_unit_test_framework


test_sat_SOURCES = tests/test_sat.cpp  
test_sat_LDADD = $(SAT_LIBS) \
                 -lboost_unit_test_framework

test_transition_relation_SOURCES = tests/test_transition_relation.cpp
test_transition_relation_LDADD = libpme.a \
                                 $(SAT_LIBS) \
                                 -lboost_unit_test_framework

test_cardinality_constraint_SOURCES = tests/test_cardinality_constraint.cpp
test_cardinality_constraint_LDADD = libpme.a \
                                    $(SAT_LIBS) \
                                    -lboost_unit_test_framework

test_proof_checker_SOURCES = tests/test_proof_checker.cpp  
test_proof_checker_LDADD = libpme.a \
                           $(SAT_LIBS) \
                           -lboost_unit_test_framework

SUBDIRS = src/sat/solvers/glucose src/sat/solvers/minisat 

EXTRA_DIST = LICENSE README.md glucose.supp

# Glucose can generate memcheck errors with certain compilers and flags
@VALGRIND_CHECK_RULES@
VALGRIND_SUPPRESSIONS_FILES = glucose.supp

