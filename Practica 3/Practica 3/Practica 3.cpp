#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    int mirango, numprocs;
    int n;  // Tama�o de los vectores
    int* vector_x = NULL;  // Vector X completo (solo en proceso 0)
    int* vector_y = NULL;  // Vector Y completo (solo en proceso 0)
    int elemento_x;        // Elemento local de X para cada proceso
    int elemento_y;        // Elemento local de Y para cada proceso
    int producto_parcial;  // Producto local: elemento_x * elemento_y
    int producto_escalar;  // Resultado final del producto escalar
    double tiempo_inicio, tiempo_fin;
    // Inicializar MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // El proceso 0 inicializa los vectores y solicita el tama�o
    if (mirango == 0) {
        printf("  PRODUCTO ESCALAR DE VECTORES CON MPI\n");
        printf("Numero de procesos: %d\n", numprocs);
        fflush(stdout);
        // Bucle de validaci�n: repetir hasta que el usuario introduzca el tama�o correcto
        int tama�o_valido = 0;
        while (!tama�o_valido) {
            printf("Introduce el tama�o de los vectores (debe ser %d): ", numprocs);
            fflush(stdout);

            // Verificar que la entrada sea un n�mero v�lido
            if (scanf_s("%d", &n) != 1) {
                // Limpiar el buffer en caso de entrada no num�rica
                while (getchar() != '\n');
                printf("\nERROR: Debes introducir un numero entero.\n");
                continue;
            }
            // Validar que el tama�o coincida con el n�mero de procesos
            if (n != numprocs) {
                printf("\nERROR: El tama�o debe ser %d (numero de procesos disponibles).\n", numprocs);
                printf("Has introducido: %d. Intentalo de nuevo.\n\n", n);
            }
            else if (n <= 0) {
                printf("\nERROR: El tama�o debe ser un numero positivo.\n\n");
            }
            else {
                tama�o_valido = 1;  // Tama�o correcto, salir del bucle
            }
        }
        // Reservar memoria para los vectores completos
        vector_x = (int*)malloc(n * sizeof(int));
        vector_y = (int*)malloc(n * sizeof(int));

        // Inicializar los vectores con valores
        printf("\nInicializando vectores...\n");
        printf("Vector X: [ ");
        for (int i = 0; i < n; i++) {
            vector_x[i] = i + 1;  // X = [1, 2, 3, ..., n]
            printf("%d ", vector_x[i]);
        }
        printf("]\n");

        printf("Vector Y: [ ");
        for (int i = 0; i < n; i++) {
            vector_y[i] = i + 1;  // Y = [1, 2, 3, ..., n]
            printf("%d ", vector_y[i]);
        }
        printf("]\n\n");
        // Iniciar medici�n de tiempo
        tiempo_inicio = MPI_Wtime();
    }
    // Broadcast del tama�o de los vectores a todos los procesos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // Distribuir un elemento de cada vector a cada proceso usando MPI_Scatter
    // Proceso 0 reparte los elementos: el elemento i va al proceso de rango i
    MPI_Scatter(vector_x,1,MPI_INT,&elemento_x,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Scatter(vector_y,1,MPI_INT,&elemento_y,1,MPI_INT,0,MPI_COMM_WORLD);
    // Cada proceso calcula su producto parcial: x_i * y_i
    producto_parcial = elemento_x * elemento_y;

    // Sincronizar antes de imprimir los c�lculos parciales
    MPI_Barrier(MPI_COMM_WORLD);

    // Imprimir los c�lculos de forma ordenada por rango
    for (int i = 0; i < numprocs; i++) {
        if (mirango == i) {
            printf("[Proceso %d] Calculando: %d * %d = %d\n",
                mirango, elemento_x, elemento_y, producto_parcial);
            fflush(stdout);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Esperar a que todos terminen de imprimir
    if (mirango == 0) {
        printf("\n");
        fflush(stdout);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Reducir todos los productos parciales sum�ndolos en el proceso 0
    // Esto implementa: producto_escalar = sum(x_i * y_i) para i = 0 hasta n-1
    MPI_Reduce(&producto_parcial,&producto_escalar,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);

    // El proceso 0 muestra el resultado final
    if (mirango == 0) {
        // Finalizar medici�n de tiempo
        tiempo_fin = MPI_Wtime();

        printf("\n========================================\n");
        printf("RESULTADO:\n");
        printf("========================================\n");
        printf("Producto escalar (X � Y) = %d\n", producto_escalar);
        printf("Tiempo de ejecucion: %.6f segundos\n", tiempo_fin - tiempo_inicio);
        printf("========================================\n");

        // Liberar memoria
        free(vector_x);
        free(vector_y);
    }
    // Finalizar MPI
    MPI_Finalize();
    return 0;
}