#include "../../include/models/Employee.h"


// Static ID counter
int Employee::id = 0;
vector<Employee> Employee::employeeList{};

// constructors
Employee::Employee()
	: Person(0, "", ""), salary(0.0), position("") {}

Employee::Employee(string name, string password,
	string position, double salary)
	: Person(++Employee::id, name, password),
	position(position), salary(salary) {}

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

void Employee::setPosition(string position) {
	this->position = position;
}

void Employee::setEmployeeList(vector<Employee> e) {
	employeeList = e;
}


void Employee::setGlobalID(int d) {
	id = d;
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
	return position;
}

vector<Employee> Employee::getEmployeeList() const {
	return employeeList;
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
