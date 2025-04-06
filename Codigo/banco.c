#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "comun.h"
#include <pthread.h>

#define MAX_LINE_LENGTH 256

void menu();
void RegistrarCuenta(const char *nombreArchivo, const char *nombre_input);
void LoginCuenta(int numeroCuenta);
void init_cuentas(const char *nombre_archivo);
bool VerificarCuenta(char nombre[], int numero, const char *nombreArchivo);
void Manejar_Ctrl();
void *escucharPipe(void *arg);

Config configuracion;

Usuario usuario;
sem_t *semaforo, *semaforo_log;

int main()
{


    semaforo = sem_open("semaforo_cuentas", O_CREAT, 0644, 1);
    semaforo_log = sem_open("semaforo_log", O_CREAT, 0644, 1);
    semaforo_log = sem_open("semaforo_log", 0);

    sem_wait(semaforo_log);
    AgregarLog("EL banco se ha iniciado");
    sem_post(semaforo_log);

    sem_wait(semaforo_log);
    AgregarLog("Se crean los semaforos de log y cuentas");
    sem_post(semaforo_log);

    system("clear");

    int valor = sem_getvalue(semaforo, &valor);
    if (valor == 0)
    {
        sem_wait(semaforo_log);
        AgregarLog("El semaforo no se cerro correctamente");
        sem_post(semaforo_log);
        sem_post(semaforo);
    }

    signal(SIGINT, Manejar_Ctrl);

    configuracion = leer_configuracion("../Archivos_datos/config.txt");

    //*******************************inicializo el monitor***********************************
    /*if (mkfifo(FIFO_BANCO_MONITOR, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Error al crear la FIFO");
            exit(1);
        }
    }*/

    sem_wait(semaforo_log);
    AgregarLog("Creo el pipe y el proceso monitor");
    sem_post(semaforo_log);

    pthread_t hiloPipe; // creo un hilo para el pipe
    int fd[2];
    pipe(fd);
    pid_t pid = pid = fork();

    char mensaje[250];
    pid_t pid_banco = getpid();
    if (pid < 0)
    {
        sem_wait(semaforo_log);
        AgregarLog("Error al iniciar el monitor");
        sem_post(semaforo_log);
        perror("Error al crear el monitor");
        exit(1);
    }
    else if (pid == 0)
    {

        sem_wait(semaforo_log);
        AgregarLog("Proceso monitor creado correctamente");
        sem_post(semaforo_log);
        // hijo
        close(fd[0]); // Cierra lectura
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]); // Ya está duplicado, cerramos
        snprintf(mensaje, sizeof(mensaje),"%d", pid_banco);
        execl("./monitor", "monitor",mensaje, NULL);
        perror("Error en exec de monitor");
        exit(1);
    }
    else
    {
        // padre
        /*
        int fifo_fd = open(FIFO_BANCO_MONITOR, O_RDONLY);
        if (fifo_fd == -1) {
            perror("Error al abrir la FIFO en banco");
            exit(1);
        }
        char mensaje[256];
        while (read(fifo_fd, mensaje, sizeof(mensaje)) > 0) {
            printf("%s\n", mensaje);
        }
        close(fifo_fd);
        */

        close(fd[1]); // solo va a leer
        pthread_create(&hiloPipe, NULL, escucharPipe, &fd);
        sem_wait(semaforo_log);
        AgregarLog("Se ha iniciado el menu");
        sem_post(semaforo_log);
        menu(configuracion.archivo_cuentas);
        close(fd[0]);
    }

    sem_close(semaforo);
    sem_unlink("semaforo_cuentas");

    return (0);
}

