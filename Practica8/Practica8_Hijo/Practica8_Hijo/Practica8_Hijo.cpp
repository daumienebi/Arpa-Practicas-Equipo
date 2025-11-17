/*
================================================================================
  PRÁCTICA 8: GESTIÓN DINÁMICA DE PROCESOS - PROCESO HIJO
  Universidad de Burgos - Escuela Politécnica Superior
================================================================================
  DESCRIPCIÓN:
  Programa hijo que es lanzado dinámicamente por el proceso padre.
  Se comunica con el padre mediante intercomunicadores.

  FUNCIONALIDAD:
  - Detecta que fue lanzado por un padre usando MPI_Comm_get_parent
  - Recibe mensaje de saludo del padre
  - Envía respuesta de confirmación al padre

  REQUISITOS:
  - Compilar como practica8_hijo.exe
  - Debe estar en el mismo directorio que practica8_padre.exe
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    int mirango, numprocs;
    MPI_Comm parent_comm;  // Comunicador con el padre
    MPI_Comm intracomm;    // Intracomunicador fusionado
    char mensaje_recibido[100];
    char mensaje_respuesta[100];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    /*
       MPI_Comm_get_parent: Obtiene el intercomunicador con el padre.
       Si este proceso fue lanzado por MPI_Comm_spawn, devuelve el
       intercomunicador. Si no tiene padre, devuelve MPI_COMM_NULL.
       Parámetros:
       - &parent_comm: Comunicador padre (salida)
    */
    MPI_Comm_get_parent(&parent_comm);
    if (parent_comm == MPI_COMM_NULL) {
        printf("[HIJO %d] ERROR: Este proceso no fue lanzado por un padre.\n", mirango);
        printf("[HIJO %d] Ejecuta primero 'practica8_padre.exe'\n", mirango);
        MPI_Finalize();
        return 1;
    }
    printf("[HIJO %d] Proceso hijo iniciado (total hijos: %d)\n", mirango, numprocs);
    // Fusionar intercomunicador en intracomunicador
    // Los hijos usan 1 para quedar después de los padres en el orden
    MPI_Intercomm_merge(parent_comm, 1, &intracomm);

    int nuevo_rango, nuevo_tamano;
    MPI_Comm_rank(intracomm, &nuevo_rango);
    MPI_Comm_size(intracomm, &nuevo_tamano);
    printf("[HIJO %d] Fusionado en intracomunicador. Nuevo rango: %d de %d\n",
        mirango, nuevo_rango, nuevo_tamano);
    // Recibir mensaje del padre
    // El padre tiene rango 0 en el intracomunicador
    MPI_Recv(mensaje_recibido, 100, MPI_CHAR, 0, 0, intracomm, MPI_STATUS_IGNORE);
    printf("[HIJO %d] Mensaje recibido del padre: \"%s\"\n", mirango, mensaje_recibido);
    // Enviar respuesta al padre
    sprintf_s(mensaje_respuesta, "Hijo %d (rango %d) confirma recepcion",
        mirango, nuevo_rango);
    MPI_Send(mensaje_respuesta, 100, MPI_CHAR, 0, 1, intracomm);
    printf("[HIJO %d] Respuesta enviada al padre.\n", mirango);
    // Sincronizar todos los hijos antes de que el hijo 0 envíe mensajes
    MPI_Barrier(intracomm);
    // EL HIJO DE MENOR RANGO (hijo 0) SALUDA AL RESTO DE HIJOS
    if (mirango == 0) {
        printf("\n[HIJO 0] Enviando saludos a los demas hijos...\n");
        char saludo_entre_hijos[100];
        sprintf_s(saludo_entre_hijos, "Hola desde el hijo de menor rango (hijo 0, rango %d)", nuevo_rango);
        // Enviar a todos los demás hijos
        for (int i = 1; i < numprocs; i++) {
            int rango_destino = nuevo_rango - mirango + i;  // Calcular rango en intracomm
            MPI_Send(saludo_entre_hijos, 100, MPI_CHAR, rango_destino, 2, intracomm);
            printf("[HIJO 0] -> Saludo enviado al hijo %d (rango %d)\n", i, rango_destino);
        }
    }
    else {
        // Los demás hijos reciben el saludo del hijo 0
        char saludo_recibido[100];
        int rango_hijo0 = nuevo_rango - mirango;  // Rango del hijo 0 en intracomm
        MPI_Recv(saludo_recibido, 100, MPI_CHAR, rango_hijo0, 2, intracomm, MPI_STATUS_IGNORE);
        printf("[HIJO %d] Saludo recibido de hijo 0: \"%s\"\n", mirango, saludo_recibido);
    }
    // Sincronizar antes de finalizar
    MPI_Barrier(intracomm); 
    printf("[HIJO %d] Finalizando...\n", mirango);
    MPI_Finalize();
    return 0;
}
