#pragma once
#include "Employee.h"

class Admin : public Employee {
    static vector<Admin> adminList;

public:
    // Constructors
    Admin();
    Admin(std::string name, std::string password);


    static void setAdminList(vector<Admin> a);
    static vector<Admin> getAdminList() ;

};
