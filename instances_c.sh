#!/bin/bash
echo "Cantidad de Clientes C para instanciar:"
read n
for ((i = 0; i < n; i++ ))
do 
   xterm -hold -e ./bin/client_c localhost 8082 download$i.db &
done
