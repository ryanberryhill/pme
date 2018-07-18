AM_CPPFLAGS = -I$(srcdir)/src 
# unused private field warnings are only supported in clang, so get rid
# of them for consistency
AM_CXXFLAGS = -Wall -Wpedantic -Werror -Wno-unused-private-field
# No pedantic for CFLAGS because aiger has some mixed code and declarations
# that gcc4 errors about
AM_CFLAGS = -Wall -Werror

ACLOCAL_AMFLAGS = -I m4

bin_SCRIPTS = scripts/pme-canonicalize
noinst_SCRIPTS = scripts/run-integration scripts/make-golden

noinst_LIBRARIES = libpme.a libsat.a

libpme_a_SOURCES = src/aiger/aiger.h \
                   src/aiger/aiger.c \
                   src/pme/pme.h \
                   src/pme/pme.cpp \
                   src/pme/id.h \
                   src/pme/id.cpp \
                   src/pme/cpme.cpp \
                   src/pme/safety.h \
                   src/pme/engine/engine.h \
                   src/pme/engine/engine.cpp \
                   src/pme/engine/transition_relation.h \
                   src/pme/engine/transition_relation.cpp \
                   src/pme/engine/debug_transition_relation.h \
                   src/pme/engine/debug_transition_relation.cpp \
                   src/pme/engine/sat_adaptor.h \
                   src/pme/engine/sat_adaptor.cpp \
                   src/pme/engine/variable_manager.h \
                   src/pme/engine/variable_manager.cpp \
                   src/pme/engine/consecution_checker.h \
                   src/pme/engine/consecution_checker.cpp \
                   src/pme/engine/collapse_set_finder.h \
                   src/pme/engine/collapse_set_finder.cpp \
                   src/pme/engine/global_state.h \
                   src/pme/engine/global_state.cpp \
                   src/pme/engine/logger.h \
                   src/pme/engine/logger.cpp \
                   src/pme/util/cardinality_constraint.h \
                   src/pme/util/cardinality_constraint.cpp \
                   src/pme/util/clause_database.h \
                   src/pme/util/clause_database.cpp \
                   src/pme/util/proof_checker.h \
                   src/pme/util/proof_checker.cpp \
                   src/pme/util/maxsat_solver.h \
                   src/pme/util/maxsat_solver.cpp \
                   src/pme/util/find_safe_mis.h \
                   src/pme/util/find_safe_mis.cpp \
                   src/pme/util/find_minimal_support.h \
                   src/pme/util/find_minimal_support.cpp \
                   src/pme/util/simplify_tr.h \
                   src/pme/util/simplify_tr.cpp \
                   src/pme/minimization/marco.h \
                   src/pme/minimization/marco.cpp \
                   src/pme/minimization/camsis.h \
                   src/pme/minimization/camsis.cpp \
                   src/pme/minimization/brute_force.h \
                   src/pme/minimization/brute_force.cpp \
                   src/pme/minimization/sisi.h \
                   src/pme/minimization/sisi.cpp \
                   src/pme/minimization/minimization.h \
                   src/pme/minimization/minimization.cpp \
                   src/pme/ic3/ic3.h \
                   src/pme/ic3/ic3.cpp \
                   src/pme/ic3/ic3_solver.h \
                   src/pme/ic3/ic3_solver.cpp \
                   src/pme/ic3/ic3_debugger.h \
                   src/pme/ic3/ic3_debugger.cpp \
                   src/pme/ic3/inductive_trace.h \
                   src/pme/ic3/inductive_trace.cpp \
                   src/pme/ic3/frame_solver.h \
                   src/pme/ic3/frame_solver.cpp \
                   src/pme/ic3/transition_relation_solver.h \
                   src/pme/ic3/transition_relation_solver.cpp \
                   src/pme/ic3/unsat_core_lifter.h \
                   src/pme/ic3/unsat_core_lifter.cpp \
                   src/pme/bmc/bmc_solver.h \
                   src/pme/bmc/bmc_solver.cpp \
                   src/pme/ivc/ivc.h \
                   src/pme/ivc/ivc.cpp \
                   src/pme/ivc/correction_set_finder.h \
                   src/pme/ivc/correction_set_finder.cpp \
                   src/pme/ivc/caivc.h \
                   src/pme/ivc/caivc.cpp 

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

