PROJECT_HOME=$(HOME)/monitor/integrator
OBJDIR=$(PROJECT_HOME)/obj/dpresDir
SRCDIR=src

CXX=g++

LIBS= ../com/ehcom.a ../DpLib/DpLib.a -lpthread -lm -lz -lrt -lc
IPATH= -I../com/include

LIBPATH= 

INCLUDE = $(IPATH)
CFLAGS = $(INCLUDE) 

DEF=

#debug
CXXFLAGS= -g -Wall -ansi $(DEF) $(CFLAGS)  2>err

#release
#CXXFLAGS= -O -Wall -ansi $(DEF) $(CFLAGS)  2>err

LDFLAGS=$(LIBPATH) $(LIBS)

TARGET=dpres

OBJECTS = \
	$(OBJDIR)/EqMonitor.o \
	$(OBJDIR)/Main.o \
	
.SUFFIXES:
.SUFFIXES: .cpp .o

$(OBJDIR)/EqMonitor.o: EqMonitor.cpp 
	$(CXX) $(CXXFLAGS) -c -o $@ $?

$(OBJDIR)/Main.o: Main.cpp 
	$(CXX) $(CXXFLAGS) -c -o $@ $? 

	
all : all-am

all-am : $(TARGET)


$(TARGET) : $(OBJECTS)
	echo 'Making $(TARGET)'
	echo 'LDFLAGS $(LDFLAGS)'
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(TARGET) 2>lerr

install : 
	cp $(TARGET) ../../bin

clean : 
	rm $(OBJECTS) $(TARGET)
