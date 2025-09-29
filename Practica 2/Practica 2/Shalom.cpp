#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[])
{
    int mirango, tamano;

    // Inicializar MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &tamano);

    int N = 0;

    // Proceso 0 pide N al usuario
    if (mirango == 0) {
        /** do {
             printf("Introduce el tamaño N de la matriz cuadrada: ");
             fflush(stdout);
             scanf_s("%d", &N);
             if (N <= 0) printf("Error: N debe ser mayor que 0.\n");
         } while (N <= 0);
         **/
        N = 1;
    }

    // Compartir N con todos los procesos
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Comprobar que el número de procesos coincide con N
    if (tamano != N) {
        if (mirango == 0)
            printf("Error: el número de procesos (%d) debe ser igual a N (%d)\n", tamano, N);
        MPI_Finalize();
        return 1;
    }

    // Reservar memoria dinámica para matrices 2D
    int** A = (int**)malloc(N * sizeof(int*));
    int** B = (int**)malloc(N * sizeof(int*));
    int** C = (int**)malloc(N * sizeof(int*));
    for (int i = 0; i < N; i++) {
        A[i] = (int*)malloc(N * sizeof(int));
        B[i] = (int*)malloc(N * sizeof(int));
        C[i] = (int*)malloc(N * sizeof(int));
    }

    // Cada proceso tiene un array para su columna
    int* colA = (int*)malloc(N * sizeof(int));
    int* colB = (int*)malloc(N * sizeof(int));
    int* colC = (int*)malloc(N * sizeof(int));

    // Inicializar matrices en proceso 0 con números aleatorios
    if (mirango == 0) {
        srand(time(NULL));
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++) {
                A[i][j] = rand() % 100; // 0..99
                B[i][j] = rand() % 100;
            }

        // Mostrar matrices generadas
        printf("\nMatrices generadas por el proceso 0:\n");

        printf("\nMatriz A:\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++)
                printf("%4d ", A[i][j]);
            printf("\n");
        }

        printf("\nMatriz B:\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++)
                printf("%4d ", B[i][j]);
            printf("\n");
        }
    }

    // Enviar matrices completas a todos los procesos usando MPI_Bcast
    for (int i = 0; i < N; i++) {
        MPI_Bcast(A[i], N, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(B[i], N, MPI_INT, 0, MPI_COMM_WORLD);
    }

    double t1 = MPI_Wtime(); // tiempo inicio cálculo

    // Cada proceso extrae su columna y calcula la suma
    for (int i = 0; i < N; i++) {
        colA[i] = A[i][mirango];
        colB[i] = B[i][mirango];
        colC[i] = colA[i] + colB[i];
    }

    // Recolectar columnas en el proceso 0 usando MPI_Gather
    MPI_Gather(colC, N, MPI_INT, C[0], N, MPI_INT, 0, MPI_COMM_WORLD);

    double t2 = MPI_Wtime(); // tiempo fin cálculo

    // Proceso 0 reconstruye la matriz C y la imprime
    if (mirango == 0) {
        printf("\nMatriz C = A + B:\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++)
                printf("%4d ", C[i][j]);
            printf("\n");
        }

        printf("\nTiempo de cálculo: %f segundos\n", t2 - t1);
    }

    // Liberar memoria dinámica 2D
    for (int i = 0; i < N; i++) {
        free(A[i]);
        free(B[i]);
        free(C[i]);
    }
    free(A);
    free(B);
    free(C);

    free(colA);
    free(colB);
    free(colC);

    MPI_Finalize();
    return 0;
}

