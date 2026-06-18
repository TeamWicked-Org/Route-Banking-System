#pragma once
#include "Person.h"
class Employee : public Person {

private:
	static int id;
	double salary;
	string position;

public:

	// constructors
	Employee();
	Employee(string name, string password,
			 string position, double salary);

	// Setters  --> Caller must handle the validation himself
	void setName(string name);
	void setPassword(string password);
	void setSalary(double salary);
	void setPosition(string position);

	// Getters
	string getName() const;
	string getPassword() const;
	int getID() const;
	double getSalary() const;
	string getRole() const;

	// Misc
	void display() const override;

	static void decreaseStaticID();

};
