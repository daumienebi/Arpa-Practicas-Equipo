/*
================================================================================
  PRÁCTICA 6: NUEVOS MODOS DE ENVÍO - COMUNICACIÓN NO BLOQUEANTE
  Universidad de Burgos - Escuela Politécnica Superior
  Grado en Ingeniería Informática
  Arquitectura Paralela con MPI
================================================================================

  DESCRIPCIÓN:
  Este programa demuestra el uso de comunicaciones no bloqueantes en MPI.
  El proceso 0 recibe números del usuario y los envía al proceso 1 para calcular
  su factorial. La comunicación no bloqueante permite que el proceso 0 continue
  trabajando mientras espera que se complete el envío anterior.

  ESCENARIO:
  - Proceso 0: Recibe números del usuario y los envía al proceso 1
  - Proceso 1: Calcula el factorial de cada número recibido
  - Condición de salida: Introducir 0 termina el programa

  COMUNICACIÓN NO BLOQUEANTE:
  - MPI_Isend: Inicia el envío sin bloquear el proceso
  - MPI_Irecv: Inicia la recepción sin bloquear el proceso
  - MPI_Wait: Espera a que se complete una operación no bloqueante
  - MPI_Test: Comprueba si una operación ha terminado sin bloquear

  VENTAJAS:
  - Permite solapar cálculo y comunicación
  - Evita esperas innecesarias
  - Mejora el rendimiento en aplicaciones complejas

  REQUISITOS:
  - Compatible con Visual Studio + DeinoMPI
  - Mínimo 2 procesos (proceso 0 y proceso 1)
================================================================================
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

// Función para calcular el factorial de un número
unsigned long long calcular_factorial(int n) {
    if (n < 0) return 0;  // Factorial no definido para negativos
    if (n == 0 || n == 1) return 1;

    unsigned long long resultado = 1;
    for (int i = 2; i <= n; i++) {
        resultado *= i;
    }
    return resultado;
}

int main(int argc, char* argv[]) {
    int mirango, numprocs;

    // Variables para comunicación no bloqueante
    MPI_Request request_envio;     // Manejador para operación de envío
    MPI_Request request_recepcion; // Manejador para operación de recepción
    MPI_Status status;             // Estado de las operaciones

    int numero;                    // Número a procesar
    int flag_completado;           // Flag para MPI_Test
    int envio_en_curso = 0;        // Indica si hay un envío pendiente

    // =========================================================================
    // FASE 1: INICIALIZACIÓN DE MPI
    // =========================================================================
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // =========================================================================
    // FASE 2: VALIDACIÓN DEL NÚMERO DE PROCESOS
    // =========================================================================
    if (numprocs < 2) {
        if (mirango == 0) {
            printf("ERROR: Este programa requiere al menos 2 procesos.\n");
            printf("Ejecuta con: mpiexec -n 2 practica6.exe\n");
        }
        MPI_Finalize();
        return 1;
    }

    // =========================================================================
    // FASE 3: LÓGICA DEL PROCESO 0 (EMISOR)
    // =========================================================================
    if (mirango == 0) {
        printf("================================================================================\n");
        printf("  PRACTICA 6: COMUNICACION NO BLOQUEANTE - CALCULO DE FACTORIALES\n");
        printf("================================================================================\n\n");
        printf("Instrucciones:\n");
        printf("  - Introduce numeros enteros positivos para calcular su factorial\n");
        printf("  - El proceso 1 calculara los factoriales\n");
        printf("  - Introduce 0 para salir\n");
        printf("  - Se usara comunicacion no bloqueante (MPI_Isend/MPI_Wait)\n\n");
        printf("--------------------------------------------------------------------------------\n\n");

        while (1) {
            // Solicitar número al usuario
            printf("Introduce un numero (0 para salir): ");
            fflush(stdout);

            if (scanf_s("%d", &numero) != 1) {
                // Limpiar buffer en caso de entrada no válida
                while (getchar() != '\n');
                printf("ERROR: Entrada no valida. Introduce un numero entero.\n\n");
                continue;
            }

            /*
               Si hay un envío anterior en curso, debemos esperar a que se complete
               antes de reutilizar el buffer 'numero' para evitar condiciones de carrera.

               MPI_Wait: Bloquea el proceso hasta que la operación no bloqueante
                        referenciada por 'request_envio' se haya completado.

               Parámetros:
               - request_envio: Manejador de la operación a esperar
               - &status: Estado de la operación (no lo usamos aquí)
            */
            if (envio_en_curso) {
                printf("  [Esperando a que se complete el envio anterior...]\n");
                MPI_Wait(&request_envio, &status);
                printf("  [Envio anterior completado]\n\n");
                envio_en_curso = 0;
            }

            // Condición de salida
            if (numero == 0) {
                printf("\n[Proceso 0] Enviando señal de terminacion al proceso 1...\n");

                /*
                   MPI_Isend: Inicia un envío no bloqueante

                   A diferencia de MPI_Send (bloqueante), esta función retorna
                   inmediatamente después de iniciar el envío, permitiendo que
                   el proceso continue ejecutándose.

                   Parámetros:
                   - &numero: Buffer con los datos a enviar
                   - 1: Número de elementos a enviar
                   - MPI_INT: Tipo de datos
                   - 1: Rango del proceso destino
                   - 0: Etiqueta del mensaje
                   - MPI_COMM_WORLD: Comunicador
                   - &request_envio: Manejador para la operación (salida)

                   El manejador 'request_envio' se usa posteriormente con MPI_Wait
                   o MPI_Test para verificar/esperar la finalización.
                */
                MPI_Isend(&numero, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request_envio);

                // Esperar a que se complete el envío de salida
                MPI_Wait(&request_envio, &status);
                printf("[Proceso 0] Terminando...\n");
                break;
            }

            // Validar que el número sea válido para factorial
            if (numero < 0) {
                printf("ERROR: El factorial no esta definido para numeros negativos.\n\n");
                continue;
            }

            if (numero > 20) {
                printf("ADVERTENCIA: El factorial de %d puede causar desbordamiento.\n", numero);
            }

            // Enviar número al proceso 1 usando comunicación no bloqueante
            printf("[Proceso 0] Enviando %d al proceso 1 (envio no bloqueante)...\n", numero);

            MPI_Isend(&numero, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request_envio);
            envio_en_curso = 1;  // Marcar que hay un envío pendiente

            printf("[Proceso 0] Envio iniciado. Puedes continuar trabajando...\n");

            /*
               Aquí se podría hacer trabajo adicional mientras el envío se completa.
               En este caso, simplemente volvemos al inicio del bucle para pedir
               el siguiente número, pero el MPI_Wait al inicio del bucle se encargará
               de esperar si es necesario.
            */

            printf("\n");
        }
    }

    // =========================================================================
    // FASE 4: LÓGICA DEL PROCESO 1 (RECEPTOR Y CALCULADOR)
    // =========================================================================
    else if (mirango == 1) {
        printf("[Proceso 1] Listo para recibir numeros y calcular factoriales.\n");
        printf("[Proceso 1] Esperando datos del proceso 0...\n\n");
        fflush(stdout);

        while (1) {
            /*
               MPI_Irecv: Inicia una recepción no bloqueante

               Similar a MPI_Isend, esta función retorna inmediatamente después
               de iniciar la operación de recepción. El proceso puede continuar
               ejecutándose y verificar posteriormente si los datos han llegado.

               Parámetros:
               - &numero: Buffer donde se almacenarán los datos recibidos
               - 1: Número de elementos a recibir
               - MPI_INT: Tipo de datos
               - 0: Rango del proceso emisor
               - 0: Etiqueta del mensaje
               - MPI_COMM_WORLD: Comunicador
               - &request_recepcion: Manejador para la operación (salida)
            */
            MPI_Irecv(&numero, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request_recepcion);

            /*
               Aquí se podría hacer trabajo útil mientras esperamos el mensaje.
               Para demostrar el concepto, usamos MPI_Test en un bucle que
               permite hacer otras tareas mientras verificamos si llegó el mensaje.
            */

            flag_completado = 0;
            int contador_espera = 0;

            while (!flag_completado) {
                /*
                   MPI_Test: Comprueba si una operación no bloqueante ha terminado

                   A diferencia de MPI_Wait (que bloquea), MPI_Test retorna
                   inmediatamente indicando si la operación se completó o no.

                   Parámetros:
                   - &request_recepcion: Manejador de la operación a verificar
                   - &flag_completado: Flag que indica si se completó (1) o no (0)
                   - &status: Estado de la operación si se completó

                   Esto permite hacer polling: verificar periódicamente sin bloquear.
                */
                MPI_Test(&request_recepcion, &flag_completado, &status);

                if (!flag_completado) {
                    // Aquí se podría hacer trabajo útil
                    // En este ejemplo, simplemente mostramos que estamos esperando
                    contador_espera++;

                    if (contador_espera % 100000000 == 0) {
                        printf("[Proceso 1] Aun esperando datos...\n");
                        fflush(stdout);
                    }
                }
            }

            // Ya tenemos el dato, procesarlo
            printf("[Proceso 1] Numero recibido: %d\n", numero);
            fflush(stdout);

            // Condición de salida
            if (numero == 0) {
                printf("[Proceso 1] Señal de terminacion recibida. Finalizando...\n");
                fflush(stdout);
                break;
            }

            // Calcular factorial
            printf("[Proceso 1] Calculando factorial de %d...\n", numero);
            fflush(stdout);

            unsigned long long resultado = calcular_factorial(numero);

            printf("[Proceso 1] Resultado: %d! = %llu\n\n", numero, resultado);
            fflush(stdout);
        }
    }

    // =========================================================================
    // FASE 5: OTROS PROCESOS (SI LOS HAY)
    // =========================================================================
    else {
        // Los procesos adicionales (si los hay) no hacen nada en este ejemplo
        printf("[Proceso %d] No participa en este ejercicio. Esperando...\n", mirango);
        fflush(stdout);
    }

    // =========================================================================
    // FASE 6: FINALIZACIÓN
    // =========================================================================
    MPI_Finalize();
    return 0;
}