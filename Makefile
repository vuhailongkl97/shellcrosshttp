.PHONY: ss

INSTALL_PATH?=/usr/bin

ss:
	$(CC) main.c -o ss  -g

test:ss
	./ss

install: ss
	sudo cp ss form.html ${INSTALL_PATH}
