# Deploy this Node.js project to the public folder
NODE_PATH = $(HOME)/cmpt433/public/myApps/beatbox-server-copy
SOUNDS_PATH = $(HOME)/cmpt433/public/myApps/beatbox-wav-files
OUTFILE = beatbox
OUTDIR = $(HOME)/cmpt433/public/myApps
LFLAGS = -L$(HOME)/cmpt433/public/asound_lib_BBB -lasound

CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -pthread

all: deploy node_install beatbox

deploy:
	@echo 'COPYING THE NODE.JS FILES TO $(DEPLOY_PATH)'
	@echo ''
	mkdir -p $(NODE_PATH)
	chmod a+rwx $(NODE_PATH)
	mkdir -p $(SOUNDS_PATH)
	chmod a+rwx $(SOUNDS_PATH)
	cp -r node/* $(NODE_PATH)
	cp -r sounds/* $(SOUNDS_PATH)
	@echo 'Do not edit any files in this folder; they are copied!' > $(NODE_PATH)/DO_NOT_EDIT-FILES_COPIED.txt
	@echo ''
	@echo 'NOTE: On the host, in $(NODE_PATH), it is best to run: npm install'
	@echo '      Or, just run the node_install target in this makefile.'

node_install:
	cd $(NODE_PATH) && npm install

beatbox:
	$(CC_C) $(CFLAGS) beatbox.c getInput.c server.c audioMixer.c helper.c accelerometer.c -o $(OUTDIR)/$(OUTFILE) $(LFLAGS)

clean:
	rm $(OUTDIR)/$(OUTFILE)
	rm -rf $(NODE_PATH)
	rm -rf $(SOUNDS_PATH)
	
