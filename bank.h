#ifndef BANK_H
#define BANH_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#define ACCERR "Account Error:"
#define BNKERR "Bank Error:"
#define ACCNUM 20
#define NAMLEN 100

typedef enum {FALSE, TRUE} bool;


//Account
typedef struct{
	char name[NAMLEN + 1]; //max length 100 + '\0'
	float currentBalance; //String formatter: %2.2f 
	pthread_mutex_t lock; //to lock accounts
	bool inSession;
} Accnt;

/* Initialize an Account, return 0 on success, -1/-2 on failure */
int openAccount(Accnt * deAccount, char * aName);

/* Return the current balance of the input account. If no account return 0 */
int balanceAccount(Accnt * deAccount);

/* Add the balance of the input account by the input amount
	Return the new balance. If no account return 0 */
int creditMoney(Accnt * deAccount, float amount);

/* Subtract the balance of the input account by the input amount
	Return the new balance. If no account return 0 */
int debitMoney(Accnt * deAccount, float amount);


//Bank
typedef struct {
	pthread_mutex_t lock; //lock for printing/adding/removing(?) accounts
	Accnt accounts[ACCNUM]; //Bank Accounts
	int activeAccounts; //keep the number of active accounts
} Bank;

/* Initialize a Bank
	return 0 on success, -1/-2 on failure */
int buildDaBank(Bank * daBank);

/* Create and add an account to the Bank
	return 0 on success, -1/-2/-3/-4 on failure */
int addAccount(Bank * daBank, char * aName);

/* Search the Bank for an Account by name
	return the position of the Accnt on success, -1/-2/-3 on failure */
int findAccount( Bank * daBank, char * aName);

/* Remove an Account from the Bank
	return 0 on success and -1/-2/-3/-4 on failure
  **This function is/was not needed in the implementation of our Bank** */
int removeAccount(Bank * daBank, char * aName);

/* Print Account information for all accounts in the Bank
 ** This function will run in its own thread, waiting for a Semaphore
	to consume before performing the next iteration. Works together with a
	SIGALARM that sounds every 20 seconds and posts the Semaphore ** */
int printAccounts(Bank * daBank);

#endif