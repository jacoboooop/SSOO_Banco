#ifndef COMUN_H
#define COMUN_H

typedef struct Usuario{
    int numero_cuenta;
    char titular[50];
    float saldo;
    int num_transacciones;
} Usuario;

typedef struct Config {
    int limite_retiro;
    int limite_transferencia;
    int umbral_retiros;
    int umbral_transferencias;
    int num_hilos;
    char archivo_cuentas[50];
    char archivo_log[50];
} Config;

#define FIFO_BANCO_MONITOR "/tmp/fifo_banco_monitor"

Config leer_configuracion(const char *ruta);

void AgregarLog(const char *operacion);

void* Estado_banco(void* arg);

#endif