PME_TEST_LIBS = libpme.a \
                $(SAT_LIBS) \
                -lboost_unit_test_framework

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
        test_debug_transition_relation \
        test_variable_manager \
        test_cardinality_constraint \
        test_proof_checker \
        test_proof_minimizer \
        test_maxsat_solver \
        test_consecution_checker \
        test_find_safe_mis \
        test_find_minimal_support \
        test_logger \
        test_clause_database \
        test_collapse_set_finder \
        test_inductive_trace \
        test_frame_solver \
        test_unsat_core_lifter \
        test_ic3_solver \
        test_ic3_debugger \
        test_bmc_solver \
        test_basics \
        test_correction_set_finder \
        test_ivc

check_PROGRAMS = $(TESTS)

test_variable_manager_SOURCES = tests/test_variable_manager.cpp
test_variable_manager_LDADD = libpme.a \
                              -lboost_unit_test_framework


test_sat_adaptor_SOURCES = tests/test_sat_adaptor.cpp  
test_sat_adaptor_LDADD = $(PME_TEST_LIBS)


test_sat_SOURCES = tests/test_sat.cpp  
test_sat_LDADD = $(SAT_LIBS) \
                 -lboost_unit_test_framework

test_transition_relation_SOURCES = tests/test_transition_relation.cpp
test_transition_relation_LDADD = $(PME_TEST_LIBS) 

test_debug_transition_relation_SOURCES = tests/test_debug_transition_relation.cpp
test_debug_transition_relation_LDADD = $(PME_TEST_LIBS) 

test_cardinality_constraint_SOURCES = tests/test_cardinality_constraint.cpp
test_cardinality_constraint_LDADD = $(PME_TEST_LIBS) 

test_proof_checker_SOURCES = tests/test_proof_checker.cpp  
test_proof_checker_LDADD = $(PME_TEST_LIBS) 

test_proof_minimizer_SOURCES = tests/test_proof_minimizer.cpp  
test_proof_minimizer_LDADD = $(PME_TEST_LIBS)

test_maxsat_solver_SOURCES = tests/test_maxsat_solver.cpp  
test_maxsat_solver_LDADD = $(PME_TEST_LIBS)

test_consecution_checker_SOURCES = tests/test_consecution_checker.cpp  
test_consecution_checker_LDADD = $(PME_TEST_LIBS)

test_find_safe_mis_SOURCES = tests/test_find_safe_mis.cpp  
test_find_safe_mis_LDADD = $(PME_TEST_LIBS)

test_find_minimal_support_SOURCES = tests/test_find_minimal_support.cpp  
test_find_minimal_support_LDADD = $(PME_TEST_LIBS)

test_logger_SOURCES = tests/test_logger.cpp
test_logger_LDADD = $(PME_TEST_LIBS)

test_clause_database_SOURCES = tests/test_clause_database.cpp
test_clause_database_LDADD = $(PME_TEST_LIBS)

test_collapse_set_finder_SOURCES = tests/test_collapse_set_finder.cpp
test_collapse_set_finder_LDADD = $(PME_TEST_LIBS)

test_inductive_trace_SOURCES = tests/test_inductive_trace.cpp
test_inductive_trace_LDADD = $(PME_TEST_LIBS)

test_frame_solver_SOURCES = tests/test_frame_solver.cpp
test_frame_solver_LDADD = $(PME_TEST_LIBS)

test_unsat_core_lifter_SOURCES = tests/test_unsat_core_lifter.cpp
test_unsat_core_lifter_LDADD = $(PME_TEST_LIBS)

test_ic3_solver_SOURCES = tests/test_ic3_solver.cpp
test_ic3_solver_LDADD = $(PME_TEST_LIBS)

test_ic3_debugger_SOURCES = tests/test_ic3_debugger.cpp
test_ic3_debugger_LDADD = $(PME_TEST_LIBS)

test_bmc_solver_SOURCES = tests/test_bmc_solver.cpp
test_bmc_solver_LDADD = $(PME_TEST_LIBS)

test_basics_SOURCES = tests/test_basics.cpp
test_basics_LDADD = $(PME_TEST_LIBS)

test_ivc_SOURCES = tests/test_ivc.cpp
test_ivc_LDADD = $(PME_TEST_LIBS)

test_correction_set_finder_SOURCES = tests/test_correction_set_finder.cpp
test_correction_set_finder_LDADD = $(PME_TEST_LIBS)

SUBDIRS = src/sat/solvers/glucose src/sat/solvers/minisat 

EXTRA_DIST = LICENSE README.md glucose.supp

# Glucose can generate memcheck errors with certain compilers and flags
@VALGRIND_CHECK_RULES@
VALGRIND_SUPPRESSIONS_FILES = glucose.supp

