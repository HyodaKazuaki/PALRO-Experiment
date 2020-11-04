
MOD_NAME = HelloWorld
MOD_SRC  = $(MOD_NAME).cpp
MOD_OBJ  = $(MOD_SRC:.cpp=.o)

INC_PATH = \
	-I/usr/local/include/sapie \
	-I/usr/local/include/papi
CFLAGS = -Wall -c -Wno-deprecated $(INC_PATH)
LFLAGS = -Wl
LIB_FILE = /usr/local/lib/libsapie.so.3.0 /usr/local/lib/libpapi.so.3.0 -lcxcore -lcv -lhighgui -lcvaux -lml 

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

