/*
================================================================================
  PR�CTICA 6: NUEVOS MODOS DE ENV�O - COMUNICACI�N NO BLOQUEANTE
  Universidad de Burgos - Escuela Polit�cnica Superior
  Grado en Ingenier�a Inform�tica
  Arquitectura Paralela con MPI
================================================================================

  DESCRIPCI�N:
  Este programa demuestra el uso de comunicaciones no bloqueantes en MPI.
  El proceso 0 recibe n�meros del usuario y los env�a al proceso 1 para calcular
  su factorial. La comunicaci�n no bloqueante permite que el proceso 0 continue
  trabajando mientras espera que se complete el env�o anterior.

  ESCENARIO:
  - Proceso 0: Recibe n�meros del usuario y los env�a al proceso 1
  - Proceso 1: Calcula el factorial de cada n�mero recibido
  - Condici�n de salida: Introducir 0 termina el programa

  COMUNICACI�N NO BLOQUEANTE:
  - MPI_Isend: Inicia el env�o sin bloquear el proceso
  - MPI_Irecv: Inicia la recepci�n sin bloquear el proceso
  - MPI_Wait: Espera a que se complete una operaci�n no bloqueante
  - MPI_Test: Comprueba si una operaci�n ha terminado sin bloquear

  VENTAJAS:
  - Permite solapar c�lculo y comunicaci�n
  - Evita esperas innecesarias
  - Mejora el rendimiento en aplicaciones complejas

  REQUISITOS:
  - Compatible con Visual Studio + DeinoMPI
  - M�nimo 2 procesos (proceso 0 y proceso 1)
================================================================================
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

// Funci�n para calcular el factorial de un n�mero
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

    // Variables para comunicaci�n no bloqueante
    MPI_Request request_envio;     // Manejador para operaci�n de env�o
    MPI_Request request_recepcion; // Manejador para operaci�n de recepci�n
    MPI_Status status;             // Estado de las operaciones

    int numero;                    // N�mero a procesar
    int flag_completado;           // Flag para MPI_Test
    int envio_en_curso = 0;        // Indica si hay un env�o pendiente

    // =========================================================================
    // FASE 1: INICIALIZACI�N DE MPI
    // =========================================================================
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // =========================================================================
    // FASE 2: VALIDACI�N DEL N�MERO DE PROCESOS
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
    // FASE 3: L�GICA DEL PROCESO 0 (EMISOR)
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
            // Solicitar n�mero al usuario
            printf("Introduce un numero (0 para salir): ");
            fflush(stdout);

            if (scanf_s("%d", &numero) != 1) {
                // Limpiar buffer en caso de entrada no v�lida
                while (getchar() != '\n');
                printf("ERROR: Entrada no valida. Introduce un numero entero.\n\n");
                continue;
            }

            /*
               Si hay un env�o anterior en curso, debemos esperar a que se complete
               antes de reutilizar el buffer 'numero' para evitar condiciones de carrera.

               MPI_Wait: Bloquea el proceso hasta que la operaci�n no bloqueante
                        referenciada por 'request_envio' se haya completado.

               Par�metros:
               - request_envio: Manejador de la operaci�n a esperar
               - &status: Estado de la operaci�n (no lo usamos aqu�)
            */
            if (envio_en_curso) {
                printf("  [Esperando a que se complete el envio anterior...]\n");
                MPI_Wait(&request_envio, &status);
                printf("  [Envio anterior completado]\n\n");
                envio_en_curso = 0;
            }

            // Condici�n de salida
            if (numero == 0) {
                printf("\n[Proceso 0] Enviando se�al de terminacion al proceso 1...\n");

                /*
                   MPI_Isend: Inicia un env�o no bloqueante

                   A diferencia de MPI_Send (bloqueante), esta funci�n retorna
                   inmediatamente despu�s de iniciar el env�o, permitiendo que
                   el proceso continue ejecut�ndose.

                   Par�metros:
                   - &numero: Buffer con los datos a enviar
                   - 1: N�mero de elementos a enviar
                   - MPI_INT: Tipo de datos
                   - 1: Rango del proceso destino
                   - 0: Etiqueta del mensaje
                   - MPI_COMM_WORLD: Comunicador
                   - &request_envio: Manejador para la operaci�n (salida)

                   El manejador 'request_envio' se usa posteriormente con MPI_Wait
                   o MPI_Test para verificar/esperar la finalizaci�n.
                */
                MPI_Isend(&numero, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request_envio);

                // Esperar a que se complete el env�o de salida
                MPI_Wait(&request_envio, &status);
                printf("[Proceso 0] Terminando...\n");
                break;
            }

            // Validar que el n�mero sea v�lido para factorial
            if (numero < 0) {
                printf("ERROR: El factorial no esta definido para numeros negativos.\n\n");
                continue;
            }

            if (numero > 20) {
                printf("ADVERTENCIA: El factorial de %d puede causar desbordamiento.\n", numero);
            }

            // Enviar n�mero al proceso 1 usando comunicaci�n no bloqueante
            printf("[Proceso 0] Enviando %d al proceso 1 (envio no bloqueante)...\n", numero);

            MPI_Isend(&numero, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request_envio);
            envio_en_curso = 1;  // Marcar que hay un env�o pendiente

            printf("[Proceso 0] Envio iniciado. Puedes continuar trabajando...\n");

            /*
               Aqu� se podr�a hacer trabajo adicional mientras el env�o se completa.
               En este caso, simplemente volvemos al inicio del bucle para pedir
               el siguiente n�mero, pero el MPI_Wait al inicio del bucle se encargar�
               de esperar si es necesario.
            */

            printf("\n");
        }
    }

    // =========================================================================
    // FASE 4: L�GICA DEL PROCESO 1 (RECEPTOR Y CALCULADOR)
    // =========================================================================
    else if (mirango == 1) {
        printf("[Proceso 1] Listo para recibir numeros y calcular factoriales.\n");
        printf("[Proceso 1] Esperando datos del proceso 0...\n\n");
        fflush(stdout);

        while (1) {
            /*
               MPI_Irecv: Inicia una recepci�n no bloqueante

               Similar a MPI_Isend, esta funci�n retorna inmediatamente despu�s
               de iniciar la operaci�n de recepci�n. El proceso puede continuar
               ejecut�ndose y verificar posteriormente si los datos han llegado.

               Par�metros:
               - &numero: Buffer donde se almacenar�n los datos recibidos
               - 1: N�mero de elementos a recibir
               - MPI_INT: Tipo de datos
               - 0: Rango del proceso emisor
               - 0: Etiqueta del mensaje
               - MPI_COMM_WORLD: Comunicador
               - &request_recepcion: Manejador para la operaci�n (salida)
            */
            MPI_Irecv(&numero, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request_recepcion);

            /*
               Aqu� se podr�a hacer trabajo �til mientras esperamos el mensaje.
               Para demostrar el concepto, usamos MPI_Test en un bucle que
               permite hacer otras tareas mientras verificamos si lleg� el mensaje.
            */

            flag_completado = 0;
            int contador_espera = 0;

            while (!flag_completado) {
                /*
                   MPI_Test: Comprueba si una operaci�n no bloqueante ha terminado

                   A diferencia de MPI_Wait (que bloquea), MPI_Test retorna
                   inmediatamente indicando si la operaci�n se complet� o no.

                   Par�metros:
                   - &request_recepcion: Manejador de la operaci�n a verificar
                   - &flag_completado: Flag que indica si se complet� (1) o no (0)
                   - &status: Estado de la operaci�n si se complet�

                   Esto permite hacer polling: verificar peri�dicamente sin bloquear.
                */
                MPI_Test(&request_recepcion, &flag_completado, &status);

                if (!flag_completado) {
                    // Aqu� se podr�a hacer trabajo �til
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

            // Condici�n de salida
            if (numero == 0) {
                printf("[Proceso 1] Se�al de terminacion recibida. Finalizando...\n");
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
    // FASE 6: FINALIZACI�N
    // =========================================================================
    MPI_Finalize();
    return 0;
}