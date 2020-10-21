
MOD_NAME = SampleImageProcessing
MOD_SRC  = $(MOD_NAME).cpp
MOD_OBJ  = $(MOD_SRC:.cpp=.o)

CXXFLAGS += -I/usr/local/include/opencv
LFLAGS = -L/usr/local/lib 
LIB_FILE = -lcxcore -lcv -lhighgui -lcvaux -lml

all: release

release: CFLAGS += -O3
release: $(MOD_NAME)

debug: CFLAGS += -g
debug: $(MOD_NAME)

$(MOD_NAME): $(MOD_OBJ)
	$(CC) $(LFLAGS) $(LIB_FILE) -o $@ $(MOD_OBJ) 

.cpp.o:
	$(CC) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(MOD_OBJ) $(MOD_NAME)

