#pragma once
#include "Employee.h"

class Admin : public Employee {
public:
    // Constructors
    Admin();
    Admin(std::string name, std::string password);

};