#!/bin/bash
echo "Cantidad de Clientes B para instanciar:"
read n
for ((i = 0; i < n; i++ ))
do 
   xterm -hold -e ./bin/client_b localhost 8081 &
done
