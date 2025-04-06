#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include "comun.h"

Usuario AsignarUsuario(int NumeroCuenta);
void *realizar_deposito(void *u);
void *realizar_retiro(void *u);
void *realizar_transferencia(void *u);
void *consultar_saldo(void *u);
void *Estado_banco(void *arg);
void Manejar_Ctr();
void EscribirTransacciones(int ID, const char tipo[], float cantidad);

char mensaje[100];

sem_t *semaforo, *semaforo_log;
pthread_t thread_deposito, thread_retiro, thread_transferencia;

int cerrar = 0;

int main(int argc, char *argv[])
{
    semaforo = sem_open("semaforo_cuentas", 0);
    semaforo_log = sem_open("semaforo_log", 0);

    // sem_post(semaforo);

    int numero_cuenta = atoi(argv[1]); // guardo ncuenta para operaciones

    pthread_t banco;
    pid_t pid_banco = atoi(argv[2]);
    pthread_create(&banco, NULL, Estado_banco, &pid_banco);

    Config configuracion = leer_configuracion("../Archivos_datos/config.txt");

    signal(SIGINT, Manejar_Ctr);

    int opcion;
    while (1)
    {
        system("clear");
        printf("1. Depósito\n2. Retiro\n3. Transferencia\n4. Consultar saldo\n5. Salir\n");
        printf("\n<----- ¿Que operación desea relaizar?: ");
        scanf("%d", &opcion);
        // while(getchar()!='\n');
        switch (opcion)
        {
        case 1:
            pthread_create(&thread_deposito, NULL, realizar_deposito, &numero_cuenta);
            pthread_join(thread_deposito, NULL);
            break;
        case 2:
            pthread_create(&thread_retiro, NULL, realizar_retiro, &numero_cuenta);
            pthread_join(thread_retiro, NULL);
            break;
        case 3:
            int resultado;
            resultado = pthread_create(&thread_transferencia, NULL, realizar_transferencia, &numero_cuenta);
            resultado = pthread_join(thread_transferencia, NULL);
            break;
        case 4:
            consultar_saldo(&numero_cuenta);
            break;
        case 5:
            exit(0);
        default:
            printf("Esa opcion no es posible, vuelva a intentarlo");
        }
        if (cerrar == 1)
        {
            char comando[200];
            snprintf(comando, sizeof(comando), "kill -9 %d", getpid());
            system(comando);
        }
    }
    sem_close(semaforo);
    sem_unlink("semaforo_cuentas");

    return 0;
}

void *realizar_deposito(void *u)
{

    sem_wait(semaforo_log);
    AgregarLog("Se esta esperando a que el semaforo para hacer un deposito");
    sem_post(semaforo_log);
    sem_wait(semaforo);

    int NumeroUsuario;

    NumeroUsuario = *(int *)u;
    Usuario usuario = AsignarUsuario(NumeroUsuario);

    Config configuracion = leer_configuracion("../Archivos_datos/config.txt");
    float deposito = 0;

    char linea[100];
    long linea_aux = -1;

    // Leo el archivo para sacar el saldo
    FILE *file = fopen(configuracion.archivo_cuentas, "r+");
    Usuario Aux;
    while (fgets(linea, sizeof(linea), file))
    {
        // Leemos todos los datos de la linea
        if (sscanf(linea, "%d,%49[^,],%f,%d", &Aux.numero_cuenta, Aux.titular, &Aux.saldo, &Aux.num_transacciones) == 4)
        {
            // Verificar si el nombre y la contraseña coinciden
            if (Aux.numero_cuenta == usuario.numero_cuenta)
            {
                linea_aux = ftell(file) - strlen(linea);
                break;
            }
        }
    }

    system("clear");
    printf("%s tiene un saldo de %f\n", usuario.titular, usuario.saldo);
    printf("\t¿Cuento dinero quiere depositar?: ");
    scanf(" %f", &deposito);
    while (getchar() != '\n')
        ;
    usuario.saldo += deposito;
    system("clear");
    printf("El deposito se ha realizado con exito.");
    printf("\nSaldo actual: %f", usuario.saldo);
    printf("\nVolviendo al menu...");
    // actualizo el archivo
    fseek(file, linea_aux, SEEK_SET);
    fprintf(file, "%d,%s,%.2f,%d", usuario.numero_cuenta, usuario.titular, usuario.saldo, usuario.num_transacciones);
    fclose(file);

    if (deposito != 0)
    {
        EscribirTransacciones(usuario.numero_cuenta, "deposito", deposito);
    }

    snprintf(mensaje, sizeof(mensaje), "Se ha realizado un deposito de: %f", deposito);
    sem_wait(semaforo_log);
    AgregarLog(mensaje);
    sem_post(semaforo_log);

    sem_wait(semaforo_log);
    AgregarLog("Se ha liberado el semaforo");
    sem_post(semaforo_log);
    sem_post(semaforo);
    sleep(4);
    system("clear");
}

