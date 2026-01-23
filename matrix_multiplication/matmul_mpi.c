#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>

#include "matrix.h"

#define PG_MM_BASE_10 10
#define PG_MM_DEBUG 1
#define PG_MM_LEADER_RANK 0
#define PG_MM_COLOR_WORKING 1
#define PG_MM_COLOR_SLEEPING 2

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
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    char* temp = *(*argv);
   --(*argc);
   ++(*argv);
   return temp;
}

static int scalar_prod(const IMatrix* A, const IMatrix* B,
        size_t A_row, size_t B_col, int N)
{
    int sum = 0;
    for (int i = 0; i < N; ++i)
    {
        sum += imatrix_get(A, A_row, i) * imatrix_get(B, i, B_col);
    }

    return sum;
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
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    uint64_t seed_B = strtoull(seed_B_stringified, (char**)NULL, PG_MM_BASE_10);
    if (seed_B == ULLONG_MAX)
    {
        perror("PG_MM: strtoull\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    int max_value = atoi(max_value_stringified);

#if PG_MM_DEBUG
    if (comm_rank == PG_MM_LEADER_RANK)
    {
        printf("PG_MM: SRC data: N = %s, seed_A = %s, seed_B = %s, max_value = %s\n", N_stringified, seed_A_stringified, seed_B_stringified, max_value_stringified);
        printf("PG_MM: N = %d, seed_A = %llu, seed_B = %llu, max_value = %d\n", N, seed_A, seed_B, max_value);
    }
#endif



    IMatrix matrix_null = (IMatrix) { .N = 0, .data = NULL };
    IMatrix A = imatrix_alloc(N);
    if ( memcmp(&A, &matrix_null, sizeof(IMatrix)) == 0 )
    {
        fprintf(stderr, "PG_MM: imatrix_alloc() failed to allocate matrix.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    IMatrix B = imatrix_alloc(N);
    if ( memcmp(&B, &matrix_null, sizeof(IMatrix)) == 0 )
    {
        fprintf(stderr, "PG_MM: imatrix_alloc() failed to allocate matrix.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    IMatrix C = imatrix_alloc(N);
    if ( memcmp(&C, &matrix_null, sizeof(IMatrix)) == 0 )
    {
        fprintf(stderr, "PG_MM: imatrix_alloc() failed to allocate matrix.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    imatrix_fill_random(&A, seed_A, max_value);
    imatrix_fill_random(&B, seed_B, max_value);

#ifdef PG_MM_DEBUG
    if (comm_rank == PG_MM_LEADER_RANK)
    {
        imatrix_print(&A, "A");
        imatrix_print(&B, "B");
    }
#endif


    int comm_size_working = (comm_size <= N)
        ? comm_size
        : N;

    int color = (comm_rank < comm_size_working)
        ? PG_MM_COLOR_WORKING
        : PG_MM_COLOR_SLEEPING;

    MPI_Comm comm_new = 0;
    MPI_Comm_split(MPI_COMM_WORLD, color, comm_rank, &comm_new);

    if (color != PG_MM_COLOR_WORKING)
    {
        MPI_Finalize();
        exit(EXIT_SUCCESS);
    }

    int working_A_rows_count = N / comm_size_working;
    int A_row_first = working_A_rows_count * comm_rank;
    int A_row_last = A_row_first + working_A_rows_count;
    if (comm_rank == comm_size_working - 1)
    {
        A_row_last = N;
    }

    for (size_t A_row_current = A_row_first;
            A_row_current < A_row_last; ++A_row_current)
    {
        for (size_t B_col_current = 0; B_col_current < N; ++B_col_current)
        {
            int product = scalar_prod(&A, &B, A_row_current, B_col_current, N);
            imatrix_set(&C, A_row_current, B_col_current, product);
#if PG_MM_DEBUG
            printf("C[%lu, %lu] = %d\n", A_row_current, B_col_current, product);
#endif
        }
    }

    int* recvbuf = NULL;
    int* recvcounts = NULL;
    int* displs = NULL;
    if (comm_rank == PG_MM_LEADER_RANK)
    {
        recvbuf = malloc(N * N * sizeof(int));
        if (recvbuf == NULL)
        {
            perror("PG_MM: malloc()\n");
            MPI_Abort(comm_new, EXIT_FAILURE);
        }

        recvcounts = malloc(comm_size_working * sizeof(int));
        if (recvcounts == NULL)
        {
            perror("PG_MM: malloc()\n");
            MPI_Abort(comm_new, EXIT_FAILURE);
        }

        displs = malloc(comm_size_working * sizeof(int));
        if (displs == NULL)
        {
            perror("PG_MM: malloc()\n");
            MPI_Abort(comm_new, EXIT_FAILURE);
        }

        for (size_t rank = 0; rank < comm_size_working; ++rank)
        {
            displs[rank] = working_A_rows_count * rank * N;
            if (rank == comm_size_working - 1)
            {
                recvcounts[rank] = (N - working_A_rows_count * rank) * N;
            }
            else
            {
                recvcounts[rank] = working_A_rows_count * N;
            }
        }

#if PG_MM_DEBUG
        int recvcounts_sum = 0;
        for (size_t rank = 0; rank < comm_size_working; ++rank)
        {
            recvcounts_sum += recvcounts[rank];
        }
        if (recvcounts_sum != N * N)
        {
            fprintf(stderr, "PG_MM: recvcounts_sum = %d, when its supposed to be: %d\n", recvcounts_sum, N * N);
            MPI_Abort(comm_new, EXIT_FAILURE);
        }
#endif
    }
    int sendcount = (comm_rank == comm_size_working - 1)
        ? (N - working_A_rows_count * comm_rank) * N
        : working_A_rows_count * N;


    int* sendbuf = &C.data[A_row_first * N];
#if PG_MM_DEBUG
    if (sendcount != (A_row_last - A_row_first) * N)
    {
        fprintf(stderr, "Rank %d, sending %d integers to leader\n", comm_rank, sendcount);
        MPI_Abort(comm_new, EXIT_FAILURE);
    }

    for (size_t row = A_row_first; row < A_row_last; ++row)
    {
        for (size_t col = 0; col < N; ++col)
        {
            int expected_value = imatrix_get(&C, row, col);
            int sendbuf_idx = (row - A_row_first) * N + col;
            if (sendbuf[sendbuf_idx] != expected_value)
            {
                fprintf(stderr, "Rank %d is sending the wrong data. sendbuf[%d] = %d instead of C[%lu, %lu] = %d", comm_rank, sendbuf_idx, sendbuf[sendbuf_idx], row, col, expected_value);
                MPI_Abort(comm_new, EXIT_FAILURE);
            }
        }
    }
#endif
    MPI_Gatherv(sendbuf, sendcount,
            MPI_INTEGER, recvbuf, recvcounts,
            displs, MPI_INTEGER,
            PG_MM_LEADER_RANK, comm_new);

    if (comm_rank == PG_MM_LEADER_RANK)
    {
#if PG_MM_DEBUG
        printf("Finished gathering, received the following values:\n");
        for (size_t i = 0; i < N * N; ++i)
        {
            if (i % N == 0 && i > 0)
            {
                printf("\n");
            }
            printf("%d, ", recvbuf[i]);
        }
        printf("\n");
        printf("Printing matrix before update...\n");
        imatrix_print(&C, "C");
#endif
        for (size_t i = 0; i < N * N; ++i)
        {
            imatrix_set(&C, i / N, i % N, recvbuf[i]);
        }

#if PG_MM_DEBUG
        printf("Printing matrix after update...\n");
        imatrix_print(&C, "C");
#endif
    }



    if (comm_rank == PG_MM_LEADER_RANK)
    {
        free(recvbuf);
        free(recvcounts);
        free(displs);
    }

    free(A.data);
    free(B.data);
    free(C.data);

    MPI_Finalize();
    return 0;
}
