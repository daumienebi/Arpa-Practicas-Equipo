/*
================================================================================
  PRÁCTICA 7: TIPOS DE DATOS DERIVADOS
  Universidad de Burgos - Escuela Politécnica Superior
  Grado en Ingeniería Informática
================================================================================
  
  DESCRIPCIÓN:
  Este programa demuestra el uso de tipos de datos derivados en MPI para enviar
  estructuras complejas de datos no contiguos en memoria.
  
  ESCENARIO:
  - Proceso 0: Solicita tamaño N y crea una matriz N×N con números aleatorios
  - Proceso 1: Recibe la matriz triangular superior
  - Proceso 2: Recibe la matriz triangular inferior
  - Cada proceso muestra su matriz antes y después de recibir los datos
  
  TIPOS DE DATOS DERIVADOS UTILIZADOS:
  - MPI_Type_indexed: Crea tipos para elementos no contiguos
  - MPI_Type_commit: Registra el nuevo tipo en MPI
  - MPI_Type_free: Libera el tipo derivado
  
  REQUISITOS:
  - Mínimo 3 procesos
  - Compatible con Visual Studio + DeinoMPI
  - Tamaño N dinámico (especificado por usuario)
================================================================================
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Imprime una matriz N×N dinámica
void imprimir_matriz(int **matriz, int N) {
    for (int i = 0; i < N; i++) {
        printf("  ");
        for (int j = 0; j < N; j++) {
            printf("%4d ", matriz[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Reserva memoria para matriz N×N con almacenamiento contiguo
int** crear_matriz(int N) {
    int **matriz = (int **)malloc(N * sizeof(int *));
    int *datos = (int *)malloc(N * N * sizeof(int));
    
    if (matriz == NULL || datos == NULL) {
        return NULL;
    }
    // Configurar punteros de filas apuntando a posiciones contiguas
    for (int i = 0; i < N; i++) {
        matriz[i] = datos + i * N;
    }
    return matriz;
}

// Libera memoria de matriz creada con crear_matriz
void liberar_matriz(int **matriz) {
    if (matriz != NULL) {
        free(matriz[0]);  // Liberar datos contiguos
        free(matriz);     // Liberar array de punteros
    }
}

int main(int argc, char *argv[]) {
    int mirango, numprocs;
    int N;  // Tamaño de la matriz (dinámico)
    int **matriz_original = NULL;
    int **matriz_triangular = NULL;
    
    MPI_Datatype tipo_triangular_superior;
    MPI_Datatype tipo_triangular_inferior;

    // Inicialización
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // Validar número de procesos
    if (numprocs < 3) {
        if (mirango == 0) {
            printf("ERROR: Se requieren al menos 3 procesos.\n");
            printf("Ejecuta con: mpiexec -n 3 practica7.exe o agrega tres procesos en la interfaz de DeinoMPI\n");
        }
        MPI_Finalize();
        return 1;
    }

    // SOLICITAR TAMAÑO DE LA MATRIZ (PROCESO 0)
    if (mirango == 0) {
        printf("  PRACTICA 7: TIPOS DE DATOS DERIVADOS - MATRICES TRIANGULARES\n");
        // Bucle de validación para obtener N válido
        int tamano_valido = 0;
        while (!tamano_valido) {
            printf("Introduce el tamaño de la matriz N (N×N): ");
            fflush(stdout);
            
            if (scanf_s("%d", &N) != 1) {
                while (getchar() != '\n');
                printf("ERROR: Debes introducir un numero entero.\n\n");
                continue;
            }
            
            if (N <= 0) {
                printf("ERROR: El tamaño debe ser mayor que 0.\n\n");
            } else if (N > 20) {
                printf("ADVERTENCIA: Un tamaño muy grande puede ser dificil de visualizar.\n");
                printf("¿Continuar con N=%d? (1=Si, 0=No): ", N);
                int confirmar;
                if (scanf_s("%d", &confirmar) == 1 && confirmar == 1) {
                    tamano_valido = 1;
                } else {
                    printf("\n");
                }
            } else {
                tamano_valido = 1;
            }
        }
        
        printf("\nTamaño aceptado: %d×%d\n\n", N, N);
    }
    
    // Difundir el tamaño N a todos los procesos
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // CREAR MATRIZ Y DEFINIR TIPOS DERIVADOS
    if (mirango == 0) {
        printf("Configuracion:\n");
        printf("  - Tamaño de matriz: %d×%d\n", N, N);
        printf("  - Proceso 0: Inicializa matriz completa\n");
        printf("  - Proceso 1: Recibe triangular superior\n");
        printf("  - Proceso 2: Recibe triangular inferior\n\n");

        // Crear matriz dinámica
        matriz_original = crear_matriz(N);
        if (matriz_original == NULL) {
            printf("ERROR: No se pudo asignar memoria para la matriz.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Inicializar generador aleatorio
        srand((unsigned int)time(NULL));

        // Inicializar matriz con valores aleatorios
        printf("Inicializando matriz original...\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                matriz_original[i][j] = rand() % 100;  // Enteros de 0 a 99
            }
        }
        printf("%s\n", "MATRIZ ORIGINAL:");
        imprimir_matriz(matriz_original, N);

        // CREAR TIPO DERIVADO: TRIANGULAR SUPERIOR
        int num_elementos_superior = (N * (N + 1)) / 2;
        int *longitudes_superior = (int *)malloc(num_elementos_superior * sizeof(int));
        int *desplazamientos_superior = (int *)malloc(num_elementos_superior * sizeof(int));
        
        if (longitudes_superior == NULL || desplazamientos_superior == NULL) {
            printf("ERROR: No se pudo asignar memoria para los arrays de tipo derivado.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        int indice = 0;
        for (int i = 0; i < N; i++) {
            for (int j = i; j < N; j++) {  // j >= i (triangular superior)
                longitudes_superior[indice] = 1;
                desplazamientos_superior[indice] = i * N + j;
                indice++;
            }
        }

        MPI_Type_indexed(num_elementos_superior, 
                        longitudes_superior, 
                        desplazamientos_superior, 
                        MPI_INT, 
                        &tipo_triangular_superior);
        MPI_Type_commit(&tipo_triangular_superior);

        // CREAR TIPO DERIVADO: TRIANGULAR INFERIOR
        int num_elementos_inferior = (N * (N + 1)) / 2;
        int *longitudes_inferior = (int *)malloc(num_elementos_inferior * sizeof(int));
        int *desplazamientos_inferior = (int *)malloc(num_elementos_inferior * sizeof(int));
        
        if (longitudes_inferior == NULL || desplazamientos_inferior == NULL) {
            printf("ERROR: No se pudo asignar memoria.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        indice = 0;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j <= i; j++) {  // j <= i (triangular inferior)
                longitudes_inferior[indice] = 1;
                desplazamientos_inferior[indice] = i * N + j;
                indice++;
            }
        }

        MPI_Type_indexed(num_elementos_inferior, 
                        longitudes_inferior, 
                        desplazamientos_inferior, 
                        MPI_INT, 
                        &tipo_triangular_inferior);
        MPI_Type_commit(&tipo_triangular_inferior);

        // ENVIAR MATRICES TRIANGULARES
        printf("Enviando triangular superior al proceso 1...\n");
        MPI_Send(matriz_original[0], 1, tipo_triangular_superior, 1, 0, MPI_COMM_WORLD);

        printf("Enviando triangular inferior al proceso 2...\n\n");
        MPI_Send(matriz_original[0], 1, tipo_triangular_inferior, 2, 0, MPI_COMM_WORLD);

        // Liberar memoria
        free(longitudes_superior);
        free(desplazamientos_superior);
        free(longitudes_inferior);
        free(desplazamientos_inferior);
        MPI_Type_free(&tipo_triangular_superior);
        MPI_Type_free(&tipo_triangular_inferior);
        liberar_matriz(matriz_original);

        printf("Proceso 0 ha terminado el envio.\n");
    }

    // RECIBIR TRIANGULAR SUPERIOR
    else if (mirango == 1) {
        // Crear matriz dinámica
        matriz_triangular = crear_matriz(N);
        if (matriz_triangular == NULL) {
            printf("[Proceso 1] ERROR: No se pudo asignar memoria.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Inicializar con ceros
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                matriz_triangular[i][j] = 0;
            }
        }

        printf("[Proceso 1] Matriz ANTES de recibir datos:\n");
        printf("%s\n", "TRIANGULAR SUPERIOR (vacia):");
        imprimir_matriz(matriz_triangular, N);

        // Crear tipo derivado
        int num_elementos = (N * (N + 1)) / 2;
        int *longitudes = (int *)malloc(num_elementos * sizeof(int));
        int *desplazamientos = (int *)malloc(num_elementos * sizeof(int));
        
        if (longitudes == NULL || desplazamientos == NULL) {
            printf("[Proceso 1] ERROR: No se pudo asignar memoria.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        int indice = 0;
        for (int i = 0; i < N; i++) {
            for (int j = i; j < N; j++) {
                longitudes[indice] = 1;
                desplazamientos[indice] = i * N + j;
                indice++;
            }
        }

        MPI_Type_indexed(num_elementos, longitudes, desplazamientos, 
                        MPI_INT, &tipo_triangular_superior);
        MPI_Type_commit(&tipo_triangular_superior);

        // Recibir datos
        printf("[Proceso 1] Recibiendo triangular superior del proceso 0...\n\n");
        MPI_Recv(matriz_triangular[0], 1, tipo_triangular_superior, 
                 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf("[Proceso 1] Matriz DESPUES de recibir datos:\n");
        printf("%s\n", "TRIANGULAR SUPERIOR (recibida):");
        imprimir_matriz(matriz_triangular, N);

        free(longitudes);
        free(desplazamientos);
        MPI_Type_free(&tipo_triangular_superior);
        liberar_matriz(matriz_triangular);
    }

    // RECIBIR TRIANGULAR INFERIOR
    else if (mirango == 2) {
        // Crear matriz dinámica
        matriz_triangular = crear_matriz(N);
        if (matriz_triangular == NULL) {
            printf("[Proceso 2] ERROR: No se pudo asignar memoria.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        // Inicializar con ceros
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                matriz_triangular[i][j] = 0;
            }
        }

        printf("[Proceso 2] Matriz ANTES de recibir datos:\n");
        printf("%s\n", "MATRIZ ORIGINAL:");
        imprimir_matriz(matriz_triangular, N);

        // Crear tipo derivado
        int num_elementos = (N * (N + 1)) / 2;
        int *longitudes = (int *)malloc(num_elementos * sizeof(int));
        int *desplazamientos = (int *)malloc(num_elementos * sizeof(int));
        
        if (longitudes == NULL || desplazamientos == NULL) {
            printf("[Proceso 2] ERROR: No se pudo asignar memoria.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        int indice = 0;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j <= i; j++) {
                longitudes[indice] = 1;
                desplazamientos[indice] = i * N + j;
                indice++;
            }
        }

        MPI_Type_indexed(num_elementos, longitudes, desplazamientos, 
                        MPI_INT, &tipo_triangular_inferior);
        MPI_Type_commit(&tipo_triangular_inferior);

        // Recibir datos
        printf("[Proceso 2] Recibiendo triangular inferior del proceso 0...\n\n");
        MPI_Recv(matriz_triangular[0], 1, tipo_triangular_inferior, 
                 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf("[Proceso 2] Matriz DESPUES de recibir datos:\n");
        printf("%s\n", "TRIANGULAR INFERIOR (recibida):");
        imprimir_matriz(matriz_triangular, N);

        free(longitudes);
        free(desplazamientos);
        MPI_Type_free(&tipo_triangular_inferior);
        liberar_matriz(matriz_triangular);
    }
    // OTROS PROCESOS
    else {
        printf("[Proceso %d] No participa en este ejercicio.\n", mirango);
    }

    MPI_Finalize();
    return 0;
}