void menu(const char *nombreArchivo)
{
    system("clear");
    int caso;
    printf("1. Login\n2. Registeer\n3. Salir\n");
    printf("\n<---¿Que desea hacer?: ");
    scanf("%d", &caso);
    char NombreUsuario[50];
    int NumeroCuenta;

    switch (caso)
    {
    case 1:
        sem_wait(semaforo_log);
        AgregarLog("Se esta intentando hacer login");
        sem_post(semaforo_log);
        // Proceso de login
        printf("_______________________________________________\n");
        printf("\nNombre de usuario: ");
        scanf("%s", NombreUsuario);
        printf("\nNumeroCuenta: ");
        scanf("%d", &NumeroCuenta);
        if (VerificarCuenta(NombreUsuario, NumeroCuenta, nombreArchivo))
        {
            LoginCuenta(NumeroCuenta);
            AgregarLog("Se ha hecho login");
        }
        else
        {
            system("clear");
            sem_wait(semaforo_log);
            AgregarLog("El usuario no existe");
            sem_post(semaforo_log);
            printf("\nEl usuario no existe\n");
            sleep(3);
        }
        menu(nombreArchivo);
        break;
    case 2:
        sem_wait(semaforo_log);
        AgregarLog("Se esta registrando un nuevo usuario");
        sem_post(semaforo_log);
        // registro
        printf("\nNombre de usuario: ");
        scanf("%s", NombreUsuario);
        RegistrarCuenta(nombreArchivo, NombreUsuario);
        sem_wait(semaforo_log);
        AgregarLog("Se ha registrado un nuevo usuario");
        sem_post(semaforo_log);
        int NumeroCuenta;
        FILE *file = fopen(nombreArchivo, "r");
        char linea[MAX_LINE_LENGTH];
        Usuario usuario;
        while (fgets(linea, sizeof(linea), file))
        {
            // Leemos todos los datos de la linea
            if (sscanf(linea, "%d,%49[^,],%f,%d", &usuario.numero_cuenta, usuario.titular, &usuario.saldo, &usuario.num_transacciones) == 4)
            {
                // Verificar si el nombre y la contraseña coinciden
                if (strcmp(usuario.titular, NombreUsuario) == 0)
                {
                    NumeroCuenta = usuario.numero_cuenta;
                    fclose(file);
                }
            }
        }
        sem_wait(semaforo_log);
        AgregarLog("Se ha hecho login");
        sem_post(semaforo_log);
        LoginCuenta(NumeroCuenta);
        menu(nombreArchivo);
        break;
    case 3:
        Manejar_Ctrl();
        return;
    default:
        sem_wait(semaforo_log);
        AgregarLog("Se ha introducido un parametor incorrecto");
        sem_post(semaforo_log);
        printf("\nOpción no válida. Inténtalo de nuevo.\n");
        break;
    }
}

void LoginCuenta(int numeroCuenta)
{

    pid_t pid_banco = getpid();
    pid_t pid2 = fork();

    if (pid2 < 0)
    {
        perror("Error al crear el usuario");
        exit(1);
    }
    else if (pid2 == 0)
    {
        sem_wait(semaforo_log);
        AgregarLog("Se crea el proceso para hacer login");
        sem_post(semaforo_log);
        sleep(1);
        char comando[250];
        snprintf(comando, sizeof(comando), "./usuario %d %d", numeroCuenta, pid_banco);
        execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", comando, NULL);
        perror("Error en exec");
        exit(1);
    }
    else
    {
        printf("Esperando al inicio de sesion...\n");
        wait(NULL);
        printf("\nAccediendo al usuairo...\n\n");
    }
}

void RegistrarCuenta(const char *nombreArchivo, const char *nombre_input)
{

    char linea[MAX_LINE_LENGTH];

    sem_wait(semaforo_log);
    AgregarLog("Se abre el ficho en modo lectura");
    sem_post(semaforo_log);
    FILE *Registro = fopen(nombreArchivo, "r");
    int contador = 0;

    // Leer el archivo línea por línea
    while (fgets(linea, sizeof(linea), Registro))
    {
        // Leemos todos los datos de la linea
        contador++;
    }
    fclose(Registro);

    sem_wait(semaforo_log);
    AgregarLog("Se abre el ficheoro en modo escritura");
    sem_post(semaforo_log);
    FILE *Registro1 = fopen(nombreArchivo, "a");

    fprintf(Registro1, "%d,%s,500,0\n", contador + 1001, nombre_input);
    fclose(Registro1);
}

bool VerificarCuenta(char nombre[], int numero, const char *nombreArchivo)
{
    sem_wait(semaforo_log);
    AgregarLog("Se abre el fichero en modo lectura");
    sem_post(semaforo_log);
    FILE *file = fopen(nombreArchivo, "r");
    char linea[MAX_LINE_LENGTH];
    Usuario usuario;
    while (fgets(linea, sizeof(linea), file) != NULL)
    {
        // Leemos todos los datos de la linea
        if (sscanf(linea, "%d,%49[^,],%f,%d", &usuario.numero_cuenta, usuario.titular, &usuario.saldo, &usuario.num_transacciones) == 4)
        {
            // Verificar si el nombre y la contraseña coinciden
            if (strcmp(usuario.titular, nombre) == 0 && (usuario.numero_cuenta == numero))
            {
                fclose(file);
                return true;
            }
        }
    }
    fclose(file);
    return false;
}

