#pragma once
#include "Person.h"
class Employee : public Person {

private:
	static int id;
	double salary;
	string position;
	static vector<Employee> employeeList;

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
	static void setEmployeeList(vector<Employee> e);
	static void setGlobalID(int d);

	// Getters
	string getName() const;
	string getPassword() const;
	int getID() const;
	double getSalary() const;
	string getRole() const;
	static vector<Employee> getEmployeeList();
	static int getGlobalID();

	// Misc
	void display() const override;

	static void decreaseStaticID();

};
