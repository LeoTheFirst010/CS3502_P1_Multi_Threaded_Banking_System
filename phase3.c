#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_ACCOUNTS 2
#define NUM_TRANSFER_THREADS 2
#define INITIAL_BALANCE 1000.0
#define TRANSFER_AMOUNT 100.0

typedef struct {
    int account_id;
    double balance;
    pthread_mutex_t lock;
} Account;

typedef struct {
    int thread_number;
    int from_id;
    int to_id;
    double amount;
} TransferData;

Account accounts[NUM_ACCOUNTS];

pthread_barrier_t first_lock_barrier;

static void sleep_seconds(int seconds) {
    struct timespec delay = {seconds, 0};
    nanosleep(&delay, NULL);
}

void *unsafe_transfer_thread(void *arg) {
    TransferData *data = (TransferData *)arg;

    printf(
        "Thread %d (%lu): attempting transfer %.2f "
        "from account %d to account %d\n",
        data->thread_number,
        (unsigned long)pthread_self(),
        data->amount,
        data->from_id,
        data->to_id
    );

    fflush(stdout);

    /*
     * Each thread first locks its source account.
     */
    pthread_mutex_lock(
        &accounts[data->from_id].lock
    );

    printf(
        "Thread %d: locked account %d\n",
        data->thread_number,
        data->from_id
    );

    fflush(stdout);

    /*
     * Wait until both threads have locked their
     * source accounts.
     */
    pthread_barrier_wait(&first_lock_barrier);

    printf(
        "Thread %d: waiting to lock account %d...\n",
        data->thread_number,
        data->to_id
    );

    fflush(stdout);

    /*
     * Thread 1 waits for account 1.
     * Thread 2 waits for account 0.
     * This creates circular wait and deadlock.
     */
    pthread_mutex_lock(
        &accounts[data->to_id].lock
    );

    /*
     * This code should never execute during
     * the intentional deadlock demonstration.
     */
    accounts[data->from_id].balance -= data->amount;
    accounts[data->to_id].balance += data->amount;

    pthread_mutex_unlock(
        &accounts[data->to_id].lock
    );

    pthread_mutex_unlock(
        &accounts[data->from_id].lock
    );

    return NULL;
}

void *deadlock_watchdog(void *arg) {
    (void)arg;

    sleep_seconds(3);

    printf(
        "\nWATCHDOG: No transfer completed for 3 seconds.\n"
    );

    printf(
        "WATCHDOG: Possible deadlock detected.\n"
    );

    printf(
        "Thread 1 holds account 0 and waits for account 1.\n"
    );

    printf(
        "Thread 2 holds account 1 and waits for account 0.\n"
    );

    printf(
        "The program will now terminate the intentional "
        "deadlock demonstration.\n"
    );

    fflush(stdout);

    /*
     * Normal cleanup cannot complete because the
     * worker threads are permanently blocked.
     */
    _Exit(EXIT_SUCCESS);
}

int main(void) {
    pthread_t transfer_threads[NUM_TRANSFER_THREADS];
    pthread_t watchdog_thread;

    TransferData transfers[NUM_TRANSFER_THREADS] = {
        {1, 0, 1, TRANSFER_AMOUNT},
        {2, 1, 0, TRANSFER_AMOUNT}
    };

    printf(
        "Phase 3: Intentional Deadlock Demonstration\n"
    );

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;

        if (
            pthread_mutex_init(
                &accounts[i].lock,
                NULL
            ) != 0
        ) {
            fprintf(
                stderr,
                "Error initializing mutex for account %d\n",
                i
            );

            return EXIT_FAILURE;
        }
    }

    if (
        pthread_barrier_init(
            &first_lock_barrier,
            NULL,
            NUM_TRANSFER_THREADS
        ) != 0
    ) {
        fprintf(
            stderr,
            "Error initializing thread barrier\n"
        );

        return EXIT_FAILURE;
    }

    printf(
        "Account 0 balance: %.2f\n",
        accounts[0].balance
    );

    printf(
        "Account 1 balance: %.2f\n\n",
        accounts[1].balance
    );

    for (int i = 0; i < NUM_TRANSFER_THREADS; i++) {
        int result = pthread_create(
            &transfer_threads[i],
            NULL,
            unsafe_transfer_thread,
            &transfers[i]
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

    if (
        pthread_create(
            &watchdog_thread,
            NULL,
            deadlock_watchdog,
            NULL
        ) != 0
    ) {
        fprintf(
            stderr,
            "Error creating watchdog thread\n"
        );

        return EXIT_FAILURE;
    }

    /*
     * The joins will block because the transfer threads
     * are deadlocked. The watchdog ends the program after
     * reporting the deadlock.
     */
    for (int i = 0; i < NUM_TRANSFER_THREADS; i++) {
        pthread_join(
            transfer_threads[i],
            NULL
        );
    }

    pthread_join(watchdog_thread, NULL);

    return EXIT_SUCCESS;
}
