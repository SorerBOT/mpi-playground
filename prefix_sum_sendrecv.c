#include <stdio.h>
#include <mpi.h>

#define PG_MPI_COMM_LEADER 0
#define PG_MPI_TAG_HELLO 0



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

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    MPI_Errhandler error_handler = {0};
    MPI_Comm_create_errhandler(mpi_error_handler_callback, &error_handler);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, error_handler);

    int comm_rank = -1;
    int comm_size = -1;
    int error_code = 0;

    error_code = MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);


    error_code = MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    error_code = MPI_Send(&comm_rank, 1,
            MPI_INTEGER, PG_MPI_COMM_LEADER,
            PG_MPI_TAG_HELLO, MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}
