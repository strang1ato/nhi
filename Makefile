build-lib:
	cd lib/src/ && gcc nhi.c sqlite.c -D_GNU_SOURCE -Wall -fPIC -ldl -pthread -lsqlite3 -shared -o ../nhi.so

build-cli:
	go build main.go

format:
	astyle --style=otbs --indent=spaces=2 *.c *.h --recursive && go fmt ./...

create-db:
	touch test.db

rm-db:
	rm test.db
