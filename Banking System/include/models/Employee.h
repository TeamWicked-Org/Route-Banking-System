#pragma once
#include "Person.h"
class Employee : public Person {

private:
	static int id;
	double salary;

public:

	// constructors
	Employee();
	Employee(std::string name, std::string password);

	// Setters  --> Caller must handle the validation himself
	void setName(std::string name);
	void setPassword(std::string password);
	void setsalary(double salary);

	// Getters 
	std::string getName() const override;
	std::string getPassword() const override;
	int getID() const override;
	double getsalary() const;

	// Misc
	void display() const override;

};
