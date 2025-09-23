#include <mpi.h>
#include <stdio.h>
int main(int argc, char* argv[])
{
	int mirango, tamano;
	int longitud;
	char nombre[32];
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &mirango);
	MPI_Comm_size(MPI_COMM_WORLD, &tamano);
	MPI_Get_processor_name(nombre, &longitud);
	MPI_Status estado;

	// Pedir un dato al usuario y el rango 0 manda lo envia als rango 1
	int num = 0;
	if (mirango == 0) {
		printf("Soy el proceso %d de %d, y voy a enviar un dato\n", mirango+1, tamano);
		fflush(stdout);
		do {
			printf("Introduce un numero entre 0 y 1000\n");
			fflush(stdout);
			scanf_s("%d", &num);
			if (num <= 0 || num > 1000) {
				printf("\nEl numero tiene que estar entre 1 y 1000\n\n");
			}
		} while (num <= 0 || num > 1000);
		// El rango 0 envia el dato
		// El tag podria ser cualquier valor, siempre que sea lo mismo en los dos
		// en el MPI_Send y MPI_Recv

		// El rango 0 envia el dato
		MPI_Send(&num, 1, MPI_INT, 1, 2000, MPI_COMM_WORLD);
		printf("\nRango %d ha enviado: %d",mirango,num);
		fflush(stdout);
	}
	else if (mirango == 1) {
		printf("Soy el proceso %d de %d, y voy a recibir un dato\n", mirango+1, tamano);
		fflush(stdout);
		//El rango 1 recibe el dato
		MPI_Recv(&num, 1, MPI_INT, 0, 2000, MPI_COMM_WORLD, &estado);
		printf("\nRango %d ha recibido: %d",mirango,num);
		fflush(stdout);
	}
	else {
		printf("Error!");
		fflush(stdout);
	}
	// La propia documentacion de deinompi recomienda llamar al metodo fflush(stdout)
	// porque la salida de la apliacion es almacenado en el buffer por defecto y debemos llamar 
	// a esa funcion para ver la salida del printf inmediatamente

	MPI_Finalize();
	return 0;
}
