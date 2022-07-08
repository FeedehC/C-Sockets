#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sqlite3.h>
#include <zdb/zdb.h>

int main() //int argc, char *argv[])
{
//Ejemplo libzdb
    URL_T url = URL_new("sqlite:///home/fedevm/Reposgit/soii---2022---laboratorio-ii-FeedehC/test.db?synchronous=normal&heap_limit=8000&foreign_keys=on");
    ConnectionPool_T pool = ConnectionPool_new(url); //Default 5 conexiones
    ConnectionPool_setInitialConnections(pool, 5); //Se indica que se inicia con 5 conexiones
    ConnectionPool_setMaxConnections(pool, 5); //Se indica el maximo de 5 conexiones
    ConnectionPool_start(pool);
    //[..]
    Connection_T con = ConnectionPool_getConnection(pool);
    ResultSet_T result = Connection_executeQuery(con, "SELECT * FROM Cars Where Id > %d", 4);
    while (ResultSet_next(result)) 
    {
        int id = ResultSet_getInt(result, 1);
        const char *car_name = ResultSet_getString(result, 2);
        int price = ResultSet_getInt(result, 3);

        printf("Id = %d\nMarca = %s\nPrecio = %d\n", id, car_name, price);
        //int blobSize;
        //const void *image = ResultSet_getBlob(result, 3, &blobSize);
        //[..]
    }
    Connection_close(con);
    //[..]
    ConnectionPool_free(&pool);
    URL_free(&url);
}