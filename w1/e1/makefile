
MOD_NAME = SampleImageProcessing
MOD_SRC  = $(MOD_NAME).cpp
MOD_OBJ  = $(MOD_SRC:.cpp=.o)

ifeq ($(shell uname), Darwin)
CXXFLAGS += -I/usr/local/Cellar/opencv@2/2.4.13.7_11/include
LFLAGS = -L/usr/local/Cellar/opencv@2/2.4.13.7_11/lib
LIB_FILE = -lopencv_core -lopencv_highgui -lopencv_ml
else
CXXFLAGS += -I/usr/local/include/opencv
LIB_FILE = -lcxcore -lcv -lhighgui -lcvaux -lml
endif
LFLAGS += -L/usr/local/lib

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

