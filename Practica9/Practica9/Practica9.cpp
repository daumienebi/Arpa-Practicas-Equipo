/**
 * PRÁCTICA 9: Ejemplo de aplicación práctica (Algoritmo de Strassen con Relleno).
 * * Este programa implementa el algoritmo recursivo de Strassen en paralelo
 * utilizando la técnica de división de comunicadores (MPI_Comm_split).
 *
 * FUNCIONAMIENTO MEJORADO:
 * 1. Acepta cualquier tamaño de matriz N (ej. 1000).
 * 2. Calcula la siguiente potencia de 2 (ej. 1024) y usa "padding" (relleno de ceros).
 * 3. Ejecuta el algoritmo de Strassen recursivo sobre la matriz con relleno (N_pad).
 * 4. Solo imprime las matrices A, B y C si N <= 20.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Prototipos de Funciones
int encontrar_siguiente_potencia_de_2(int n);
float** reservar_matriz_contigua(int filas, int cols);
void liberar_matriz_contigua(float** mat);
void inicializar_matriz_con_padding(float** mat, int n_real, int n_pad);
void imprimir_matriz_con_padding(float** mat, int n_real, const char* titulo);
void matrix_add(float** A, float** B, float** C, int n);
void matrix_sub(float** A, float** B, float** C, int n);
void matrix_mult_tradicional(float** A, float** B, float** C, int n);
void dividir_matriz(float** A, float** A11, float** A12, float** A21, float** A22, int n_half);
void unir_matrices(float** C11, float** C12, float** C21, float** C22, float** C, int n_half);
void strassen_paralelo(float** A, float** B, float** C, int n, MPI_Comm comm);

#define BASE_CASE_N 64     // Umbral para cambiar a multiplicación tradicional
#define PRINT_THRESHOLD 20 // Límite de N para imprimir matrices

int main(int argc, char* argv[]) {
    int rango_global, num_procesos_global;
    int N_real = 0; // El N del usuario (ej. 1000)
    int N_pad = 0;  // El N potencia de 2 (ej. 1024)
    float** A, ** B, ** C;
    double t_inicio, t_fin;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rango_global);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_global);

    if (rango_global == 0) {
        printf("Introduce el tamaño (N) de las matrices: ");
        fflush(stdout);
        scanf_s("%d", &N_real);

        if (N_real <= 0) {
            N_real = -1; // Señal de error
        }
        N_pad = encontrar_siguiente_potencia_de_2(N_real);
        printf("[Proceso 0] N Real: %d. Usando N con relleno (Padding): %d\n", N_real, N_pad);
    }
    MPI_Bcast(&N_real, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&N_pad, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (N_real <= 0) {
        MPI_Finalize();
        return 1;
    }
    A = reservar_matriz_contigua(N_pad, N_pad);
    B = reservar_matriz_contigua(N_pad, N_pad);
    C = reservar_matriz_contigua(N_pad, N_pad);
    if (rango_global == 0) {
        srand((unsigned int)time(NULL));
        inicializar_matriz_con_padding(A, N_real, N_pad);
        inicializar_matriz_con_padding(B, N_real, N_pad);
        // --- IMPRESIÓN CONDICIONAL DE A Y B ---
        if (N_real <= PRINT_THRESHOLD) {
            printf("\n--- (N <= %d) Imprimiendo matrices generadas ---\n", PRINT_THRESHOLD);
            imprimir_matriz_con_padding(A, N_real, "Matriz A Generada");
            imprimir_matriz_con_padding(B, N_real, "Matriz B Generada");
        }
        else {
            printf("\n--- (N > %d, omitiendo impresión de matrices A y B) ---\n", PRINT_THRESHOLD);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    t_inicio = MPI_Wtime();
    MPI_Bcast(A[0], N_pad * N_pad, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(B[0], N_pad * N_pad, MPI_FLOAT, 0, MPI_COMM_WORLD);
    strassen_paralelo(A, B, C, N_pad, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    t_fin = MPI_Wtime();
    if (rango_global == 0) {
        printf("\n[Proceso 0] Cálculo de Strassen completado.\n");
        printf("[Proceso 0] N Real=%d (N Pad=%d), Procesos=%d\n", N_real, N_pad, num_procesos_global);
        printf("[Proceso 0] Tiempo total de ejecución: %f segundos.\n", t_fin - t_inicio);
        if (N_real <= PRINT_THRESHOLD) {
            imprimir_matriz_con_padding(C, N_real, "Resultado C");
        }
        else {
            printf("--- (N > %d, omitiendo impresión de matriz C) ---\n", PRINT_THRESHOLD);
        }
    }
    liberar_matriz_contigua(A);
    liberar_matriz_contigua(B);
    liberar_matriz_contigua(C);
    MPI_Finalize();
    return 0;
}

// Función recursiva paralela de Strassen.
void strassen_paralelo(float** A, float** B, float** C, int n, MPI_Comm comm) {
    int rango, num_procesos;
    MPI_Comm_rank(comm, &rango);
    MPI_Comm_size(comm, &num_procesos);

    // Caso base
    if (n <= BASE_CASE_N || num_procesos == 1) {
        if (rango == 0) {
            matrix_mult_tradicional(A, B, C, n);
        }
        MPI_Bcast(C[0], n * n, MPI_FLOAT, 0, comm);
        return;
    }
    // PASO RECURSIVO
    int n_half = n / 2;
    // Asignar memoria para cuadrantes (A, B, C)
    float** A11, ** A12, ** A21, ** A22;
    float** B11, ** B12, ** B21, ** B22;
    float** C11, ** C12, ** C21, ** C22;
    A11 = reservar_matriz_contigua(n_half, n_half);
    A12 = reservar_matriz_contigua(n_half, n_half);
    A21 = reservar_matriz_contigua(n_half, n_half);
    A22 = reservar_matriz_contigua(n_half, n_half);
    B11 = reservar_matriz_contigua(n_half, n_half);
    B12 = reservar_matriz_contigua(n_half, n_half);
    B21 = reservar_matriz_contigua(n_half, n_half);
    B22 = reservar_matriz_contigua(n_half, n_half);
    C11 = reservar_matriz_contigua(n_half, n_half);
    C12 = reservar_matriz_contigua(n_half, n_half);
    C21 = reservar_matriz_contigua(n_half, n_half);
    C22 = reservar_matriz_contigua(n_half, n_half);

    // Asignar memoria para matrices S (Sumas) y P (Productos)
    float** S1, ** S2, ** S3, ** S4, ** S5, ** S6, ** S7, ** S8, ** S9, ** S10;
    float** P1, ** P2, ** P3, ** P4, ** P5, ** P6, ** P7;
    S1 = reservar_matriz_contigua(n_half, n_half);
    S2 = reservar_matriz_contigua(n_half, n_half);
    S3 = reservar_matriz_contigua(n_half, n_half);
    S4 = reservar_matriz_contigua(n_half, n_half);
    S5 = reservar_matriz_contigua(n_half, n_half);
    S6 = reservar_matriz_contigua(n_half, n_half);
    S7 = reservar_matriz_contigua(n_half, n_half);
    S8 = reservar_matriz_contigua(n_half, n_half);
    S9 = reservar_matriz_contigua(n_half, n_half);
    S10 = reservar_matriz_contigua(n_half, n_half);
    P1 = reservar_matriz_contigua(n_half, n_half);
    P2 = reservar_matriz_contigua(n_half, n_half);
    P3 = reservar_matriz_contigua(n_half, n_half);
    P4 = reservar_matriz_contigua(n_half, n_half);
    P5 = reservar_matriz_contigua(n_half, n_half);
    P6 = reservar_matriz_contigua(n_half, n_half);
    P7 = reservar_matriz_contigua(n_half, n_half);
    // Dividir A y B en cuadrantes (Localmente en todos los procesos)
    dividir_matriz(A, A11, A12, A21, A22, n_half);
    dividir_matriz(B, B11, B12, B21, B22, n_half);
    // Calcular las 10 matrices S (Localmente en todos los procesos)
    matrix_sub(B12, B22, S1, n_half);
    matrix_add(A11, A12, S2, n_half);
    matrix_add(A21, A22, S3, n_half);
    matrix_sub(B21, B11, S4, n_half);
    matrix_add(A11, A22, S5, n_half);
    matrix_add(B11, B22, S6, n_half);
    matrix_sub(A12, A22, S7, n_half);
    matrix_add(B21, B22, S8, n_half);
    matrix_sub(A11, A21, S9, n_half);
    matrix_add(B11, B12, S10, n_half);
    // Dividir el comunicador 'comm' en 7 sub-grupos (colores)
    int color = rango % 7;
    MPI_Comm new_comm;
    MPI_Comm_split(comm, color, rango, &new_comm);
    // Llamadas recursivas paralelas
    switch (color) {
    case 0: strassen_paralelo(A11, S1, P1, n_half, new_comm); break;
    case 1: strassen_paralelo(S2, B22, P2, n_half, new_comm); break;
    case 2: strassen_paralelo(S3, B11, P3, n_half, new_comm); break;
    case 3: strassen_paralelo(A22, S4, P4, n_half, new_comm); break;
    case 4: strassen_paralelo(S5, S6, P5, n_half, new_comm); break;
    case 5: strassen_paralelo(S7, S8, P6, n_half, new_comm); break;
    case 6: strassen_paralelo(S9, S10, P7, n_half, new_comm); break;
    }
    // Recolectar resultados (Matrices P) en el Proceso 0 del 'comm' padre
    int new_rango;
    MPI_Comm_rank(new_comm, &new_rango);
    if (new_rango == 0 && rango != 0) {
        float** P_local = NULL;
        switch (color) {
        case 1: P_local = P2; break;
        case 2: P_local = P3; break;
        case 3: P_local = P4; break;
        case 4: P_local = P5; break;
        case 5: P_local = P6; break;
        case 6: P_local = P7; break;
        }
        if (P_local) {
            MPI_Send(P_local[0], n_half * n_half, MPI_FLOAT, 0, color, comm);
        }
    }
    if (rango == 0) {
        // Soy el líder principal (rango 0). Recibo los otros P_i.
        for (int i = 1; i < 7; i++) {
            float** P_target = NULL;
            switch (i) {
            case 1: P_target = P2; break;
            case 2: P_target = P3; break;
            case 3: P_target = P4; break;
            case 4: P_target = P5; break;
            case 5: P_target = P6; break;
            case 6: P_target = P7; break;
            }

            if (i < num_procesos) {
                MPI_Recv(P_target[0], n_half * n_half, MPI_FLOAT, i, i,
                    comm, MPI_STATUS_IGNORE);
            }
            else {
                // Nadie calculó esta P. La calculo secuencialmente.
                switch (i) {
                case 1: matrix_mult_tradicional(S2, B22, P2, n_half); break;
                case 2: matrix_mult_tradicional(S3, B11, P3, n_half); break;
                case 3: matrix_mult_tradicional(A22, S4, P4, n_half); break;
                case 4: matrix_mult_tradicional(S5, S6, P5, n_half); break;
                case 5: matrix_mult_tradicional(S7, S8, P6, n_half); break;
                case 6: matrix_mult_tradicional(S9, S10, P7, n_half); break;
                }
            }
        }
        // Calcular matrices C (Solo el líder principal)
        float** T1 = reservar_matriz_contigua(n_half, n_half);
        float** T2 = reservar_matriz_contigua(n_half, n_half);

        matrix_add(P5, P4, T1, n_half);
        matrix_sub(T1, P2, T2, n_half);
        matrix_add(T2, P6, C11, n_half); // C11 = P5 + P4 - P2 + P6
        matrix_add(P1, P2, C12, n_half); // C12 = P1 + P2
        matrix_add(P3, P4, C21, n_half); // C21 = P3 + P4
        matrix_add(P5, P1, T1, n_half);
        matrix_sub(T1, P3, T2, n_half);
        matrix_sub(T2, P7, C22, n_half); // C22 = P5 + P1 - P3 - P7
        // Unir C (Solo el líder principal)
        unir_matrices(C11, C12, C21, C22, C, n_half);
        liberar_matriz_contigua(T1);
        liberar_matriz_contigua(T2);
    }
    // Distribuir el resultado C final a todos en 'comm'
    MPI_Bcast(C[0], n * n, MPI_FLOAT, 0, comm);
    // Liberar memoria intermedia
    MPI_Comm_free(&new_comm);
    liberar_matriz_contigua(A11); liberar_matriz_contigua(A12);
    liberar_matriz_contigua(A21); liberar_matriz_contigua(A22);
    liberar_matriz_contigua(B11); liberar_matriz_contigua(B12);
    liberar_matriz_contigua(B21); liberar_matriz_contigua(B22);
    liberar_matriz_contigua(C11); liberar_matriz_contigua(C12);
    liberar_matriz_contigua(C21); liberar_matriz_contigua(C22);
    liberar_matriz_contigua(S1); liberar_matriz_contigua(S2);
    liberar_matriz_contigua(S3); liberar_matriz_contigua(S4);
    liberar_matriz_contigua(S5); liberar_matriz_contigua(S6);
    liberar_matriz_contigua(S7); liberar_matriz_contigua(S8);
    liberar_matriz_contigua(S9); liberar_matriz_contigua(S10);
    liberar_matriz_contigua(P1); liberar_matriz_contigua(P2);
    liberar_matriz_contigua(P3); liberar_matriz_contigua(P4);
    liberar_matriz_contigua(P5); liberar_matriz_contigua(P6);
    liberar_matriz_contigua(P7);
}
// Calcula la potencia de 2 más pequeña que es >= n.
int encontrar_siguiente_potencia_de_2(int n) {
    if (n <= 0) return 1;
    if ((n & (n - 1)) == 0) return n; // Ya es potencia de 2
    int power = 1;
    while (power < n) {
        power *= 2;
    }
    return power;
}

// Asigna memoria para una matriz de [filas x cols] de forma contigua.
float** reservar_matriz_contigua(int filas, int cols) {
    float* datos = (float*)malloc(filas * cols * sizeof(float));
    if (datos == NULL) return NULL;
    float** matriz = (float**)malloc(filas * sizeof(float*));
    if (matriz == NULL) {
        free(datos);
        return NULL;
    }
    for (int i = 0; i < filas; i++) {
        matriz[i] = datos + (i * cols);
    }
    return matriz;
}

// Libera la memoria de una matriz asignada de forma contigua.
void liberar_matriz_contigua(float** mat) {
    if (mat == NULL) return;
    free(mat[0]); // Libera el bloque de datos
    free(mat);    // Libera el array de punteros
}

/**
 * Inicializa una matriz (N_pad x N_pad) con valores aleatorios
 * en la submatriz (N_real x N_real) y 0.0 en el resto (padding).
 */
