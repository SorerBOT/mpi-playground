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

/*
 * In case the input size is not a power of 2, and that 2^k is the highest power of two still lower
 * than the input size, then I'll simply comute the prefix-sum of the first 2^k elements using the parallel algorithm, and the remaining elements sequentially.
 * There are other ways to do this, like simulating nodes with 0 rank, or by comple
*/
void prefix_sum_sequential(int initial_rank, int prefix_sum)
{
    int comm_rank = -1;
    int comm_size = -1;

    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

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
        for (int i = 1; i <= highest_two_power_in_comm_size; ++i)
        {
            int segment_size =  1 << i;
            int segment_position = comm_rank % segment_size;
            int segment_start = comm_rank - segment_position;


            int segment_sendercomm_rank = segment_start + segment_size / 2 - 1;
            int segment_receivercomm_rank = segment_start + segment_size - 1;

            if (comm_rank == segment_sendercomm_rank)
            {
                MPI_Send(&prefix_sum, 1, MPI_INTEGER, segment_receivercomm_rank, PG_MPI_TAG_SUM, new_comm);
            }
            else if (comm_rank == segment_receivercomm_rank)
            {
                int result = 0;
                MPI_Recv(&result, 1, MPI_INTEGER, segment_sendercomm_rank, PG_MPI_TAG_SUM, new_comm, MPI_STATUS_IGNORE);
                prefix_sum += result;
            }

            if (comm_rank == segment_size - 1)
            {
                printf("rank=%d x=%d prefix=%d\n", comm_rank, comm_rank, prefix_sum);
            }

            MPI_Barrier(new_comm);
        }
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
