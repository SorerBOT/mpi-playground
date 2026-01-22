#include <stdio.h>
#include <mpi.h>

#define PG_MPI_RANK_MINIMUM 0
#define PG_MPI_TAG_SUM 0



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

    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    int prefix_sum = -1;
    if (comm_rank == PG_MPI_RANK_MINIMUM)
    {
        prefix_sum = comm_rank;
    }
    else
    {
        MPI_Recv(&prefix_sum, 1, MPI_INTEGER, comm_rank - 1, PG_MPI_TAG_SUM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        prefix_sum += comm_rank;
    }

    printf("rank=%d x=%d prefix=%d\n", comm_rank, comm_rank, prefix_sum);

    if (comm_rank < comm_size - 1)
    {
        MPI_Send(&prefix_sum, 1, MPI_INTEGER, comm_rank + 1, PG_MPI_TAG_SUM, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
