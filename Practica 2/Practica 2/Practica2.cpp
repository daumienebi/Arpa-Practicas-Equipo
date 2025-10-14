#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[])
{
    int mirango, tamano;
    int N;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &tamano);

    // El proceso 0 pide el tamano de la matriz
    // En nuestro caso, queremos que el minimoo que se pueda calcular sea
    // una matriz 2x2
    if (mirango == 0) {
        do {
            printf("Introduce un numero para la matriz NxN: ");
            fflush(stdout);
            scanf_s("%d", &N);
            if (N < 2) {
                printf("El numero tiene que ser mayor o igual que 2\n");
            }
        } while (N < 2);
    }

    // Enviamos N a todos los procesos para que sepan el tamano de la matriz
    // Depues de hacer este broadcast, todos los procesos ya pueden trabajar con el valor de N
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Verificamos que el número de procesos sea igual a N
    if (tamano != N) {
        if (mirango == 0) {
            printf("Error: El numero de procesos debe ser igual a N=%d\n", N);
        }    
        MPI_Finalize();
        return 0;
    }

    // Creamos la matrices dinámicas y reservamos memoria para sus valores
    int* A = (int*)malloc(N * N * sizeof(int));
    int* B = (int*)malloc(N * N * sizeof(int));
    int* C = (int*)malloc(N * N * sizeof(int));
    int* filaC = (int*)malloc(N * sizeof(int));

    double inicio, fin; // Variables para medir el rendimiento

    // Inicializamos matrices A y B solo en el proceso 0
    // Generando numeros aleatorios entre 1 y 50
    if (mirango == 0) {
        // Cambiamos la semilla para evitar la misma secuencia de numeros aleatorios
        srand(time(NULL)); 
        printf("\nMatriz A:\n");
        for (int i = 0; i < N * N; i++) {
            A[i] = rand() % 50 + 1;
            printf("%3d ", A[i]);
            if ((i + 1) % N == 0) {
                printf("\n");
            }
        }

        printf("\nMatriz B:\n");
        for (int i = 0; i < N * N; i++) {
            B[i] = rand() % 50 + 1;
            printf("%3d ", B[i]);
            if ((i + 1) % N == 0) {
                printf("\n");
            }
        }
        printf("\n");
    }

    // Sincronizamos y medimos 
    // Sincronizamos con MPI_Barrier para poder medir el tiempo correctamente
    // y para que se mida cuando comiencen la suma todos los procesos
    MPI_Barrier(MPI_COMM_WORLD);
    inicio = MPI_Wtime();

    // Enviamos matrices a todos los procesos
    MPI_Bcast(A, N * N, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(B, N * N, MPI_INT, 0, MPI_COMM_WORLD);

    // Cada proceso calcula su fila
    int fila = mirango;
    for (int i = 0; i < N; i++) {
        filaC[i] = A[fila * N + i] + B[fila * N + i];
    }

    // Recolectamos resultados en C (hecho por cada proceso)
    MPI_Gather(filaC, N, MPI_INT, C, N, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    fin = MPI_Wtime();

    if (mirango == 0) {
        printf("Matriz C = A + B:\n");
        for (int i = 0; i < N * N; i++) {
            printf("%3d ", C[i]);
            // Detectar si hay que imprimir el salto de linea o no
            // para imprimir la matriz en la consola correctamente
            // Dado que accedemos de forma contigua
            if ((i + 1) % N == 0) {
                printf("\n");
            }
        }
        printf("\nTiempo de ejecucion: %f segundos\n", fin - inicio);
    }

    // Liberar la memoria dado que hemos reservado memoria de forma dinamica
    free(A); free(B); free(C); free(filaC);
    MPI_Finalize();
    return 0;
}
