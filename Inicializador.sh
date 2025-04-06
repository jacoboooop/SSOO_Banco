#!/bin/bash

cd Codigo/
gcc banco.c comun.c -o banco
gcc monitor.c comun.c -o monitor
gcc usuario.c comun.c -o usuario
cd ..
cd Archivos_datos
echo "" > cuentas.dat
echo ""> Registro.log
echo ""> Transacciones.log
cd ..
cd Codigo/
gcc init_cuentas.c -o init_cuentas
./init_cuentas
