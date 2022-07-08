#!/bin/bash
echo "Cantidad de Clientes A para instanciar:"
read n
for ((i = 0; i < n; i++ ))
do 
   xterm -hold -e ./bin/client_a localhost 8080 &
done
