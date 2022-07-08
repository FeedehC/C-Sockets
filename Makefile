CC = gcc
CFLAGS = -g -Wall -pedantic -Werror -Wextra -Wconversion -std=gnu11

#En caso de ejecutar "make" sin argumento se aplica este/estos target
all: folders server client_a client_b client_c #client-tcp client-udp client-tcp6
#version insert_data last_row_id select_all parameterized named_placeholders insert_image read_image column_names list_tables
reqs:
	sudo apt install sqlite3 -y
	sudo apt install libsqlite3-dev -y
	sudo apt install xterm -y
	sudo chmod +x ./instances_a.sh
	sudo chmod +x ./instances_b.sh
	sudo chmod +x ./instances_c.sh

#sudo apt install libzdb-dev -y

folders:
	mkdir -p ./bin ./obj

server: ./src/server.c utils.o
	$(CC) $(CFLAGS) -o ./bin/server ./src/server.c ./obj/utils.o -lsqlite3 -L/usr/lib -lzdb -I/usr/include/zdb

client_a: ./src/client_a.c utils.o
	$(CC) $(CFLAGS) -o ./bin/client_a ./src/client_a.c ./obj/utils.o

client_b: ./src/client_b.c utils.o
	$(CC) $(CFLAGS) -o ./bin/client_b ./src/client_b.c ./obj/utils.o

client_c: ./src/client_c.c utils.o
	$(CC) $(CFLAGS) -o ./bin/client_c ./src/client_c.c ./obj/utils.o

utils.o: ./src/utils.c
	$(CC) $(CFLAGS) -o ./obj/utils.o -c ./src/utils.c

clean: #Si se invoca "make clean" se elimina lo que aqui se indica
	rm -rf ./bin ./obj

clean_db:
	rm -rf *.db

########################################################################

version: ./src/version.c
	$(CC) $(CFLAGS) -o ./bin/version ./src/version.c -lsqlite3

insert_data: ./src/insert_data.c
	$(CC) $(CFLAGS) -o ./bin/insert_data ./src/insert_data.c -lsqlite3

last_row_id: ./src/last_row_id.c
	$(CC) $(CFLAGS) -o ./bin/last_row_id ./src/last_row_id.c -lsqlite3

select_all: ./src/select_all.c
	$(CC) $(CFLAGS) -o ./bin/select_all ./src/select_all.c -lsqlite3

parameterized: ./src/parameterized.c
	$(CC) $(CFLAGS) -o ./bin/parameterized ./src/parameterized.c -lsqlite3

named_placeholders: ./src/named_placeholders.c
	$(CC) $(CFLAGS) -o ./bin/named_placeholders ./src/named_placeholders.c -lsqlite3

insert_image: ./src/insert_image.c
	$(CC) $(CFLAGS) -o ./bin/insert_image ./src/insert_image.c -lsqlite3

read_image: ./src/read_image.c
	$(CC) $(CFLAGS) -o ./bin/read_image ./src/read_image.c -lsqlite3

column_names: ./src/column_names.c
	$(CC) $(CFLAGS) -o ./bin/column_names ./src/column_names.c -lsqlite3

list_tables: ./src/list_tables.c
	$(CC) $(CFLAGS) -o ./bin/list_tables ./src/list_tables.c -lsqlite3

client-tcp: ./src/client-tcp.c utils.o
	$(CC) $(CFLAGS) -o ./bin/client-tcp ./src/client-tcp.c ./obj/utils.o

client-udp: ./src/client-udp.c utils.o
	$(CC) $(CFLAGS) -o ./bin/client-udp ./src/client-udp.c ./obj/utils.o

client-tcp6: ./src/client-tcp6.c utils.o
	$(CC) $(CFLAGS) -o ./bin/client-tcp6 ./src/client-tcp6.c ./obj/utils.o

test: ./src/test.c 
	$(CC) $(CFLAGS) -o ./bin/test ./src/test.c -lsqlite3 -L/usr/lib -lzdb -I/usr/include/zdb

server_transfer: ./src/server_transfer.c utils.o
	$(CC) $(CFLAGS) -o ./bin/server_transfer ./src/server_transfer.c

