#include "../../include/utilities/Validation.h"

// Username validator
 bool Validation::validateName(std::string name) {
	int contigiousSpaceCount = 0;
	Validation::validationConstraints vldcons;

	// size constraints validation
	if (name.empty() || name.size() < vldcons.minNameSize || name.size() > vldcons.maxNameSize) return false;

	// invalidate prefix and postfix spaces
	if ( name[0] == ' ' || name[name.size() - 1] == ' ') return false;

	// invalidate non alphabetic characters and non spaces
	for (char c : name) {
		if ((c > 'Z' && c < 'a') || (c < 'A' && c != ' ') || (c > 'z')) return false;
		
		// invalidate contigious space input block i.e "  "
		{
			if (c == ' ') {
				contigiousSpaceCount++;
			}
			else
			{
				contigiousSpaceCount = 0;
			}

			if (contigiousSpaceCount >= 2)
				return false;
		}
	}
	return true;
 }
 // =================================================================================================================

 // Password validator
 bool Validation::validatePassword(std::string password) {
	 Validation::validationConstraints vldcons;

	 // size constraints validation
	 if (password.empty() || password.size() < vldcons.minPasswordSize || password.size() > vldcons.maxPasswordSize)
		 return false;

	 // invalidate spaces
	 for (char c : password) {
		 if (c == ' ') return false;
	 }
	 return true;
 }
 // =================================================================================================================

 // Balance validator
 bool Validation::validateBalance(double bal) {
	 Validation::validationConstraints vldcons;

	 if (bal < vldcons.minBalance)
		 return false;

	 return true;
 }
 // =================================================================================================================

 // Salary validator
 bool Validation::validateSalary(double sal) {
	 Validation::validationConstraints vldcons;

	 if (sal < vldcons.minSalary)
		 return false;

	 return true;
 }