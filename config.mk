# version
VERSION = 0.1

# paths
DESTDIR 	?= /tmp
DEP_PATH 	?= $(DESTDIR)

DEPS 		:= 

AR 			?= ar
ARFLAGS		:= -rcvs
CXX 		?=
CXXFLAGS 	:= -Wall -Werror -std=c++11 -pthread
DBGFLAGS 	:= -g -ggdb
TESTFLAGS 	:= -fprofile-arcs -ftest-coverage --coverage
LINKFLAGS 	:= -shared

COBJFLAGS 	:= $(CXXFLAGS) -c -fPIC
test: COBJFLAGS 	+= $(TESTFLAGS)
test: LINKFLAGS 	+= -fprofile-arcs -lgcov
debug: COBJFLAGS 	+= $(DBGFLAGS)
