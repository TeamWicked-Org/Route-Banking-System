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


// Renad Forgot the following:
// 1- Constructors
// 2- Employee info getters
// 3- Employee info setters (Username + passwords)
// 4- display don't have override keyword
// 5- Static Employee's id counter to be viewable and shared across all Employee instances
// 6- Salary checking didn't use utils::Validation::validateSalary from utils class
// 7- Salary checking will be done by the caller not inside the function
