
MOD_NAME = HelloWorld
MOD_SRC  = $(MOD_NAME).cpp
MOD_OBJ  = $(MOD_SRC:.cpp=.o)
WORKDIR = $(shell cd ../lib && pwd)

ifeq ($(shell uname), Darwin)
INC_PATH = \
	-I$(WORKDIR)/include/sapie \
	-I$(WORKDIR)/include/papi
LIB_FILE = $(WORKDIR)/lib/libsapie.so.3.0 $(WORKDIR)/lib/libpapi.so.3.0 -lcxcore -lcv -lhighgui -lcvaux -lml 
else
INC_PATH = \
	-I/usr/local/include/sapie \
	-I/usr/local/include/papi
LIB_FILE = /usr/local/lib/libsapie.so.3.0 /usr/local/lib/libpapi.so.3.0 -lcxcore -lcv -lhighgui -lcvaux -lml 
endif
CFLAGS = -Wall -c -Wno-deprecated $(INC_PATH)
LFLAGS = -Wl
LFLAGS = 

all: release

release: CFLAGS += -O3
release: $(MOD_NAME)

debug: CFLAGS += -g
debug: $(MOD_NAME)

$(MOD_NAME): $(MOD_OBJ)
	$(CC) $(LFLAGS) -o $@ $(MOD_OBJ) $(LIB_FILE)

.cpp.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(MOD_OBJ) $(MOD_NAME)

