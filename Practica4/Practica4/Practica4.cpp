/*
================================================================================
  PRÁCTICA 4: TOPOLOGÍAS VIRTUALES - SUMA DE MATRICES M×N
  Universidad de Burgos - Escuela Politécnica Superior
  Grado en Ingeniería Informática
  Arquitectura Paralela con MPI
================================================================================

  DESCRIPCIÓN:
  Este programa realiza la suma de dos matrices M×N utilizando una topología
  cartesiana virtual en MPI. Cada proceso de la topología se encarga de sumar
  un elemento específico de las dos matrices según sus coordenadas cartesianas.

  TOPOLOGÍA:
  - Se crea una topología cartesiana 2D (filas × columnas)
  - Cada proceso tiene coordenadas [fila][columna]
  - El proceso en coordenadas (i,j) suma: C[i][j] = A[i][j] + B[i][j]

  REQUISITOS:
  - El número de procesos debe ser igual a M × N
  - Compatible con Visual Studio + DeinoMPI
  - Usa MPI_Cart_create y MPI_Cart_coords
================================================================================
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[]) {
    int mirango, numprocs;
    int FILAS, COLUMNAS;              // Dimensiones dinámicas de las matrices
    int ndims = 2;                    // Topología 2D (filas y columnas)
    int dims[2];                      // Dimensiones de la topología cartesiana (se definirán después)
    int periods[2] = { 0, 0 };          // No periódica en ninguna dimensión
    int reorder = 1;                  // Permitir reordenamiento de procesos
    int coords[2];                    // Coordenadas cartesianas [fila][columna]

    MPI_Comm comm_cart;               // Comunicador con topología cartesiana

    // Punteros para matrices dinámicas
    int** matrizA = NULL;
    int** matrizB = NULL;
    int** matrizC = NULL;

    // Elementos locales de cada proceso
    int elemento_A, elemento_B, elemento_C;

    double tiempo_inicio, tiempo_fin;

    // =========================================================================
    // FASE 1: INICIALIZACIÓN DE MPI
    // =========================================================================
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // =========================================================================
    // FASE 2: SOLICITAR DIMENSIONES DE LAS MATRICES (PROCESO 0)
    // =========================================================================
    if (mirango == 0) {
        printf("================================================================================\n");
        printf("  SUMA DE MATRICES M×N CON TOPOLOGIA CARTESIANA MPI\n");
        printf("================================================================================\n\n");
        printf("Numero de procesos disponibles: %d\n\n", numprocs);

        // Bucle de validación para obtener dimensiones válidas
        int dimensiones_validas = 0;
        while (!dimensiones_validas) {
            printf("Introduce el numero de FILAS (M): ");
            fflush(stdout);

            if (scanf_s("%d", &FILAS) != 1 || FILAS <= 0) {
                while (getchar() != '\n'); // Limpiar buffer
                printf("ERROR: Debes introducir un numero entero positivo.\n\n");
                continue;
            }

            printf("Introduce el numero de COLUMNAS (N): ");
            fflush(stdout);

            if (scanf_s("%d", &COLUMNAS) != 1 || COLUMNAS <= 0) {
                while (getchar() != '\n'); // Limpiar buffer
                printf("ERROR: Debes introducir un numero entero positivo.\n\n");
                continue;
            }

            // Validar que el producto sea igual al número de procesos
            if (FILAS * COLUMNAS != numprocs) {
                printf("\nERROR: El producto %d × %d = %d no coincide con el numero de procesos (%d)\n",
                    FILAS, COLUMNAS, FILAS * COLUMNAS, numprocs);
                printf("Intentalo de nuevo.\n\n");
            }
            else {
                dimensiones_validas = 1;
            }
        }

        printf("\nDimensiones aceptadas: %d × %d = %d procesos\n\n",
            FILAS, COLUMNAS, FILAS * COLUMNAS);
    }

    // Difundir las dimensiones a todos los procesos
    MPI_Bcast(&FILAS, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&COLUMNAS, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Configurar dimensiones de la topología cartesiana
    dims[0] = FILAS;
    dims[1] = COLUMNAS;

    // =========================================================================
    // FASE 3: CREACIÓN DE LA TOPOLOGÍA CARTESIANA
    // =========================================================================
    /*
       MPI_Cart_create: Crea un nuevo comunicador con topología cartesiana
       - MPI_COMM_WORLD: Comunicador original
       - ndims: Número de dimensiones (2 para matriz 2D)
       - dims: Array con el tamaño de cada dimensión [FILAS, COLUMNAS]
       - periods: Array indicando si cada dimensión es periódica (0=no, 1=sí)
       - reorder: Permite a MPI reordenar los procesos para optimización
       - comm_cart: Nuevo comunicador cartesiano creado
    */
    MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, reorder, &comm_cart);

    // Obtener el nuevo rango en el comunicador cartesiano
    MPI_Comm_rank(comm_cart, &mirango);

    // =========================================================================
    // FASE 4: OBTENER COORDENADAS CARTESIANAS DE CADA PROCESO
    // =========================================================================
    /*
       MPI_Cart_coords: Convierte el rango en coordenadas cartesianas
       - comm_cart: Comunicador cartesiano
       - mirango: Rango del proceso actual
       - ndims: Número de dimensiones
       - coords: Array donde se almacenan las coordenadas [fila, columna]

       Ejemplo con 4×2 procesos:
       Rango 0 → coords[0]=0, coords[1]=0 → posición [0][0]
       Rango 1 → coords[0]=0, coords[1]=1 → posición [0][1]
       Rango 2 → coords[0]=1, coords[1]=0 → posición [1][0]
       ...
    */
    MPI_Cart_coords(comm_cart, mirango, ndims, coords);

    // =========================================================================
    // FASE 5: PROCESO 0 INICIALIZA LAS MATRICES (RESERVA DINÁMICA)
    // =========================================================================
    if (mirango == 0) {
        // Reservar memoria dinámica para las matrices
        matrizA = (int**)malloc(FILAS * sizeof(int*));
        matrizB = (int**)malloc(FILAS * sizeof(int*));
        matrizC = (int**)malloc(FILAS * sizeof(int*));

        for (int i = 0; i < FILAS; i++) {
            matrizA[i] = (int*)malloc(COLUMNAS * sizeof(int));
            matrizB[i] = (int*)malloc(COLUMNAS * sizeof(int));
            matrizC[i] = (int*)malloc(COLUMNAS * sizeof(int));
        }

        printf("Configuracion:\n");
        printf("  - Numero de procesos: %d\n", numprocs);
        printf("  - Topologia: %d filas × %d columnas\n", FILAS, COLUMNAS);
        printf("  - Dimensiones periodicas: NO\n\n");

        // Inicializar generador de números aleatorios
        srand((unsigned int)time(NULL));

        // Inicializar Matriz A con valores aleatorios entre 1 y 10
        printf("Matriz A:\n");
        for (int i = 0; i < FILAS; i++) {
            printf("  ");
            for (int j = 0; j < COLUMNAS; j++) {
                matrizA[i][j] = (int)(rand() % 10 + 1);
                printf("%d ", matrizA[i][j]);
            }
            printf("\n");
        }

        // Inicializar Matriz B con valores aleatorios entre 1 y 10
        printf("\nMatriz B:\n");
        for (int i = 0; i < FILAS; i++) {
            printf("  ");
            for (int j = 0; j < COLUMNAS; j++) {
                matrizB[i][j] = (int)(rand() % 10 + 1);
                printf("%d ", matrizB[i][j]);
            }
            printf("\n");
        }
        printf("\n");

        // Inicializar matriz resultado en ceros
        for (int i = 0; i < FILAS; i++) {
            for (int j = 0; j < COLUMNAS; j++) {
                matrizC[i][j] = 0;
            }
        }

        // Iniciar temporizador
        tiempo_inicio = MPI_Wtime();
    }

    // =========================================================================
    // FASE 6: DISTRIBUCIÓN DE ELEMENTOS A CADA PROCESO
    // =========================================================================
    /*
       Cada proceso recibe el elemento de las matrices A y B que corresponde
       a sus coordenadas cartesianas. El proceso en (i,j) recibe A[i][j] y B[i][j].

       Usamos comunicación punto a punto porque cada proceso necesita un elemento
       diferente según su posición en la topología.
    */

    if (mirango == 0) {
        // Proceso 0 envía elementos a todos los demás procesos
        for (int rango_destino = 1; rango_destino < numprocs; rango_destino++) {
            int coord_destino[2];

            // Obtener las coordenadas del proceso destino
            MPI_Cart_coords(comm_cart, rango_destino, ndims, coord_destino);

            int fila = coord_destino[0];
            int col = coord_destino[1];

            // Enviar el elemento correspondiente de la matriz A
            MPI_Send(&matrizA[fila][col], 1, MPI_INT, rango_destino, 0, comm_cart);

            // Enviar el elemento correspondiente de la matriz B
            MPI_Send(&matrizB[fila][col], 1, MPI_INT, rango_destino, 1, comm_cart);
        }

        // Proceso 0 toma sus propios elementos (posición [0][0])
        elemento_A = matrizA[0][0];
        elemento_B = matrizB[0][0];
    }
    else {
        // Los demás procesos reciben sus elementos
        MPI_Recv(&elemento_A, 1, MPI_INT, 0, 0, comm_cart, MPI_STATUS_IGNORE);
        MPI_Recv(&elemento_B, 1, MPI_INT, 0, 1, comm_cart, MPI_STATUS_IGNORE);
    }

    // =========================================================================
    // FASE 7: CADA PROCESO CALCULA SU SUMA LOCAL
    // =========================================================================
    /*
       Cálculo paralelo: Cada proceso suma los elementos que recibió
       El proceso en coordenadas (i,j) calcula: C[i][j] = A[i][j] + B[i][j]
    */
    elemento_C = elemento_A + elemento_B;

    // =========================================================================
    // FASE 8: MOSTRAR CÁLCULOS DE FORMA ORDENADA
    // =========================================================================
    // Sincronizar para imprimir en orden por coordenadas
    for (int fila = 0; fila < FILAS; fila++) {
        for (int col = 0; col < COLUMNAS; col++) {
            if (coords[0] == fila && coords[1] == col) {
                printf("[Proceso %d - Coords(%d,%d)] %d + %d = %d\n",
                    mirango, coords[0], coords[1],
                    elemento_A, elemento_B, elemento_C);
                fflush(stdout);
            }
            MPI_Barrier(comm_cart);
        }
    }

    // =========================================================================
    // FASE 9: RECOLECTAR RESULTADOS EN EL PROCESO 0
    // =========================================================================
    if (mirango == 0) {
        // Proceso 0 coloca su resultado
        matrizC[0][0] = elemento_C;

        // Recibir resultados de los demás procesos
        for (int rango_origen = 1; rango_origen < numprocs; rango_origen++) {
            int coord_origen[2];
            int resultado;

            // Obtener coordenadas del proceso origen
            MPI_Cart_coords(comm_cart, rango_origen, ndims, coord_origen);

            int fila = coord_origen[0];
            int col = coord_origen[1];

            // Recibir el resultado
            MPI_Recv(&resultado, 1, MPI_INT, rango_origen, 2, comm_cart, MPI_STATUS_IGNORE);

            // Almacenar en la matriz resultado
            matrizC[fila][col] = resultado;
        }
    }
    else {
        // Los demás procesos envían su resultado
        MPI_Send(&elemento_C, 1, MPI_INT, 0, 2, comm_cart);
    }

    // =========================================================================
    // FASE 10: PROCESO 0 MUESTRA EL RESULTADO FINAL
    // =========================================================================
    if (mirango == 0) {
        // Finalizar temporizador
        tiempo_fin = MPI_Wtime();

        printf("\n================================================================================\n");
        printf("MATRIZ RESULTADO (C = A + B):\n");
        printf("================================================================================\n");
        for (int i = 0; i < FILAS; i++) {
            printf("  ");
            for (int j = 0; j < COLUMNAS; j++) {
                printf("%d ", matrizC[i][j]);
            }
            printf("\n");
        }
        printf("\n================================================================================\n");
        printf("Tiempo de ejecucion: %.6f segundos\n", tiempo_fin - tiempo_inicio);
        printf("================================================================================\n");
    }

    // =========================================================================
    // FASE 11: FINALIZACIÓN Y LIBERACIÓN DE MEMORIA
    // =========================================================================
    if (mirango == 0) {
        // Liberar memoria dinámica
        for (int i = 0; i < FILAS; i++) {
            free(matrizA[i]);
            free(matrizB[i]);
            free(matrizC[i]);
        }
        free(matrizA);
        free(matrizB);
        free(matrizC);
    }

    MPI_Finalize();
    return 0;
}