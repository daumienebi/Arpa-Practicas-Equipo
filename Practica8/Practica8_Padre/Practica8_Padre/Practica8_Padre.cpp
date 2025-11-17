/*
================================================================================
  PRÁCTICA 8: GESTIÓN DINÁMICA DE PROCESOS - PROCESO PADRE
  Universidad de Burgos - Escuela Politécnica Superior
================================================================================
  DESCRIPCIÓN:
  Programa padre que lanza dinámicamente procesos hijo usando MPI_Comm_spawn.
  Demuestra comunicación entre procesos padre-hijo mediante intercomunicadores.
  
  FUNCIONALIDAD:
  - Solicita número de procesos hijo a lanzar
  - Usa MPI_Comm_spawn para crear procesos hijo dinámicamente
  - Envía mensaje de saludo a todos los hijos
  - Recibe confirmación de cada hijo

  REQUISITOS:
  - Compilar como practica8_padre.exe
  - El ejecutable practica8_hijo.exe debe estar en el mismo directorio
================================================================================
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int mirango, numprocs;
    int num_hijos;
    MPI_Comm intercomm;  // Intercomunicador para comunicación padre-hijo
    MPI_Comm intracomm;  // Intracomunicador (padre + hijos juntos)
    char mensaje_saludo[100];
    char mensaje_respuesta[100];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    printf("  PRACTICA 8: GESTION DINAMICA DE PROCESOS - PROCESO PADRE\n");
    printf("[PADRE] Proceso padre iniciado (rango %d de %d)\n\n", mirango, numprocs);
    // Solicitar número de procesos hijo
    if (mirango == 0) {
        int valido = 0;
        while (!valido) {
            printf("Introduce el numero de procesos hijo a lanzar (1-10): ");
            fflush(stdout);
            if (scanf_s("%d", &num_hijos) != 1) {
                while (getchar() != '\n');
                printf("ERROR: Entrada invalida.\n\n");
                continue;
            }
            if (num_hijos < 1 || num_hijos > 10) {
                printf("ERROR: El numero debe estar entre 1 y 10.\n\n");
            } else {
                valido = 1;
            }
        }
        printf("\n[PADRE] Lanzando %d procesos hijo...\n\n", num_hijos);
    }
    // Difundir número de hijos a todos los procesos padre (si hay más de uno)
    MPI_Bcast(&num_hijos, 1, MPI_INT, 0, MPI_COMM_WORLD);
    /*
       MPI_Comm_spawn: Lanza procesos hijo dinámicamente
       Parámetros:
       - "practica8_hijo.exe": Nombre del ejecutable hijo
       - MPI_ARGV_NULL: Sin argumentos de línea de comandos
       - num_hijos: Número de procesos hijo a crear
       - MPI_INFO_NULL: Sin información de ubicación específica
       - 0: Rango del proceso raíz en MPI_COMM_WORLD
       - MPI_COMM_WORLD: Comunicador padre
       - &intercomm: Intercomunicador creado (salida)
       - MPI_ERRCODES_IGNORE: Ignorar códigos de error individuales
    */
    int resultado = MPI_Comm_spawn((char*)"C:\\Users\\ddsak\\Desktop\\practica8_hijo.exe",MPI_ARGV_NULL,num_hijos,MPI_INFO_NULL,
                    0,MPI_COMM_WORLD,&intercomm,MPI_ERRCODES_IGNORE);
    if (resultado != MPI_SUCCESS) {
        printf("[PADRE] ERROR: No se pudieron lanzar los procesos hijo.\n");
        printf("[PADRE] Verifica que 'practica8_hijo.exe' este en el mismo directorio.\n");
        MPI_Finalize();
        return 1;
    }
    printf("[PADRE] Procesos hijo lanzados exitosamente.\n\n");
    /*
       MPI_Intercomm_merge: Convierte intercomunicador en intracomunicador
       Esto permite comunicación más flexible entre padre e hijos.
       Parámetros:
       - intercomm: Intercomunicador a fusionar
       - 0: Orden de fusión (0 = padres primero, 1 = hijos primero)
       - &intracomm: Nuevo intracomunicador (salida)
    */
    MPI_Intercomm_merge(intercomm, 0, &intracomm);
    int nuevo_rango, nuevo_tamano;
    MPI_Comm_rank(intracomm, &nuevo_rango);
    MPI_Comm_size(intracomm, &nuevo_tamano);

    printf("[PADRE] Intercomunicador fusionado.\n");
    printf("[PADRE] Nuevo rango: %d, Nuevo tamaño total: %d procesos\n\n", 
           nuevo_rango, nuevo_tamano);
    // Enviar mensaje de saludo a todos los hijos
    if (mirango == 0) {
        sprintf_s(mensaje_saludo, "Hola desde el proceso padre (rango %d)", mirango);
        printf("[PADRE] Enviando mensajes de saludo a los hijos...\n");
        for (int i = 0; i < num_hijos; i++) {
            // Los hijos tienen rangos consecutivos después de los padres en intracomm
            int rango_hijo = numprocs + i;
            MPI_Send(mensaje_saludo, 100, MPI_CHAR, rango_hijo, 0, intracomm);
            printf("[PADRE] -> Mensaje enviado al hijo con rango %d\n", rango_hijo);
        }
        printf("\n[PADRE] Esperando respuestas de los hijos...\n");
        // Recibir confirmación de cada hijo
        for (int i = 0; i < num_hijos; i++) {
            int rango_hijo = numprocs + i;
            MPI_Recv(mensaje_respuesta, 100, MPI_CHAR, rango_hijo, 1, 
                     intracomm, MPI_STATUS_IGNORE);
            printf("[PADRE] <- Respuesta recibida: \"%s\"\n", mensaje_respuesta);
        }
        printf("\n[PADRE] Todas las comunicaciones completadas exitosamente.\n");
    }
    // Sincronizar antes de finalizar
    MPI_Barrier(intracomm);

    if (mirango == 0) {
        printf("RESUMEN:\n");
        printf("  - Procesos padre: %d\n", numprocs);
        printf("  - Procesos hijo lanzados: %d\n", num_hijos);
        printf("  - Total de procesos: %d\n", nuevo_tamano);
        printf("  - Comunicacion padre-hijo: Exitosa\n");
    }
    MPI_Finalize();
    return 0;
}