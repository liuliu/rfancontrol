LDFLAGS := -lnvidia-ml $(LDFLAGS)
CFLAGS := -O3 -Wall -I"/usr/local/cuda/include" $(CFLAGS)

TARGETS = rfancontrol

TARGET_SRCS := $(patsubst %,%.cc,$(TARGETS))

.PHONY: all clean dep

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS)

$(TARGETS): %: %.o
	$(CC) -o $@ $< $(LDFLAGS)

%.o: %.cc
	$(CC) $< -o $@ -c $(CFLAGS)

dep: .dep.mk
.dep.mk: $(TARGET_SRCS)
	$(CC) $(CFLAGS) -MM $^ > .dep.mk

-include .dep.mk
