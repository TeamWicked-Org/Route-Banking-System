#include "../../include/models/Admin.h"

Admin::Admin()
    : Employee() {}

// Admin uses Employee's ID counter, position is fixed as "Administrator"
Admin::Admin(string name, string password)
    : Employee(name, password, "Administrator", 0.0) {}