void *realizar_retiro(void *u)
{
    sem_wait(semaforo_log);
    AgregarLog("Se esta esperando a que el semaforo para hacer un retiro");
    sem_post(semaforo_log);
    sem_wait(semaforo);

    int NumeroUsuario;

    NumeroUsuario = *(int *)u;
    Usuario usuario = AsignarUsuario(NumeroUsuario);

    Config configuracion = leer_configuracion("../Archivos_datos/config.txt");
    float retiro;

    char linea[100];
    long linea_aux = -1;

    // Leo el archivo para sacar el saldo
    FILE *file = fopen(configuracion.archivo_cuentas, "r+");
    Usuario Aux;
    while (fgets(linea, sizeof(linea), file))
    {
        // Leemos todos los datos de la linea
        if (sscanf(linea, "%d,%49[^,],%f,%d", &Aux.numero_cuenta, Aux.titular, &Aux.saldo, &Aux.num_transacciones) == 4)
        {
            // Verificar si el nombre y la contraseña coinciden
            if (Aux.numero_cuenta == usuario.numero_cuenta)
            {
                linea_aux = ftell(file) - strlen(linea);
                break;
            }
        }
    }

    system("clear");
    printf("%s tiene un saldo de %f\n", usuario.titular, usuario.saldo);
    printf("\t¿Cuento dinero quiere retirar?: ");
    scanf(" %f", &retiro);
    while (getchar() != '\n')
        ;
    if (retiro > usuario.saldo)
    {
        printf("No tiene suficiente dinero");
        fclose(file);

        sem_wait(semaforo_log);
        AgregarLog("Se ha liberado el semaforo");
        sem_post(semaforo_log);
        sem_post(semaforo);
        sem_wait(semaforo_log);
        AgregarLog("El usuario no tiene suficiente dinero");
        sem_post(semaforo_log);
    }
    else
    {
        retiro = retiro * (-1);
        usuario.saldo += retiro;
        system("clear");
        printf("El retiro se ha realizado con exito.");
        printf("\nSaldo actual: %f", usuario.saldo);
        printf("\nVolviendo al menu...");
        sleep(3);
        // actualizo el archivo
        fseek(file, linea_aux, SEEK_SET);
        fprintf(file, "%d,%s,%.2f,%d\n", usuario.numero_cuenta, usuario.titular, usuario.saldo, usuario.num_transacciones);
        fclose(file);

        if (retiro != 0)
        {
            EscribirTransacciones(usuario.numero_cuenta, "retiro", retiro);
        }

        sem_wait(semaforo_log);
        AgregarLog("Se ha liberado el semaforo");
        sem_post(semaforo_log);
        sem_post(semaforo);
        system("clear");

        snprintf(mensaje, sizeof(mensaje), "Se ha realizado un retiro de: %f", retiro);
        sem_wait(semaforo_log);
        AgregarLog(mensaje);
        sem_post(semaforo_log);
    }
}

void *realizar_transferencia(void *u)
{
    sem_wait(semaforo_log);
    AgregarLog("Se esta esperando a que el semaforo para hacer una transaccion");
    sem_post(semaforo_log);
    sem_wait(semaforo);

    Usuario Destino;

    int NumeroUsuario;

    NumeroUsuario = *(int *)u;
    Usuario usuario = AsignarUsuario(NumeroUsuario);

    Config configuracion = leer_configuracion("../Archivos_datos/config.txt");

    // Datos de la transferencia
    int NumeroCuenta = 0;
    float Cantidad = 0;

    sem_wait(semaforo_log);
    AgregarLog("Se esta abriendo el fichero para hacer una transaccion");
    sem_post(semaforo_log);

    // Leo el archivo para sacar el saldo
    FILE *file = fopen(configuracion.archivo_cuentas, "r+");

    printf("\nDima el numero de cuenta al que quieres hacer una transferencia: ");
    scanf("%d", &NumeroCuenta);
    printf("\nQue cantidad le quieres ingresar: ");
    scanf("%f", &Cantidad);

    if (Cantidad > usuario.saldo)
    {
        sem_wait(semaforo_log);
        AgregarLog("Saldo para la transaccion insuficiente");
        sem_post(semaforo_log);
        printf("La operacion supera su saldo.");

        sem_wait(semaforo_log);
        AgregarLog("Se ha liberado el semaforo");
        sem_post(semaforo_log);
        sem_post(semaforo);
        return 0;
    }

    long posicion_origen = -1, posicion_destino = -1;
    char linea[100];
    Usuario Aux;

    // Buscar las cuentas de origen y destino en el archivo.
    while (fgets(linea, sizeof(linea), file))
    {
        if (sscanf(linea, "%d,%49[^,],%f,%d", &Aux.numero_cuenta, Aux.titular, &Aux.saldo, &Aux.num_transacciones) == 4)
        {
            if (Aux.numero_cuenta == usuario.numero_cuenta)
            {
                posicion_origen = ftell(file) - strlen(linea);
            }
            else if (Aux.numero_cuenta == NumeroCuenta)
            {
                Destino = Aux;
                posicion_destino = ftell(file) - strlen(linea); // Guarda la posición de la línea.
            }
        }
    }

    // Verificar si se encontraron las cuentas.
    if (posicion_origen == -1 || posicion_destino == -1)
    {
        sem_wait(semaforo_log);
        AgregarLog("Algun usuario no se ha encontrado");
        sem_post(semaforo_log);
        fclose(file);
        return "-2";
    }

    // Actualizar los saldos y el número de transacciones.
    usuario.saldo -= Cantidad;
    usuario.num_transacciones++;
    Destino.saldo += Cantidad;
    Destino.num_transacciones++;

    // Escribir los datos actualizados en el archivo.
    fseek(file, posicion_origen, SEEK_SET);
    fprintf(file, "%d,%s,%.2f,%d\n", usuario.numero_cuenta, usuario.titular, usuario.saldo, usuario.num_transacciones);
    sem_wait(semaforo_log);
    AgregarLog("Se ha introducido en el archivo de usuarios el usuario con el nuevo saldo despues de la transaccion");
    sem_post(semaforo_log);

    fseek(file, posicion_destino, SEEK_SET);
    fprintf(file, "%d,%s,%.2f,%d\n", Destino.numero_cuenta, Destino.titular, Destino.saldo, Destino.num_transacciones);
    sem_wait(semaforo_log);
    AgregarLog("Se ha introducido en el archivo de usuarios el usuario con el nuevo saldo despues de la transaccion");
    sem_post(semaforo_log);

    fclose(file);

    if ((NumeroCuenta != 0) || (Cantidad != 0))
    {
        EscribirTransacciones(usuario.numero_cuenta, "transferencia", Cantidad);
    }

    sem_wait(semaforo_log);
    AgregarLog("Se ha liberado el semaforo");
    sem_post(semaforo_log);
    sem_post(semaforo);

    snprintf(mensaje, sizeof(mensaje), "Se ha realizado una transferencia de: %f", Cantidad);
    sem_wait(semaforo_log);
    AgregarLog(mensaje);
    sem_post(semaforo_log);

    return 0; // Transferencia exitosa.
}

