#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_ACCOUNTS 2
#define NUM_THREADS 6
#define TRANSFERS_PER_THREAD 10000
#define INITIAL_BALANCE 10000.0
#define TRANSFER_AMOUNT 1.0

typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock;
} Account;

typedef struct {
    int thread_number;
    int from_id;
    int to_id;
    double amount;
    int successful_transfers;
} TransferData;

Account accounts[NUM_ACCOUNTS];

static double elapsed_seconds(
    struct timespec start,
    struct timespec end
) {
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) /
           1000000000.0;
}

int safe_transfer(
    int from_id,
    int to_id,
    double amount
) {
    if (
        from_id < 0 ||
        from_id >= NUM_ACCOUNTS ||
        to_id < 0 ||
        to_id >= NUM_ACCOUNTS ||
        from_id == to_id ||
        amount <= 0.0
    ) {
        return 0;
    }

    /*
     * Deadlock prevention:
     * always lock the account with the lower ID first.
     */
    int first_lock;
    int second_lock;

    if (from_id < to_id) {
        first_lock = from_id;
        second_lock = to_id;
    } else {
        first_lock = to_id;
        second_lock = from_id;
    }

    pthread_mutex_lock(
        &accounts[first_lock].lock
    );

    pthread_mutex_lock(
        &accounts[second_lock].lock
    );

    int completed = 0;

    if (accounts[from_id].balance >= amount) {
        accounts[from_id].balance -= amount;
        accounts[to_id].balance += amount;

        accounts[from_id].transaction_count++;
        accounts[to_id].transaction_count++;

        completed = 1;
    }

    /*
     * Unlock in reverse order.
     */
    pthread_mutex_unlock(
        &accounts[second_lock].lock
    );

    pthread_mutex_unlock(
        &accounts[first_lock].lock
    );

    return completed;
}

void *transfer_thread(void *arg) {
    TransferData *data = (TransferData *)arg;

    printf(
        "Thread %d (%lu): transfers account %d -> account %d\n",
        data->thread_number,
        (unsigned long)pthread_self(),
        data->from_id,
        data->to_id
    );

    data->successful_transfers = 0;

    for (int i = 0; i < TRANSFERS_PER_THREAD; i++) {
        data->successful_transfers += safe_transfer(
            data->from_id,
            data->to_id,
            data->amount
        );
    }

    printf(
        "Thread %d completed %d successful transfers.\n",
        data->thread_number,
        data->successful_transfers
    );

    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    TransferData thread_data[NUM_THREADS];

    struct timespec start_time;
    struct timespec end_time;

    printf(
        "Phase 4: Deadlock Prevention "
        "With Lock Ordering\n"
    );

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;

        if (
            pthread_mutex_init(
                &accounts[i].lock,
                NULL
            ) != 0
        ) {
            fprintf(
                stderr,
                "Error initializing account %d mutex\n",
                i
            );

            return EXIT_FAILURE;
        }
    }

    double initial_total =
        accounts[0].balance +
        accounts[1].balance;

    printf(
        "Initial account 0 balance: %.2f\n",
        accounts[0].balance
    );

    printf(
        "Initial account 1 balance: %.2f\n",
        accounts[1].balance
    );

    printf(
        "Initial combined balance:  %.2f\n\n",
        initial_total
    );

    clock_gettime(
        CLOCK_MONOTONIC,
        &start_time
    );

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_number = i + 1;
        thread_data[i].amount = TRANSFER_AMOUNT;
        thread_data[i].successful_transfers = 0;

        if (i % 2 == 0) {
            thread_data[i].from_id = 0;
            thread_data[i].to_id = 1;
        } else {
            thread_data[i].from_id = 1;
            thread_data[i].to_id = 0;
        }

        int result = pthread_create(
            &threads[i],
            NULL,
            transfer_thread,
            &thread_data[i]
        );

        if (result != 0) {
            fprintf(
                stderr,
                "Error creating transfer thread %d\n",
                i + 1
            );

            return EXIT_FAILURE;
        }
    }

    int total_successful_transfers = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        int result = pthread_join(
            threads[i],
            NULL
        );

        if (result != 0) {
            fprintf(
                stderr,
                "Error joining thread %d\n",
                i + 1
            );

            return EXIT_FAILURE;
        }

        total_successful_transfers +=
            thread_data[i].successful_transfers;
    }

    clock_gettime(
        CLOCK_MONOTONIC,
        &end_time
    );

    double final_total =
        accounts[0].balance +
        accounts[1].balance;

    printf(
        "\nFinal account 0 balance: %.2f\n",
        accounts[0].balance
    );

    printf(
        "Final account 1 balance: %.2f\n",
        accounts[1].balance
    );

    printf(
        "Final combined balance:  %.2f\n",
        final_total
    );

    printf(
        "Successful transfers: %d\n",
        total_successful_transfers
    );

    printf(
        "Account 0 transaction count: %d\n",
        accounts[0].transaction_count
    );

    printf(
        "Account 1 transaction count: %d\n",
        accounts[1].transaction_count
    );

    printf(
        "Execution time: %.6f seconds\n",
        elapsed_seconds(start_time, end_time)
    );

    if (final_total == initial_total) {
        printf(
            "\nSuccess: all threads finished without deadlock.\n"
        );

        printf(
            "The combined account balance remained unchanged.\n"
        );
    } else {
        printf(
            "\nError: the combined balance changed unexpectedly.\n"
        );
    }

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        if (
            pthread_mutex_destroy(
                &accounts[i].lock
            ) != 0
        ) {
            fprintf(
                stderr,
                "Warning: failed to destroy "
                "account %d mutex\n",
                i
            );

            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
