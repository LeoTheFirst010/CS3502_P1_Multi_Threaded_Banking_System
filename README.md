# CS3502_P1_Multi_Threaded_Banking_System

CS 3502 OS
Name: Leandro Cherulli

Multi-Threaded Banking System Project

FILES
-----
phase1.c
Demonstrates a race condition by allowing multiple threads to update the same
account without synchronization.

phase2.c
Uses a pthread mutex to protect the shared account and ensure the final balance
and transaction count are correct.

phase3.c
Creates an intentional deadlock. Two threads each lock one account and wait for
the account locked by the other thread.

phase4.c
Prevents deadlock by always locking accounts in the same order.

COMPILE
-------
gcc -Wall -Wextra -pthread phase1.c -o phase1
gcc -Wall -Wextra -pthread phase2.c -o phase2
gcc -Wall -Wextra -pthread phase3.c -o phase3
gcc -Wall -Wextra -pthread phase4.c -o phase4

RUN
---
./phase1
./phase2
./phase3
./phase4

EXPECTED RESULTS
----------------
Phase 1 may produce incorrect totals because some thread updates are lost.

Phase 2 should always produce the correct final balance because the mutex
protects the critical section.

Phase 3 should stop making progress because both transfer threads are waiting
for each other. A watchdog reports the deadlock after about three seconds.

Phase 4 should complete normally without deadlock. The combined balance of both
accounts should remain unchanged.
