CFLAGS   = -std=c11 -Werror -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable $(foreach includedir,$(INCLUDES), -I$(includedir)) -O2
SOURCES  := .
INCLUDES := .
CC       := gcc
LD       := $(CC)
TARGET   := taskfinder
LIBS     := -ljansson -lcurl

CFILES := $(foreach cfiledir,$(SOURCES),$(wildcard $(cfiledir)/*.c))
OFILES := $(foreach cfile,$(CFILES),$(patsubst %.c,%.o,$(cfile)))

.PHONY: all
all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OFILES)
	$(LD) -o $(TARGET) $(^) $(LIBS)

clean:
	@echo cleaning...
	rm $(OFILES) $(TARGET) -rfv
