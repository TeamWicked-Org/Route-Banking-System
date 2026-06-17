#pragma once
#include "Person.h"
class Employee : public Person {

private:
	static int id;
	double salary;
	string role;

public:

	// constructors
	Employee();
	Employee(string name, string password,
			 string role, double salary);

	// Setters  --> Caller must handle the validation himself
	void setName(string name);
	void setPassword(string password);
	void setSalary(double salary);
	void setRole(string role);

	// Getters 
	string getName() const override;
	string getPassword() const override;
	int getID() const override;
	double getSalary() const;
	string getRole() const;

	// Misc
	void display() const override;

	static void decreaseStaticID();

};
