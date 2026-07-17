#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS 4
#define OPERATIONS_PER_THREAD 1000
#define DEPOSIT_AMOUNT 1.0
#define INITIAL_BALANCE 1000.0

typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock;
} Account;

typedef struct {
    int teller_id;
} ThreadData;

Account account;

static double elapsed_seconds(
    struct timespec start,
    struct timespec end
) {
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) /
           1000000000.0;
}

void deposit(
    Account *target,
    double amount,
    int teller_id,
    int operation_number
) {
    int result = pthread_mutex_lock(&target->lock);

    if (result != 0) {
        fprintf(
            stderr,
            "Teller %d failed to lock account %d\n",
            teller_id,
            target->account_id
        );
        return;
    }

    /*
     * Critical section:
     * only one thread can update the account at a time.
     */
    target->balance += amount;
    target->transaction_count++;

    if (
        operation_number < 2 ||
        operation_number == OPERATIONS_PER_THREAD - 1
    ) {
        printf(
            "Teller %d, thread %lu: deposit #%d completed; "
            "balance = %.2f\n",
            teller_id,
            (unsigned long)pthread_self(),
            operation_number + 1,
            target->balance
        );
    }

    result = pthread_mutex_unlock(&target->lock);

    if (result != 0) {
        fprintf(
            stderr,
            "Teller %d failed to unlock account %d\n",
            teller_id,
            target->account_id
        );
    }
}

void *teller_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        deposit(
            &account,
            DEPOSIT_AMOUNT,
            data->teller_id,
            i
        );
    }

    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    struct timespec start_time;
    struct timespec end_time;

    account.account_id = 1;
    account.balance = INITIAL_BALANCE;
    account.transaction_count = 0;

    if (pthread_mutex_init(&account.lock, NULL) != 0) {
        fprintf(
            stderr,
            "Error initializing account mutex\n"
        );
        return EXIT_FAILURE;
    }

    printf("Phase 2: Mutex-Protected Account\n");
    printf("Initial balance: %.2f\n\n", account.balance);

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].teller_id = i + 1;

        int result = pthread_create(
            &threads[i],
            NULL,
            teller_thread,
            &thread_data[i]
        );

        if (result != 0) {
            fprintf(
                stderr,
                "Error creating teller thread %d\n",
                i + 1
            );

            pthread_mutex_destroy(&account.lock);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        int result = pthread_join(threads[i], NULL);

        if (result != 0) {
            fprintf(
                stderr,
                "Error joining teller thread %d\n",
                i + 1
            );

            pthread_mutex_destroy(&account.lock);
            return EXIT_FAILURE;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double expected_balance =
        INITIAL_BALANCE +
        NUM_THREADS *
        OPERATIONS_PER_THREAD *
        DEPOSIT_AMOUNT;

    int expected_count =
        NUM_THREADS * OPERATIONS_PER_THREAD;

    printf("\nExpected final balance: %.2f\n", expected_balance);
    printf("Actual final balance:   %.2f\n", account.balance);

    printf(
        "Expected transaction count: %d\n",
        expected_count
    );

    printf(
        "Actual transaction count:   %d\n",
        account.transaction_count
    );

    printf(
        "Execution time: %.6f seconds\n",
        elapsed_seconds(start_time, end_time)
    );

    if (
        account.balance == expected_balance &&
        account.transaction_count == expected_count
    ) {
        printf(
            "\nSynchronization successful: "
            "no updates were lost.\n"
        );
    } else {
        printf(
            "\nUnexpected error: final values are incorrect.\n"
        );
    }

    if (pthread_mutex_destroy(&account.lock) != 0) {
        fprintf(
            stderr,
            "Warning: failed to destroy account mutex\n"
        );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
