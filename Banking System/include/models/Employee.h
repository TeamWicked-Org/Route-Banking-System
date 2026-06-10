#pragma once
#include "Person.h"
class Employee : public Person {

private:
	double salary;

public:
	void setsalary(double salary);
	double getsalary();
	void display();

};

