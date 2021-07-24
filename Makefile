build-lib:
	cd lib/src/ && gcc nhi.c sqlite_queue_client.c -D_GNU_SOURCE -Wall -fPIC -ldl -pthread -lsqlite3 -shared -o ../nhi.so

build-cli:
	go build main.go

format:
	astyle --style=otbs --indent=spaces=2 *.c *.h --recursive && go fmt ./...

create-db:
	touch ${NHI_DB_PATH}
	sqlite3 ${NHI_DB_PATH} "CREATE TABLE IF NOT EXISTS meta (indicator INTEGER, name TEXT, start_time TEXT, finish_time TEXT);"

rm-db:
	rm ${NHI_DB_PATH}
