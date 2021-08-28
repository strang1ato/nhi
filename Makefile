build-lib:
	cd lib/src/ && gcc nhi.c sqlite_queue_client.c -D_GNU_SOURCE -Wall -fPIC -ldl -pthread -lsqlite3 -shared -o ../nhi.so

build-cli:
	go build -o nhi main.go

format:
	astyle --style=otbs --indent=spaces=2 *.c *.h --recursive && go fmt ./...

copy-.nhi:
	cp -r .nhi/ ~/.nhi/

create-db:
	mkdir -p ~/.nhi
	touch ~/.nhi/db
	sqlite3 ~/.nhi/db "CREATE TABLE IF NOT EXISTS meta (indicator INTEGER, name TEXT, start_time TEXT, finish_time TEXT);"

rm-db:
	rm ~/.nhi/db

install-sqlite-queue-service:
	mkdir -p ~/.local/share/systemd/user
	cp sqlite-queue.service ~/.local/share/systemd/user
	systemctl --user enable sqlite-queue.service

start-sqlite-queue-service:
	systemctl --user start sqlite-queue.service

install:
	cp bash-nhi /usr/local/bin
	cp lib/nhi.so /usr/lib
	cp nhi /usr/local/bin
