#include "../../include/models/Admin.h"

Admin::Admin()
    : Employee() {}

// Admin uses Employee's ID counter, role is fixed as "Administrator"
Admin::Admin(string name, string password)
    : Employee(name, password, "Administrator", 0.0) {}

void Admin::display() const {
    cout << "Admin ID: " << getID() << endl;
    cout << "Admin Name: " << getName() << endl;
    cout << "Admin Role: " << getRole() << endl;
}