void inicializar_matriz_con_padding(float** mat, int n_real, int n_pad) {
    for (int i = 0; i < n_pad; i++) {
        for (int j = 0; j < n_pad; j++) {
            if (i < n_real && j < n_real) {
                mat[i][j] = (float)(rand() % 100) / 10.0;
            }
            else {
                mat[i][j] = 0.0f; // Relleno de ceros
            }
        }
    }
}

// Imprime solo la parte (N_real x N_real) de una matriz más grande.
void imprimir_matriz_con_padding(float** mat, int n_real, const char* titulo) {
    printf("\n--- %s (Mostrando N=%d) ---\n", titulo, n_real);
    for (int i = 0; i < n_real; i++) {
        printf("[ ");
        for (int j = 0; j < n_real; j++) {
            printf("%4.1f ", mat[i][j]);
        }
        printf("]\n");
    }
    printf("------------------------\n");
    fflush(stdout);
}

// Multiplicación tradicional (Caso Base).
void matrix_mult_tradicional(float** A, float** B, float** C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Suma de matrices: C = A + B
void matrix_add(float** A, float** B, float** C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = A[i][j] + B[i][j];
        }
    }
}

// Resta de matrices: C = A - B
void matrix_sub(float** A, float** B, float** C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = A[i][j] - B[i][j];
        }
    }
}

// Copia los 4 cuadrantes de A en matrices separadas.
void dividir_matriz(float** A, float** A11, float** A12, float** A21, float** A22, int n_half) {
    for (int i = 0; i < n_half; i++) {
        for (int j = 0; j < n_half; j++) {
            A11[i][j] = A[i][j];
            A12[i][j] = A[i][j + n_half];
            A21[i][j] = A[i + n_half][j];
            A22[i][j] = A[i + n_half][j + n_half];
        }
    }
}

// Monta la matriz C a partir de sus 4 cuadrantes.
void unir_matrices(float** C11, float** C12, float** C21, float** C22, float** C, int n_half) {
    for (int i = 0; i < n_half; i++) {
        for (int j = 0; j < n_half; j++) {
            C[i][j] = C11[i][j];
            C[i][j + n_half] = C12[i][j];
            C[i + n_half][j] = C21[i][j];
            C[i + n_half][j + n_half] = C22[i][j];
        }
    }
}