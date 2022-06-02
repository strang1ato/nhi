DESTDIR =

build: build-daemon build-cli

build-daemon:
	clang -Wall -g -O2 -target bpf -D__TARGET_ARCH_x86 -c daemon/src/nhi.bpf.c -o nhi.bpf.o
	clang -Wall -c daemon/src/nhi.c -o nhi.o
	clang -Wall -c daemon/src/utils.c -o utils.o
	clang -Wall -c daemon/src/sqlite.c -o sqlite.o
	clang -Wall nhi.o utils.o sqlite.o -lbpf -lelf -lz -lsqlite3 -o nhid

build-cli:
	GOCACHE=/tmp/gocache bash -c 'go build -o nhi main.go'

install: install-nhid install-shells install-nhi install-db install-service

install-nhid:
	mkdir -p $(DESTDIR)/etc/nhi
	cp nhi.bpf.o $(DESTDIR)/etc/nhi
	chmod 755 nhid
	mkdir -p $(DESTDIR)/usr/bin
	cp nhid $(DESTDIR)/usr/bin

install-shells:
	mkdir -p $(DESTDIR)/etc/nhi
	cp shell/nhi.bash $(DESTDIR)/etc/nhi
	cp shell/nhi.zsh $(DESTDIR)/etc/nhi

install-nhi:
	chmod 755 nhi
	mkdir -p $(DESTDIR)/usr/bin
	cp nhi $(DESTDIR)/usr/bin

install-db:
	mkdir -p $(DESTDIR)/var/nhi
	chmod 777 $(DESTDIR)/var/nhi
	touch $(DESTDIR)/var/nhi/db
	chmod 777 $(DESTDIR)/var/nhi/db
	sqlite3 $(DESTDIR)/var/nhi/db "PRAGMA journal_mode=WAL; CREATE TABLE IF NOT EXISTS meta (indicator INTEGER, name TEXT, start_time INTEGER, finish_time INTEGER);"

install-service:
	mkdir -p $(DESTDIR)/etc/systemd/system
	cp nhid.service $(DESTDIR)/etc/systemd/system

build-test-daemon:
	clang -Wall -g -O2 -target bpf -D__TARGET_ARCH_x86 -c daemon/src/nhi.bpf.c -o nhi.bpf.o
	clang -Wall -c daemon/src/nhi.c -o nhi.o
	clang -Wall -c daemon/src/utils.c -o utils.o
	clang -Wall -D TEST -c daemon/src/sqlite.c -o sqlite.o
	clang -Wall nhi.o utils.o sqlite.o -lbpf -lelf -lz -lsqlite3 -o nhid

build-test-cli:
	go build -o nhi -tags TEST main.go

create-test-db:
	touch testing/db
	cat testing/original_db > testing/db

format:
	astyle --style=otbs --indent=spaces=2 *.c *.h --recursive --exclude=vendor && go fmt ./...
