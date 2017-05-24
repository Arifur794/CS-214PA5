Arifur Rahman 
Systems Programming PA5

In PA5 I have implemented a multithreaded bank system that includes a client and a server. Implementation of the data structure designs and client/server designs are described below.

Data structures that were used:

The Bank: bank.h bank.c 
I used these two structs to implement the bank (X and Y)
	X) Account Struct
		This struct will contain the name for the account, current balance , and  an 			indicator to let the user know if the account is IN SESSION or not. There is also a 		mutex lock which guarantees that only one client can access the account at a 			time.  
	
	Accounts can be 
			1) open (open accountname)- this will initialize a new account with the 				desired account name
			2) balance - the current balance of the account is returned
			3) credit (credit amount)- the indicated amount is added to the 					current balance of the account
			4) debit (debit amount)- the indicated amount of money is subtracted 				from the current balance unless current balance falls below 0 
			5) start (start accountname)- this will start a customer session for a 				specific amount.
	Y) Bank Struct
		This struct contains a list of all accounts ( which has a max of 20 ), the number of present accounts that are active, and there is also a mutex lock on the bank to allow for printing accounts and adding accounts without causing discrepancies in the data of the banks.
			

			1)printing (printAccounts)- the given bank's mutex is locked so that no updates can be made to the bank during the print of the bank. All accounts in the bank are traversed and their name, balance, and servicing status are printed. There is no lock required to be placed on the account since data is only being read from the account and not altered. When the print process is completed, the bank's mutex is unlocked.
			2)adding account (addAccount)- Since the bank's mutex is locked, no other accounts can be added or the bank cannot be printed while adding an account. 

  The bank is then checked to make sure that the given account name is not present in the bank already. If not, the account is added to the bank and then the bank's mutex is unlocked. 

A removeAccount function was implemented for tessting and debugging, however this functionality is not provided by the server.
	

The Client: client.c

The client serves as a customer for the bank. It has two simple threads that are used 
to communicate with the server:
	1)read_input thread: reads the input from the command line and sends data to the 	server until the server closes or the user inputs quit. 
	2)print_output thread: reads the response sent from the server and outputs to the 	console.
						   
This thread returns when the server is no longer in service or if quit is called and server disconnects from the client. 
The main method is joined with the print_output thread and terminates when print_output returns



The Server: server.c

The server uses the banking system implemented in bank.h and bank.c. It also contains two threads:
	1)printBank thread: prints the bank every 20 seconds, using sleep(20) to wait on the 	print
	2)client_session_thread: reads the requests sent by the client and performs the 	corresponding bank actions.
							
 Responses on success/failure are then returned to the client and also printed on the server side.
							 
A new client_session_thread is spawned each time a new client connects to the server.
							 
When a client attempts to start an account, if the account is already in service, the thread will sleep and check every few seconds in an attempt to service the account.
After execution, a bank object is created and maintained throughout the duration of being connected to the server. 

***The server code has also implemented the repeated 2-second attempt at accessing an account if it is currently in use.In order to implement this functionality, the account struct being served is kept track of and is locked with a mutex. If being served, the mutex is locked for the duration of service or until the client manipulating the account quits from the bank in which case the mutex is then unlocked so another client  may access the account.***