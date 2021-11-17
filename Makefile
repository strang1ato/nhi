build: build-daemon build-cli

build-daemon:
	clang -Wall -g -O2 -target bpf -D__TARGET_ARCH_x86 -c daemon/src/nhi.bpf.c -o nhi.bpf.o
	clang -Wall -c daemon/src/nhi.c -o nhi.o
	clang -Wall -c daemon/src/utils.c -o utils.o
	clang -Wall -c daemon/src/sqlite.c -o sqlite.o
	clang -Wall nhi.o utils.o sqlite.o -lbpf -lelf -lz -lsqlite3 -o nhid

build-cli:
	go build -o nhi main.go

install: install-nhid install-nhi install-db

install-nhid:
	mkdir -p /etc/nhi
	cp nhi.bpf.o /etc/nhi
	chmod 755 nhid
	cp nhid /usr/bin

install-nhi:
	chmod 755 nhi
	cp nhi /usr/bin

install-db:
	mkdir -p /var/nhi
	chmod 777 /var/nhi
	touch /var/nhi/db
	chmod 777 /var/nhi/db
	sqlite3 /var/nhi/db "PRAGMA journal_mode=WAL; CREATE TABLE IF NOT EXISTS meta (indicator INTEGER, name TEXT, start_time INTEGER, finish_time INTEGER);"

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
	astyle --style=otbs --indent=spaces=2 *.c *.h --recursive && go fmt ./...
