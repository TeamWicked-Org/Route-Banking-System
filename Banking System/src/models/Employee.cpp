#include "../../include/models/Employee.h"

// constructors
Employee::Employee()
	: Person(0, "", ""), salary(0.0), role("") {}

Employee::Employee(string name, string password,
	string role, double salary)
	: Person(++Employee::id, name, password),
	role(role), salary(salary) {}

// Setters  --> Caller must handle the validation himself
void Employee::setName(string name) {
	Person::setName(name);
}

void Employee::setPassword(string password) {
	Person::setPassword(password);
}

void Employee::setSalary(double salary) {
	this->salary = salary;
}

void Employee::setRole(string role) {
	this->role = role;
}

// Getters 
string Employee::getName() const
{
	return Person::getName();
}

string Employee::getPassword() const
{
	return Person::getPassword();
}

int Employee::getID() const
{
	return Person::getID();
}

double Employee::getSalary() const {
	return salary;
}

string Employee::getRole() const {
	return role;
}

// Misc
void Employee::display() const {
	cout << "ID: " << getID() << endl;
	cout << "Name: " << getName() << endl;
	cout << "Salary: " << salary << endl;
}


void Employee::decreaseStaticID() {
	Employee::id -= 1;
}
