#pragma once
#include "Person.h"
class Client : public Person {

private:
	static int id;
	double balance;
	static vector<Client> clientsList;

public:


	// constructors
	Client();
	Client(string name, string password);

	// Setters  --> Caller must handle the validation himself
	void setName(string name);
	void setPassword(string password);
	void setBalance(double bal);
	static void setClientList(vector<Client> c);
	static void setGlobalID(int d);

	// Getters 
	string getName() const ;
	string getPassword() const ;
	int getID() const ;
	double getBalance() const ;
	static vector<Client>& getClientList();

	// Misc
	void display() const override;

	void deposit(double amount);

	void withdraw(double amount);

	void transferTo(double amount, Client& recipient);

	void checkBalance() const;

	static void decreaseStaticID() ;

};


