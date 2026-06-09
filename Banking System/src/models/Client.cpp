#include "../../include/models/Client.h"


// constructors
Client::Client() : Person(/*ID*/ 0,/*name*/"",/*password*/"") {
	balance = 0;
}

Client::Client(std::string name, std::string password) : Person(++Client::id, name, password) {
	balance = 0;
}


// Setters  --> Caller must handle the validation himself
void Client::setName(std::string name) {
	Person::setName(name);
}

void Client::setPassword(std::string password) {
	Person::setPassword(utils::Security::stringHash(password));
}

void Client::setBalance(double bal) {
	balance = bal;
}


// Getters 
std::string Client::getName() const
{
	return Person::getName();
}

std::string Client::getPassword() const
{
	return Person::getPassword();
}

int Client::getID() const
{
	return Person::getID();
}

double Client::getBalance() const {
	return balance;
}

// Misc
void Client::display() const {
	cout << "Client ID: " << getID() << endl;
	cout << "Client Name: " << getName() << endl;
	cout << "Client Pasword Hash: " << getPassword() << endl;
	return;
}

void Client::deposit(double amount) {
	balance += amount;
}

void Client::withdraw(double amount) {
	if (balance - amount >= 0 )
	{
		balance -= amount;
	}
}

void Client::transferTo(double amount, Client& recipient) {
	if (balance - amount >= 0 )
	{
		balance -= amount;
		recipient.deposit(amount);
	}
}

void Client::checkBalance() const {
	cout << "Client ID: " << getID() << endl;
	cout << "Client Name: " << getName() << endl;
	cout << "Client Balance: " << getBalance() << endl;
}