void *consultar_saldo(void *u)
{

    char confirmacion;

    int NumeroUsuario;

    NumeroUsuario = *(int *)u;
    Usuario usuario = AsignarUsuario(NumeroUsuario);
    system("clear");
    printf("Tienes un saldo de %f\n", usuario.saldo);
    printf("¿Desea volver al menú?: ");
    scanf(" %c", &confirmacion);
    do
    {
        if (confirmacion == 's' || confirmacion == 'S')
        {
            break;
        }
        else if (confirmacion == 'N' || confirmacion == 'n')
        {
        }
        else
        {
            printf("Opción no válida, vuelva a intentarlo.");
        }
    } while (confirmacion != 'S' && confirmacion != 's' && confirmacion != 'N' && confirmacion != 'n');

    system("clear");
}

Usuario AsignarUsuario(int NumeroCuenta)
{

    char linea[100];

    char nombreArchivo[] = "../Archivos_datos/cuentas.dat";

    Config configuracion = leer_configuracion("../Archivos_datos/config.txt");

    sem_wait(semaforo_log);
    AgregarLog("Se esta leyendo el archivo de usuarios");
    sem_post(semaforo_log);

    sem_wait(semaforo_log);
    AgregarLog("Se ha abierto el fichero para la lectura del saldo de usuario.");
    sem_post(semaforo_log);

    FILE *file = fopen(nombreArchivo, "r");
    Usuario usuario;

    while (fgets(linea, sizeof(linea), file) != NULL)
    {
        if (sscanf(linea, "%d,%49[^,],%f,%d", &usuario.numero_cuenta, usuario.titular, &usuario.saldo, &usuario.num_transacciones) == 4)
        {
            if (usuario.numero_cuenta == NumeroCuenta)
            {
                fclose(file);
                return usuario;
            }
        }
    }
    fclose(file);
    printf("\nNo se ha encontrado el usuario\n");

    sem_wait(semaforo_log);
    AgregarLog("Se leyo el saldo correctamente");
    sem_post(semaforo_log);
}

void Manejar_Ctr()
{

    char confirmacion;
    int respuesta = 0;

    system("clear");
    printf("\n¿Seguro que quieres cerrar sesion ? (s/N): ");
    scanf(" %c", &confirmacion);
    do
    {
        if (confirmacion == 's' || confirmacion == 'S')
        {
            cerrar = 1;
            break;
        }
        else if (confirmacion == 'N' || confirmacion == 'n')
        {
            break;
        }
        else
        {
            system("clear");
            printf("Opción no válida, vuelva a intentarlo.");
            sleep(3);
            break;
        }
    } while (respuesta == 0);

    return;
}

void EscribirTransacciones(int ID, const char tipo[], float cantidad)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    FILE *file = fopen("../Archivos_datos/Transacciones.log", "a");
    fprintf(file, "%d;%d/%02d/%02d;%s;%f\n", ID, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tipo, cantidad);
    fclose(file);
}