#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <mpi.h>

#define PG_MM_BASE_10 10
#define PG_MM_DEBUG 0

void mpi_error_handler_callback(MPI_Comm* comm, int* error_code, ...)
{
    int rank = -1;
    char pg_mpi_error_buffer[MPI_MAX_ERROR_STRING];
    int error_length = -1;

    MPI_Error_string(*error_code, pg_mpi_error_buffer, &error_length);
    int rank_error = MPI_Comm_rank(*comm, &rank);

    if (rank_error == MPI_SUCCESS)
    {
        fprintf(stderr, "OMPI: error occurred: %s in rank %d\n", pg_mpi_error_buffer, rank);
    }
    else
    {
        fprintf(stderr, "OMPI: error occurred: %s\n", pg_mpi_error_buffer);
    }

    MPI_Abort(*comm, *error_code);
}

char* get_argv(int* argc, char*** argv)
{
    if (argc == 0)
    {
        fprintf(stderr, "PG_MM: failed to get argv. No more arguments remaining.\n");
        exit(EXIT_FAILURE);
    }
    char* temp = *(*argv);
   --(*argc);
   ++(*argv);
   return temp;
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    MPI_Errhandler error_handler = {0};
    MPI_Comm_create_errhandler(mpi_error_handler_callback, &error_handler);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, error_handler);

    char* prog_name = get_argv(&argc, &argv);
    char* N_stringified = get_argv(&argc, &argv);
    char* seed_A_stringified = get_argv(&argc, &argv);
    char* seed_B_stringified = get_argv(&argc, &argv);
    char* max_value_stringified = get_argv(&argc, &argv);

    int N = strtol(N_stringified, (char**)NULL, PG_MM_BASE_10);

    uint64_t seed_A = strtoull(seed_A_stringified, (char**)NULL, PG_MM_BASE_10);
    if (seed_A == ULLONG_MAX)
    {
        perror("PG_MM: strtoull\n");
        exit(EXIT_FAILURE);
    }

    uint64_t seed_B = strtoull(seed_B_stringified, (char**)NULL, PG_MM_BASE_10);
    if (seed_B == ULLONG_MAX)
    {
        perror("PG_MM: strtoull\n");
        exit(EXIT_FAILURE);
    }

    int max_value = atoi(max_value_stringified);

#if PG_MM_DEBUG
    printf("SRC data: N = %s, seed_A = %s, seed_B = %s, max_value = %s\n", N_stringified, seed_A_stringified, seed_B_stringified, max_value_stringified);
    printf("N = %d, seed_A = %llu, seed_B = %llu, max_value = %d\n", N, seed_A, seed_B, max_value);
#endif

    int comm_rank = -1;
    int comm_size = -1;

    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);



    MPI_Finalize();
    return 0;
}
