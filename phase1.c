#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sched.h>
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
} Account;

typedef struct {
    int teller_id;
} ThreadData;

Account account = {1, INITIAL_BALANCE, 0};

static void tiny_delay(void) {
    struct timespec delay = {0, 1000};
    nanosleep(&delay, NULL);
}

void *teller_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        /*
         * Intentionally unsafe read-modify-write operation.
         * This creates a race condition.
         */
        double old_balance = account.balance;
        int old_count = account.transaction_count;

        sched_yield();
        tiny_delay();

        account.balance = old_balance + DEPOSIT_AMOUNT;
        account.transaction_count = old_count + 1;

        if (i < 2 || i == OPERATIONS_PER_THREAD - 1) {
            printf(
                "Teller %d, thread %lu: deposit #%d, "
                "balance %.2f -> %.2f\n",
                data->teller_id,
                (unsigned long)pthread_self(),
                i + 1,
                old_balance,
                old_balance + DEPOSIT_AMOUNT
            );
        }
    }

    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    printf("Phase 1: Threads Without Synchronization\n");
    printf("Initial balance: %.2f\n", account.balance);
    printf(
        "Number of deposits: %d\n\n",
        NUM_THREADS * OPERATIONS_PER_THREAD
    );

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
            return EXIT_FAILURE;
        }
    }

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

    if (
        account.balance != expected_balance ||
        account.transaction_count != expected_count
    ) {
        printf(
            "\nRace condition demonstrated: "
            "some updates were lost.\n"
        );
    } else {
        printf(
            "\nThis run produced the expected result.\n"
            "Run it again because race-condition results "
            "depend on thread scheduling.\n"
        );
    }

    return EXIT_SUCCESS;
}
