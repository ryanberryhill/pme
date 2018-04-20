AM_CPPFLAGS = -I$(srcdir)/src -Wall -Wpedantic -Werror 
AM_CXXFLAGS = -std=c++11

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
				   src/pme/engine/sat_adaptor.cpp 

libpme_a_LIBADD = libsat.a

libsat_a_SOURCES = src/sat/sat.h \
				   src/sat/sat.cpp

libsat_a_LIBADD = src/sat/solvers/glucose/libglucose.a

# No Werror because glucose headers cause several warnings
libsat_a_CPPFLAGS = -Wall -Wpedantic \
					-I$(srcdir)/src \
					-I$(srcdir)/src/sat/solvers/glucose/src 


bin_PROGRAMS = pme parseproof

pme_SOURCES = src/main/main.c \
			  src/aiger/aiger.h \
			  src/aiger/aiger.c

pme_LDADD = libpme.a \
			libsat.a \
			src/sat/solvers/glucose/libglucose.a \
			-lstdc++ -lm

parseproof_SOURCES = src/parseproof/main.cpp \
					 src/aiger/aiger.h \
					 src/aiger/aiger.c

TESTS = test_sat_adaptor \
		test_sat \
		test_transition_relation

check_PROGRAMS = test_sat_adaptor \
				 test_sat \
				 test_transition_relation
test_sat_adaptor_SOURCES = tests/test_sat_adaptor.cpp  
test_sat_adaptor_LDADD = libpme.a \
						 libsat.a \
						 src/sat/solvers/glucose/libglucose.a \
				 		 -lboost_unit_test_framework


test_sat_SOURCES = tests/test_sat.cpp  
test_sat_LDADD = libsat.a \
				 src/sat/solvers/glucose/libglucose.a \
				 -lboost_unit_test_framework

test_transition_relation_SOURCES = tests/test_transition_relation.cpp
test_transition_relation_LDADD = libpme.a \
						 		 libsat.a \
						         src/sat/solvers/glucose/libglucose.a \
						         -lboost_unit_test_framework


SUBDIRS = src/sat/solvers/glucose 

EXTRA_DIST = LICENSE README 

