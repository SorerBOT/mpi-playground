#include <stdio.h>
#include <mpi.h>

#define PG_MPI_RANK_MINIMUM 0
#define PG_MPI_TAG_SUM 0

#define PG_MPI_COLOR_PARALLEL 0
#define PG_MPI_COLOR_SEQUENTIAL 1

#define PG_DEBUG_MODE 1

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

    int highest_two_power_in_comm_size = 0;
    while ((1 << (highest_two_power_in_comm_size + 1)) <= comm_size)
    {
        ++highest_two_power_in_comm_size;
    }
    int highest_two_power_in_comm_size_actual_number = (1 << highest_two_power_in_comm_size);
    int color = (comm_rank < highest_two_power_in_comm_size_actual_number)
        ? PG_MPI_COLOR_PARALLEL
        : PG_MPI_COLOR_SEQUENTIAL;

    MPI_Comm new_comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, comm_rank, &new_comm);

    int prefix_sum = comm_rank;

    if (color == PG_MPI_COLOR_PARALLEL)
    {
        for (int jump_size = 1; jump_size <= highest_two_power_in_comm_size_actual_number / 2; jump_size *= 2)
        {
            int send_to = comm_rank + jump_size;
            int receive_from = comm_rank - jump_size;

            if (send_to < highest_two_power_in_comm_size_actual_number)
            {
                MPI_Send(&prefix_sum, 1, MPI_INTEGER, send_to, PG_MPI_TAG_SUM, new_comm);
            }
            if (receive_from >= 0)
            {
                int result = 0;
                MPI_Recv(&result, 1, MPI_INTEGER, receive_from, PG_MPI_TAG_SUM, new_comm, MPI_STATUS_IGNORE);
                prefix_sum += result;
            }

            MPI_Barrier(new_comm);
        }

        printf("rank=%d x=%d prefix=%d\n", comm_rank, comm_rank, prefix_sum);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (comm_rank == highest_two_power_in_comm_size_actual_number - 1)
    {
        MPI_Send(&prefix_sum, 1, MPI_INTEGER, highest_two_power_in_comm_size_actual_number, PG_MPI_TAG_SUM, MPI_COMM_WORLD);
    }

    if (color == PG_MPI_COLOR_SEQUENTIAL)
    {
        if (comm_rank == highest_two_power_in_comm_size_actual_number)
        {
            int response = 0;
            MPI_Recv(&response, 1, MPI_INTEGER, highest_two_power_in_comm_size_actual_number - 1, PG_MPI_TAG_SUM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            prefix_sum += response;
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
    }

    MPI_Finalize();
    return 0;
}
