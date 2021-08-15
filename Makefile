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
              $(SRCDIR)LogFile.o $(SRCDIR)NetInfo.o $(SRCDIR)Tokens.o   \
              $(SRCDIR)Types.o   $(SRCDIR)Buffer.o  $(SRCDIR)IniFile.o  \
              $(SRCDIR)Lock.o    $(SRCDIR)Socket.o

INCDIR      = include/utils/
HEADERS     = $(INCDIR)BitMask.hpp       $(INCDIR)IniFile.hpp      \
              $(INCDIR)Lock.hpp          $(INCDIR)NetInfo.hpp      \
              $(INCDIR)Tokens.hpp        $(INCDIR)Staque.hpp       \
              $(INCDIR)Types.hpp         $(INCDIR)Buffer.hpp       \
              $(INCDIR)KeyValuePair.hpp  $(INCDIR)LogFile.hpp      \
              $(INCDIR)Socket.hpp        $(INCDIR)Thread.hpp       \
              $(INCDIR)Writable.hpp      $(INCDIR)Utils.hpp        \
              $(INCDIR)File.hpp          $(INCDIR)Lockable.hpp     \
              $(INCDIR)Readable.hpp

libutils.a: $(OBJECTS) $(HEADERS) Makefile
	ar rcs libutils.a $(OBJECTS)
	ranlib libutils.a

%.o : %.cpp $(HEADERS) Makefile
	$(CXX) $(CFLAGS) $(CLIBFLAGS) $(CXXFLAGS) $(INCLUDES) -o $@ -c $<

%: code/%.cpp $(HEADERS) libutils.a Makefile
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBPATHS) $(LIBS)

all: vic httpd

clean:
	-rm libutils.a
	-rm $(OBJECTS)
	-rm vic
	-rm httpd

