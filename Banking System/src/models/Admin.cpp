#include "../../include/models/Admin.h"

vector<Admin> Admin::adminList{};

Admin::Admin()
    : Employee() {}

// Admin uses Employee's ID counter, position is fixed as "Administrator"
Admin::Admin(string name, string password)
    : Employee(name, password, "Administrator", 0.0) {}



void Admin::setAdminList(vector<Admin> a) {
    adminList = a;
}


vector<Admin> Admin::getAdminList() {
    return adminList;
}
