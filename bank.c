#include "bank.h"

//-------------Accnt functions---------------------

int openAccount(Accnt * deAccount, char * aName){
	if(!deAccount){
		printf("%s Invalid Accnt: \n", ACCERR) ;
		return -1 ; // fail
	}

	if(!aName || strlen(aName) > NAMLEN){
		printf("%s Invalid Name: %s\n", ACCERR, aName) ;
		return -2 ; // fail
	}
	
	strcpy(deAccount->name, aName) ;
	deAccount->currentBalance = 0 ;
	deAccount->inSession = FALSE ; 
	
	return 0 ; // success
}

int
balanceAccount(Accnt * deAccount){
	if(!deAccount){
		printf("%s Invalid Accnt: \n", ACCERR) ;
		return 0 ; 
	}
	
	return deAccount->currentBalance ;
}

int
creditMoney(Accnt * deAccount, float amount){
	if(!deAccount){
		printf("%s Invalid Accnt: \n", ACCERR) ;
		return 0 ; 
	} 
	
	if(amount < 0)
		return deAccount->currentBalance ;
	
	deAccount->currentBalance += amount ;
	
	return deAccount->currentBalance ;
}

int
debitMoney(Accnt * deAccount, float amount){
	if(!deAccount){
		printf("%s Invalid Accnt: \n", ACCERR) ;
		return 0 ; 
	}
	if(amount < 0 || deAccount->currentBalance - amount < 0)
		return deAccount->currentBalance ;
	
	/*if(deAccount->currentBalance - amount < 0){
		printf("Not enough money in account to debit. Current balance: $%4.2f\n", deAccount->currentBalance);
		return deAccount->currentBalance ;
	}*/
	
	deAccount->currentBalance -= amount ;

	return deAccount->currentBalance ;
}

 // ----------Bank Functions---------------------

int
buildDaBank(Bank * daBank){
	if(!daBank)	{
		printf("%s NULL Bank given", BNKERR) ;
		return -1 ; //fail
	}
		
	if(pthread_mutex_init(&daBank->lock, 0 ) > 0 ) //initialize the mutex
	{	printf("%s Couldn't initialize the Bank MUTEX\n", BNKERR) ;
		return -2 ; //fail
	}
	
	for(int i=0 ; i < ACCNUM ; i++){
		daBank->accounts[i].name[0] = '\0' ;
		if(pthread_mutex_init(&daBank->accounts[i].lock, 0 ) > 0 ) //initialize the mutex
		{	printf("%s Couldn't initialize the Accnt MUTEX\n", ACCERR) ;
			return -3 ; //fail
		}
	}
	daBank->activeAccounts = 0 ;
	return 0 ; // success
}

int addAccount(Bank * daBank, char * aName){
	if(!daBank){
		printf("%s NULL Bank\n", BNKERR) ;
		return -1 ; // fail
	}
	if(!aName){
		printf("%s Invalid Name: %s\n", ACCERR, aName) ;
		return -2 ; // fail
	}
	
	pthread_mutex_lock( & daBank->lock ); // lock so no-one can interfere
	
	if(daBank->activeAccounts >= ACCNUM){
		printf("%s There are already %d accounts in the bank\n", BNKERR, daBank->activeAccounts) ;
		pthread_mutex_unlock(& daBank->lock) ;
		return -3 ; // fail
	}
	
	int emptySpot = -1; // let's multi-task
	
	for(int i=0; i < ACCNUM; i++){
		if (strcmp(aName, daBank->accounts[i].name) == 0 ){
			printf("%s There is already an account with the name %s\n", ACCERR, aName) ;
			pthread_mutex_unlock(& daBank->lock) ;
			return -4 ; // fail
		}
		
		if(daBank->accounts[i].name[0] == '\0') // find the last open position
			emptySpot = i ;
	}
	
	emptySpot = openAccount(& daBank->accounts[emptySpot], aName) ; // success 0 or fail -1/-2
	
	daBank->activeAccounts = (emptySpot < 0) ? daBank->activeAccounts : daBank->activeAccounts + 1; 
	
	pthread_mutex_unlock(& daBank->lock) ;
	
	return emptySpot ; // success 0 or fail -1 / -2
}

int
findAccount( Bank * daBank, char * aName){
	if(!daBank){
		printf("%s NULL Bank\n", BNKERR) ;
		return -1 ; // fail
	}
	if(!aName){
		printf("%s Invalid Name: %s\n", ACCERR, aName) ;
		return -2 ; // fail
	}
	
	int position = -3 ;
	
	for(int i=0; i < ACCNUM; i++){
		if (strcmp(aName, daBank->accounts[i].name) == 0 ){
			position = i ;
			break ; 
		}
	}
	
	return position ; // success position or fail -1 
}

//This was created because I like to add and remove things
//It is NOT required by the program specifications
int
removeAccount(Bank * daBank, char * aName){
	if(!daBank){
		printf("%s NULL Bank\n", BNKERR) ;
		return -1 ; // fail
	}
	
	if(!aName){
		printf("%s Invalid Name: %s\n", ACCERR, aName) ;
		return -2 ; // fail
	}
	
	pthread_mutex_lock( & daBank->lock ); // lock so no-one can interfere

	int position = -1 ;

	if( (position = findAccount(daBank, aName)) < 0 ){
		printf("%s No Accnt with Name: %s\n", ACCERR, aName) ;
		pthread_mutex_unlock(& daBank->lock) ;
		return -3 ; // fail
	}
		
	if ( daBank->accounts[position].inSession == TRUE ){
		printf("%s Accnt %s currently being served and can not be deleted\n", ACCERR, aName);
		pthread_mutex_unlock(& daBank->lock) ;
		return -4 ; // fail 
	}
	
	daBank->accounts[position].name[0] = '\0' ; 
	
	pthread_mutex_unlock(& daBank->lock) ;
	
	return 0 ; // success
}

int
printAccounts(Bank * daBank){
	if(!daBank){
		printf("%s NULL Bank\n", BNKERR) ;
		return -1 ; // fail
	}
	
	//Wrap this for multi-threading
	
	pthread_mutex_lock(& daBank->lock) ; // lock so no-one can interfere
	
	for(int i = 0; i < ACCNUM; i++){
		if(daBank->accounts[i].name[0] == '\0')
			continue ;
		printf("%10s \t \x1b[2;33m%2.2f\x1b[0m ", daBank->accounts[i].name, daBank->accounts[i].currentBalance) ;
		if(daBank->accounts[i].inSession == TRUE)
			printf("\t IN SERVICE\n") ;
		else
			printf ("\t \n");
	}
	
	pthread_mutex_unlock(& daBank->lock) ;
	
	//Wrap this for multi-threading
	
	return 0; // success
}
