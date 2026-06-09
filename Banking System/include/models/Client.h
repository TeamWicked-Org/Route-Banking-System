#pragma once
#include "Person.h"
class Client : public Person {
private:
	static int id;
	double balance;
public:

	// constructors
	Client();
	Client(std::string name, std::string password);

	// Setters  --> Caller must handle the validation himself
	void setName(std::string name);
	void setPassword(std::string password);
	void setBalance(double bal);

	// Getters 
	std::string getName() const override;
	std::string getPassword() const override;
	int getID() const override;
	double getBalance() const;

	// Misc
	void display() const override;

	void deposit(double amount);

	void withdraw(double amount);

	void transferTo(double amount, Client& recipient);

	void checkBalance() const;

};


