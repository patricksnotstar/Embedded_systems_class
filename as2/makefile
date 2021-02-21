# Makefile for building embedded application.
# by Brian Fraser

OUTFILE = sorter
OUTDIR = $(HOME)/cmpt433/public/myApps

CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -pthread

all: app done

help: 
	@echo "Build Hello World program for BeagleBone"
	@echo "Targets include all, app, and clean."


app:
	$(CC_C) $(CFLAGS) sorter.c pothead.c networking.c -o $(OUTDIR)/$(OUTFILE)
	ls -l $(OUTDIR)/$(OUTFILE)

done: 
	@echo "Finished building application."

clean:
	rm $(OUTDIR)/$(OUTFILE)
