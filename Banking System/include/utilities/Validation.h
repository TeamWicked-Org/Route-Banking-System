#pragma once
#include <string>

// this is a way to do it using class but in such case a namespace would be more efficient and easier to maintain as we don't need "Class" OOP design
// so that's what we gonna do
//class Validation final {
//private:
//
//public:
//	// delete constructors to prevent instance creations
//	Validation() = delete;
// 
//	// delete copy constructor
//	Validation(const Validation&) = delete;
// 
//	// delete assingment operator
//	Validation& operator=(const Validation&) = delete;
//
//	static bool validateName(std::string name);
//	static bool validatePassword(std::string pass);
//	static bool validateBalance(double bal);
//	static bool validateSalary(double sal);
//};

// Note: Under namespace we don't need static for scope resolution calls at all
namespace Validation {
	struct validationConstraints {
		// name constraints
		int minNameSize = 3;
		int maxNameSize = 20;

		// password constraints
		int minPasswordSize = 8;
		int maxPasswordSize = 20;

		// cash constraints
		double minBalance = 1500;
		double minSalary = 5000;
	};

	bool validateName(std::string name);
	bool validatePassword(std::string password);
	bool validateBalance(double bal);
	bool validateSalary(double sal);

}
