/*
 * EJERCICIO FINAL - COMPUTACIÓN PARALELA
 * Multiplicación Matriz-Vector Distribuida con Análisis Estadístico
 * * Especificaciones del ejercicio:
 * - N = 12 (Fijo)
 * - Memoria Estática (sin malloc dinámico)
 * - Uso obligatorio de: MPI_Scatter, MPI_Bcast, MPI_Gather, MPI_Reduce
 * - Verificación: Y[0]=572. (Nota: Y[11] matemático es 1430, aunque el PDF diga 1142).
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h> // Necesario para INT_MIN

#define N 12 // Dimensión fija según restricción

int main(int argc, char* argv[]) {
    // 1. Inicialización del entorno MPI
    MPI_Init(&argc, &argv);

    int rango, n_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);

    // Validación: El número de procesos debe ser divisor de N
    if (N % n_procs != 0) {
        if (rango == 0) {
            printf("Error: El numero de procesos (%d) debe ser divisor de N (%d).\n", n_procs, N);
            printf("Por favor, ejecuta con 1, 2, 3, 4, 6 o 12 procesos.\n");
        }
        MPI_Finalize();
        return 1;
    }

    // --- VARIABLES DE MEMORIA ESTÁTICA ---

    // Variables globales (Usadas principalmente por el proceso 0)
    int A[N][N];    // Matriz completa
    int X[N];       // Vector X completo
    int Y[N];       // Vector Y resultado completo

    // Variables locales (para cada proceso esclavo)
    int filas_por_proc = N / n_procs;   // Cuantas filas le tocan a cada uno

    // Buffer para recibir el trozo de la matriz A.
    // Aunque reservamos espacio para NxN estático, solo usaremos las filas correspondientes.
    int local_A[N][N];

    // Buffer para el resultado parcial de este proceso
    int local_Y[N];

    // Variables para estadísticas locales y globales
    int local_max = INT_MIN;
    long local_suma = 0;
    int global_max;
    long global_suma;

    double tiempo_inicio, tiempo_fin;

    // 2. INICIALIZACIÓN DE DATOS (Solo Proceso 0)
    if (rango == 0) {
        printf("--- Inicio del Programa MPI ---\n");
        printf("Configuracion: N=%d, Procesos=%d, Filas/Proceso=%d\n", N, n_procs, filas_por_proc);

        // Inicializar Matriz A: A[i][j] = i + j
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i][j] = i + j;
            }
        }

        // Inicializar Vector X: X[j] = j + 1
        for (int j = 0; j < N; j++) {
            X[j] = j + 1;
        }

        tiempo_inicio = MPI_Wtime(); // Inicio del cronómetro
    }

    // 3. DISTRIBUCIÓN DE DATOS (Comunicación Colectiva)

    // A) Distribuir el Vector X completo a TODOS los procesos
    // Todos necesitan X completo para poder multiplicar contra sus filas
    MPI_Bcast(X, N, MPI_INT, 0, MPI_COMM_WORLD);

    // B) Distribuir la Matriz A por bloques de filas
    // El proceso 0 envía 'filas_por_proc' filas a cada proceso.
    // Al ser memoria estática contigua, enviamos (filas * N) enteros.
    MPI_Scatter(A,                  // Buffer de envío (root)
        filas_por_proc * N, // Cantidad de datos a enviar a cada uno
        MPI_INT,            // Tipo de dato
        local_A,            // Buffer de recepción
        filas_por_proc * N, // Cantidad de datos a recibir
        MPI_INT,
        0,                  // Root
        MPI_COMM_WORLD);

    // 4. CÁLCULO PARALELO
    // Cada proceso calcula su porción del vector Y y sus estadísticas locales

    for (int i = 0; i < filas_por_proc; i++) {
        local_Y[i] = 0; // Inicializar acumulador de la fila

        for (int j = 0; j < N; j++) {
            // Realizamos la multiplicación: Fila * Columna
            // local_A[i][j] accede a la fila 'i' dentro del bloque recibido
            local_Y[i] += local_A[i][j] * X[j];
        }

        // Cálculo estadístico local (aprovechando el mismo bucle)
        if (local_Y[i] > local_max) {
            local_max = local_Y[i];
        }
        local_suma += local_Y[i];
    }

    // 5. RECOLECCIÓN DE RESULTADOS

    // A) Reunir todos los trozos de Y en el vector completo del proceso 0
    MPI_Gather(local_Y,         // Buffer de envío (mi parte calculada)
        filas_por_proc,  // Cuantos elementos envío
        MPI_INT,
        Y,               // Buffer de recepción (donde se junta todo)
        filas_por_proc,  // Cuantos elementos recibo de CADA uno
        MPI_INT,
        0,
        MPI_COMM_WORLD);

    // B) Calcular el Máximo Global (Reducción)
    MPI_Reduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    // C) Calcular la Suma Global (Reducción)
    MPI_Reduce(&local_suma, &global_suma, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    // 6. MOSTRAR RESULTADOS (Solo Proceso 0)
    if (rango == 0) {
        tiempo_fin = MPI_Wtime();

        printf("\n--- Resultados Finales ---\n");

        // Imprimir el vector resultado
        printf("Vector Resultado Y: [ ");
        for (int k = 0; k < N; k++) printf("%d ", Y[k]);
        printf("]\n\n");

        printf("Valor Maximo Global: %d\n", global_max);
        printf("Suma Total Global:   %ld\n", global_suma);
        printf("Tiempo de Ejecucion: %f segundos\n", tiempo_fin - tiempo_inicio);

        // Verificación contra los datos del enunciado
        printf("\n--- Verificacion de Datos ---\n");

        // Verificación 1: Y[0] debe ser 572
        if (Y[0] == 572) {
            printf("[OK] Y[0] es 572 (Correcto).\n");
        }
        else {
            printf("[ERROR] Y[0] es %d (Esperado: 572).\n", Y[0]);
        }

        // Verificación 2: Y[11]. El enunciado dice 1142, pero matemáticamente es 1430.
        if (Y[11] == 1142) {
            printf("[OK] Y[11] es 1142 (Coincide con PDF).\n");
        }
        else if (Y[11] == 1430) {
            printf("[AVISO] Y[11] es 1430. (Matematicamente correcto para A=i+j, X=j+1).\n");
            printf("        Nota: El valor 1142 del enunciado parece ser una errata.\n");
        }
        else {
            printf("[ERROR] Y[11] es %d (Esperado PDF: 1142, Esperado Mat: 1430).\n", Y[11]);
        }
    }
    MPI_Finalize();
    return 0;
}