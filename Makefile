build-daemon:
	clang -Wall -g -O2 -target bpf -D__TARGET_ARCH_x86 -c daemon/src/nhi.bpf.c -o nhi.bpf.o
	clang -Wall -c daemon/src/nhi.c -o nhi.o
	clang -Wall -c daemon/src/utils.c -o utils.o
	clang -Wall -c daemon/src/sqlite.c -o sqlite.o
	clang -Wall nhi.o utils.o sqlite.o -l:libbpf.a -lelf -lz -lsqlite3 -o nhid

build-test-daemon:
	clang -Wall -g -O2 -target bpf -D__TARGET_ARCH_x86 -c daemon/src/nhi.bpf.c -o nhi.bpf.o
	clang -Wall -c daemon/src/nhi.c -o nhi.o
	clang -Wall -c daemon/src/utils.c -o utils.o
	clang -Wall -D TEST -c daemon/src/sqlite.c -o sqlite.o
	clang -Wall nhi.o utils.o sqlite.o -l:libbpf.a -lelf -lz -lsqlite3 -o nhid

build-cli:
	go build -o nhi main.go

build-test-cli:
	go build -o nhi -tags TEST main.go

format:
	astyle --style=otbs --indent=spaces=2 *.c *.h --recursive && go fmt ./...

# copy-.nhi:
# 	cp -r .nhi/ ~/.nhi/

create-db:
	mkdir -p /var/nhi
	chmod 777 /var/nhi
	touch /var/nhi/db
	chmod 777 /var/nhi/db
	sqlite3 /var/nhi/db "CREATE TABLE IF NOT EXISTS meta (indicator INTEGER, name TEXT, start_time INTEGER, finish_time INTEGER);"

rm-db:
	rm /var/nhi/db

# install-sqlite-queue-service:
# 	mkdir -p ~/.local/share/systemd/user
# 	cp sqlite-queue.service ~/.local/share/systemd/user
# 	systemctl --user enable sqlite-queue.service

# start-sqlite-queue-service:
# 	systemctl --user start sqlite-queue.service

# install:
# 	cp bash-nhi /usr/local/bin
# 	cp lib/nhi.so /usr/lib
# 	cp nhi /usr/local/bin
