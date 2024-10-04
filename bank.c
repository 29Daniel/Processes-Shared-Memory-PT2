#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#define TRUE 1

// Function to sleep for a random time between 0 and 5 seconds
void random_sleep() {
    int sleep_time = rand() % 6;  // Random number between 0 and 5
    sleep(sleep_time);
}

int main() {
    int shmid;  // Shared memory ID
    int *shared_mem;  // Pointer to shared memory (for BankAccount and Turn)
    int BankAccount = 0, Turn = 0;  // Initialize shared variables

    // Seed random number generator
    srand(time(0));

    // Create shared memory segment
    shmid = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }

    // Attach shared memory
    shared_mem = (int *)shmat(shmid, NULL, 0);
    if (shared_mem == (int *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize shared memory
    shared_mem[0] = 0;  // BankAccount
    shared_mem[1] = 0;  // Turn

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {  // Child process (Poor Student)
        for (int i = 0; i < 25; i++) {
            random_sleep();  // Sleep for random time
            int account = shared_mem[0];  // Copy BankAccount to local variable

            // Wait until Turn is 1
            while (shared_mem[1] != 1);

            // Generate a random balance needed
            int balance_needed = rand() % 51;  // Between 0 and 50
            printf("Poor Student needs $%d\n", balance_needed);

            if (balance_needed <= account) {
                account -= balance_needed;  // Withdraw money
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", balance_needed, account);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", account);
            }

            shared_mem[0] = account;  // Copy local account back to BankAccount
            shared_mem[1] = 0;  // Set Turn to 0 (Dad's turn)
        }

    } else {  // Parent process (Dear Old Dad)
        for (int i = 0; i < 25; i++) {
            random_sleep();  // Sleep for random time
            int account = shared_mem[0];  // Copy BankAccount to local variable

            // Wait until Turn is 0
            while (shared_mem[1] != 0);

            if (account <= 100) {
                // Generate a random amount to deposit
                int deposit = rand() % 101;  // Between 0 and 100
                if (deposit % 2 == 0) {  // Only deposit if the random number is even
                    account += deposit;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", deposit, account);
                } else {
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
            }

            shared_mem[0] = account;  // Copy local account back to BankAccount
            shared_mem[1] = 1;  // Set Turn to 1 (Student's turn)
        }

        // Wait for child process to finish
        wait(NULL);

        // Detach and remove shared memory
        shmdt(shared_mem);
        shmctl(shmid, IPC_RMID, NULL);
    }

    return 0;
}
