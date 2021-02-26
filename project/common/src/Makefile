PROJECT_HOME=/home/ehkim/project
OBJDIR=$(PROJECT_HOME)/com/obj
SRCDIR=src
COM_INC=$(PROJECT_HOME)/com/include
INCLUDES = -I$(COM_INC) -I$(PROJECT_HOME)/com/include/com
SUBDIRS=unix

CXX=g++

DEF=-DLINUX
CFLAGS = $(INCLUDES) $(DEFS)
CXXFLAGS= -g -O2 -Wall -pthread  $(CFLAGS)  2>err

LIBNAME=ehcom.a

OBJECTS = \
	$(OBJDIR)/$(SUBDIRS)_mtsync.o \
	$(OBJDIR)/comDateTime.o \
	$(OBJDIR)/comFileUtil.o \
	$(OBJDIR)/comMisc.o \
	$(OBJDIR)/comLogger.o \
	$(OBJDIR)/comSTLogger.o \
	$(OBJDIR)/comEnv.o \
	$(OBJDIR)/comPath.o \
	$(OBJDIR)/comProfileutil.o \
	$(OBJDIR)/hash.o \

all: all-am

.SUFFIXES:
.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $^

$(OBJDIR)/$(SUBDIRS)_mtsync.o : $(COM_INC)/com/$(SUBDIRS)/$(SUBDIRS)_mtsync.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/comMisc.o : comMisc.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/comDateTime.o : comDateTime.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/comFileUtil.o : comFileUtil.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/comLogger.o : comLogger.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/comSTLogger.o : comSTLogger.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/comEnv.o : comEnv.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/comPath.o : comPath.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/comProfileutil.o : comProfileutil.c
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/hash.o : hash.c
	$(CXX) $(CXXFLAGS) -c -o $@ $?

all-am : $(LIBNAME)


$(LIBNAME) : $(OBJECTS)
	ar -vq $(LIBNAME)  $(OBJECTS) 	


install : 
	cp $(LIBNAME) $(PROJECT_HOME)/lib

clean : 
	rm $(OBJECTS) $(LIBNAME)
