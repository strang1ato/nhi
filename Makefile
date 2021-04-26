format:
	astyle --style=otbs --indent=spaces=2 *.c *.h --recursive

build:
	cd lib/bash/src/ && gcc nhi.c sqlite.c -D_GNU_SOURCE -Wall -fPIC -ldl -lsqlite3 -shared -o ../nhi.so

create-db:
	touch test.db

rm-db:
	rm test.db
