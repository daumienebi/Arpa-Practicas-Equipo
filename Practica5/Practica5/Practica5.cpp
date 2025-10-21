/*
  PRÁCTICA 5: PROCESOS DE ENTRADA/SALIDA PARALELA
  Universidad de Burgos - Escuela Politécnica Superior
  Grado en Ingeniería Informática
  Arquitectura Paralela con MPI
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPETICIONES 10

int main(int argc, char* argv[]) {
    int mirango, numprocs;
    MPI_File fh;
    MPI_Status estado;
    MPI_Offset desplazamiento;

    char nombre_fichero[] = "salida_paralela.txt";

    char* buffer_escritura = NULL;
    char* buffer_lectura = NULL;   
    int tamanho_buffer;

    double tiempo_inicio = 0.0, tiempo_fin = 0.0;  // Inicializar ambos
    // INICIALIZACIÓN DE MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // Barrera para sincronizar antes de iniciar el timer
    MPI_Barrier(MPI_COMM_WORLD);
    tiempo_inicio = MPI_Wtime();  // Todos los procesos miden tiempo

    if (mirango == 0) {
        printf("  PRACTICA 5: ENTRADA/SALIDA PARALELA EN MPI\n");
        printf("Configuracion:\n");
        printf("  - Numero de procesos: %d\n", numprocs);
        printf("  - Fichero de salida: %s\n", nombre_fichero);
        printf("  - Repeticiones por proceso: %d\n", REPETICIONES);
        printf("  - Formato: ASCII editable\n\n");
    }

    // PREPARAR DATOS PARA ESCRITURA
    tamanho_buffer = REPETICIONES;
    buffer_escritura = (char*)malloc(tamanho_buffer * sizeof(char));
    buffer_lectura = (char*)malloc(tamanho_buffer * sizeof(char));

    // Verificación mejorada de memoria
    if (buffer_escritura == NULL || buffer_lectura == NULL) {
        fprintf(stderr, "[Proceso %d] ERROR: No se pudo asignar memoria para los buffers.\n",
            mirango);

        // Liberar memoria parcialmente asignada
        if (buffer_escritura != NULL) free(buffer_escritura);
        if (buffer_lectura != NULL) free(buffer_lectura);

        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    // Llenar el buffer con el rango convertido a ASCII
    for (int i = 0; i < REPETICIONES; i++) {
        if (mirango < 10) {
            buffer_escritura[i] = (char)(mirango + 48); // 0 -9
        }
        else {
            buffer_escritura[i] = (char)(mirango + 55); // A-Z
        }
    }
    // ABRIR EL FICHERO EN MODO ESCRITURA
    int resultado = MPI_File_open(MPI_COMM_WORLD,nombre_fichero,
        MPI_MODE_CREATE | MPI_MODE_WRONLY,MPI_INFO_NULL,&fh);

    if (resultado != MPI_SUCCESS) {
        if (mirango == 0) {
            fprintf(stderr, "ERROR: No se pudo abrir el fichero para escritura.\n");
        }
        free(buffer_escritura);
        free(buffer_lectura);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    // DEFINIR LA VISTA DEL FICHERO PARA CADA PROCESO
    desplazamiento = (MPI_Offset)(mirango * REPETICIONES);

    // Pasar "native" directamente sin cast
    resultado = MPI_File_set_view(fh,desplazamiento,MPI_CHAR,MPI_CHAR,(char*)"native",
        MPI_INFO_NULL);

    if (resultado != MPI_SUCCESS) {
        if (mirango == 0) {
            fprintf(stderr, "ERROR: No se pudo establecer la vista del fichero.\n");
        }
        MPI_File_close(&fh);
        free(buffer_escritura);
        free(buffer_lectura);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    // ESCRITURA PARALELA EN EL FICHERO
    resultado = MPI_File_write_at(fh,0,buffer_escritura,tamanho_buffer,
        MPI_CHAR,&estado);
    if (resultado != MPI_SUCCESS) {
        fprintf(stderr, "[Proceso %d] ERROR: Fallo en la escritura.\n", mirango);
        MPI_File_close(&fh);
        free(buffer_escritura);
        free(buffer_lectura);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    printf("[Proceso %d] Escritura completada: %d caracteres '%c' escritos.\n",
        mirango, REPETICIONES, buffer_escritura[0]);
    fflush(stdout);
    // CERRAR EL FICHERO DESPUES DE ESCRITURA
    MPI_File_close(&fh);
    // Sincronizar TODOS los procesos antes de continuar
    MPI_Barrier(MPI_COMM_WORLD);
    if (mirango == 0) {
        printf("\n--- Escritura completada. Iniciando lectura... ---\n\n");
        fflush(stdout);
    }
    // Segunda barrera para asegurar que el mensaje se imprime
    MPI_Barrier(MPI_COMM_WORLD);
    // ABRIR EL FICHERO EN MODO LECTURA
    resultado = MPI_File_open(MPI_COMM_WORLD,nombre_fichero,MPI_MODE_RDONLY,
        MPI_INFO_NULL,&fh);
    if (resultado != MPI_SUCCESS) {
        if (mirango == 0) {
            fprintf(stderr, "ERROR: No se pudo abrir el fichero para lectura.\n");
        }
        free(buffer_escritura);
        free(buffer_lectura);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    // DEFINIR LA VISTA PARA LECTURA
    resultado = MPI_File_set_view(fh,desplazamiento,MPI_CHAR,MPI_CHAR,
        (char*)"native",MPI_INFO_NULL);
    if (resultado != MPI_SUCCESS) {
        if (mirango == 0) {
            fprintf(stderr, "ERROR: No se pudo establecer la vista de lectura.\n");
        }
        MPI_File_close(&fh);
        free(buffer_escritura);
        free(buffer_lectura);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    // LECTURA PARALELA DEL FICHERO
    resultado = MPI_File_read_at(fh,0,buffer_lectura,tamanho_buffer,MPI_CHAR,&estado);
    if (resultado != MPI_SUCCESS) {
        fprintf(stderr, "[Proceso %d] ERROR: Fallo en la lectura.\n", mirango);
        MPI_File_close(&fh);
        free(buffer_escritura);
        free(buffer_lectura);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    // MOSTRAR LOS DATOS LEÍDOS DE FORMA ORDENADA
    for (int i = 0; i < numprocs; i++) {
        if (mirango == i) {
            printf("[Proceso %d] Lectura completada. Datos leidos: ", mirango);
            for (int j = 0; j < tamanho_buffer; j++) {
                printf("%c", buffer_lectura[j]);
            }
            printf("\n");
            fflush(stdout);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    // CERRAR EL FICHERO Y LIBERAR RECURSOS
    MPI_File_close(&fh);
    // Medir tiempo antes de la barrera final
    MPI_Barrier(MPI_COMM_WORLD);
    tiempo_fin = MPI_Wtime();
    if (mirango == 0) {
        printf("RESUMEN:\n");
        printf("  - Fichero generado: %s\n", nombre_fichero);
        printf("  - Tamaño total: %d bytes (%d procesos × %d caracteres)\n",
            numprocs * REPETICIONES, numprocs, REPETICIONES);
        printf("  - Contenido: Cada proceso escribio su rango %d veces\n", REPETICIONES);
        printf("  - Tiempo total: %.6f segundos\n", tiempo_fin - tiempo_inicio);
        printf("\nPuedes abrir '%s' con un editor de texto para ver el resultado.\n",nombre_fichero);
    }
    // Liberar memoria antes de finalizar
    free(buffer_escritura);
    free(buffer_lectura);
    MPI_Finalize();
    return 0;
}