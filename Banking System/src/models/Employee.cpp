#include "../../include/models/Employee.h"

// constructors
Employee::Employee() : Person(/*ID*/ 0,/*name*/"",/*password*/"") {
	salary = 0;
}

Employee::Employee(std::string name, std::string password) : Person(++Employee::id, name, password) {
	salary = 0;
}

// Setters  --> Caller must handle the validation himself
void Employee::setName(std::string name) {
	Person::setName(name);
}

void Employee::setPassword(std::string password) {
	Person::setPassword(password);
}

void Employee::setsalary(double salary) {
	this->salary = salary;
}

// Getters 
std::string Employee::getName() const
{
	return Person::getName();
}

std::string Employee::getPassword() const
{
	return Person::getPassword();
}

int Employee::getID() const
{
	return Person::getID();
}

double Employee::getsalary() const {
	return salary;
}

// Misc
void Employee::display() const {
	cout << "ID: " << getID() << endl;
	cout << "Name: " << getName() << endl;
	cout << "Salary: " << salary << endl;
}
