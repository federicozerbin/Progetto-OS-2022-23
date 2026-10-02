CC = gcc
CFLAGS = -Wvla -Wall -Wextra -std=c99 -pthread -lrt

SUBDIRS = master atomo alimentazione inibitore attivatore

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean: 
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