void Manejar_Ctrl()
{

    sem_wait(semaforo_log);
    AgregarLog("El usuario a pulsado CTRL+C");
    sem_post(semaforo_log);

    semaforo = sem_open("semaforo_cuentas", 0);
    int valor;
    bool respuesta = true;
    char confirmacion = 0;

    sem_getvalue(semaforo, &valor);

    system("clear");
    printf("\n¿Seguro que desea apagar el sisema ? (s/N): ");
    scanf(" %c", &confirmacion);
    do
    {
        if (confirmacion == 's' || confirmacion == 'S')
        {
            while (1)
            {
                if (valor == 0)
                {
                    system("clear");
                    printf("Hay usuarios haciendo operaciones, espere....");
                    sem_wait(semaforo_log);
                    AgregarLog("El usuario esta haciendo alguna operacion.");
                    sem_post(semaforo_log);
                    sem_wait(semaforo);
                    char comando[200];
                    sem_wait(semaforo_log);
                    AgregarLog("El programa se ha APAGADO correctamente.");
                    sem_post(semaforo_log);
                    snprintf(comando, sizeof(comando), "kill -9 %d", getpid());
                    system(comando);
                }
                else
                {
                    sem_wait(semaforo_log);
                    AgregarLog("El programa se ha APAGADO correctamente.");
                    sem_post(semaforo_log);
                    char comando[200];
                    snprintf(comando, sizeof(comando), "kill -9 %d", getpid());
                    system(comando);
                }
            }
        }
        else if (confirmacion == 'N' || confirmacion == 'n')
        {
            menu(configuracion.archivo_cuentas);
            char comando[200];
            snprintf(comando, sizeof(comando), "kill -9 %d", getpid());
            system(comando);
            return;
        }
        else
        {
            system("clear");
            printf("Opción no válida, vuelva a intentarlo.");
            sleep(3);
            respuesta = false;
            break;
        }
    } while (respuesta == false);
}

/*
void * leerMensajes(void *arg)
{
    //Bloqueamos un hilo
    //sem_wait(semaforo_hilos);

    //Creamos todas las variables
    bool salir;
    int * cerrar;
    int fd_lectura;
    char mensaje[255];
    ssize_t bytes_leidos;
    pthread_t hilo_monitor,hilo_array,hilo_pila;

    //Hacemos un cast de los paremetros que por defecto es un void, pero en verdad es un char *
    cerrar = arg;

    salir = false;

    //Creamos un doble bucle por si alguien cierra el extremo de escritura que no se cierre el hilo y se vuelva a abrir el extremo
    while(1)
    {
        //Abrimos el extremo de lectura
        fd_lectura = open(FIFO_BANCO_MONITOR, O_RDONLY);

        //Este bucle se encarga de leer todo el rato
        while (true)
        {
            //en este caso queremos cerrar el hilo , por tanto activamos la variable para salir y salimos de este bucle
            //como el otro bucle cumprueba la variable al estar en true , saldremos de ambos acabando el hilo


            bytes_leidos = read(fd_lectura,mensaje,sizeof(mensaje));
            //Si se ha leido algo , en funcion del mensaje, se hace una cosa u otra
            if (bytes_leidos > 0)
            {
                mensaje[bytes_leidos] = '\0';

                if ( mensaje[0] == '0')
                {
                    pthread_create(&hilo_array,NULL,modifificarArrayProcesos,mensaje);
                }
                else if (mensaje[0] == '1')
                {

                    pthread_create(&hilo_pila,NULL,agregarPila,mensaje);

                }

                else if (strcmp(mensaje,"salir")==0 )
                {
                    //Cerramos el extremo de lectura ya que se ha acabdo el programa
                    close(fd_lectura);
                    //Avisamos de que hau un hilo libre
                    //sem_post(semaforo_hilos);
                    return NULL;
                }
                else pthread_create(&hilo_monitor,NULL,escribirLog,mensaje);

            }
            //Este caso se dara cuando alguien cierre el extremo de lectura, por lo que salimos del bucle
            //Pero como hay otro bucle, se volvera a abir el extremo de lectura
            else if (bytes_leidos == 0)
            {

                break;
            }

        }
    }

    return NULL;
}
*/

void *escucharPipe(void *arg)
{
    int fd = *(int *)arg;
    char buffer[256];
    while (1)
    {
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0)
        {
            buffer[bytes] = '\0';
            printf("BANCO RECIBIÓ: %s\n", buffer);
        }
        else
        {
            usleep(100000);
        }
    }
    return NULL;
}