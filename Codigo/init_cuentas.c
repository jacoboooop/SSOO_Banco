#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct Cuenta {
    int numero_cuenta;
    char titular[50];
    float saldo;
    int num_transacciones;
};

int main(){

    char nombre_archivo[] = "../Archivos_datos/cuentas.dat";

    FILE *file = fopen(nombre_archivo, "w");

    struct Cuenta cuentas;

    for (int i = 1; i < 6; i++){
        cuentas.numero_cuenta = i+1000;
        sprintf(cuentas.titular, "usuario%d", i);
        fprintf(file, "%d,%s,500,0\n", cuentas.numero_cuenta, cuentas.titular);
    }

    fclose(file);
    printf("Archivo de cuentas inicializado correctamente.\n");   

    return 0; 

}