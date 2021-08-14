CFLAGS      = -O2 -pedantic \
              -W -Wall -Wextra -Wunused -Werror \
              -fstack-protector-all

CLIBFLAGS   = -fPIC
CXXFLAGS    = -std=c++17

INCLUDES    = -I./include/ -I/usr/local/include/

LIBS        = -lutils -lpthread
LIBPATHS    = -L./ -L/usr/local/lib/

SRCDIR      = code/utils/
OBJECTS     = $(SRCDIR)BitMask.o $(SRCDIR)File.o    $(SRCDIR)Lockable.o \
              $(SRCDIR)LogFile.o $(SRCDIR)NetInfo.o $(SRCDIR)String.o   \
              $(SRCDIR)Types.o   $(SRCDIR)Buffer.o  $(SRCDIR)IniFile.o  \
              $(SRCDIR)Lock.o    $(SRCDIR)Marker.o  $(SRCDIR)Socket.o   \
              $(SRCDIR)Tokens.o

INCDIR      = include/utils/
HEADERS     = $(INCDIR)BitMask.hpp       $(INCDIR)IniFile.hpp      \
              $(INCDIR)Lock.hpp          $(INCDIR)NetInfo.hpp      \
              $(INCDIR)Serializable.hpp  $(INCDIR)Staque.hpp       \
              $(INCDIR)Types.hpp         $(INCDIR)Buffer.hpp       \
              $(INCDIR)KeyValuePair.hpp  $(INCDIR)LogFile.hpp      \
              $(INCDIR)Primitive.hpp     $(INCDIR)Thread.hpp       \
              $(INCDIR)String.hpp        $(INCDIR)Utils.hpp        \
              $(INCDIR)File.hpp          $(INCDIR)Lockable.hpp     \
              $(INCDIR)Marker.hpp        $(INCDIR)Readable.hpp     \
              $(INCDIR)Socket.hpp        $(INCDIR)Tokens.hpp       \
              $(INCDIR)Writable.hpp

libutils.a: $(OBJECTS) $(HEADERS) Makefile
	ar rcs libutils.a $(OBJECTS)
	ranlib libutils.a

%.o : %.cpp $(HEADERS) Makefile
	$(CXX) $(CFLAGS) $(CLIBFLAGS) $(CXXFLAGS) $(INCLUDES) -o $@ -c $<

%: code/%.cpp $(HEADERS) libutils.a Makefile
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBPATHS) $(LIBS)

all: client server tests vic httpd

clean:
	-rm libutils.a
	-rm $(OBJECTS)
	-rm client
	-rm server
	-rm tests
	-rm vic
	-rm httpd

