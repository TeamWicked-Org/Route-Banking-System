#include "../../include/models/Person.h" 
 
// Constructors initializer list 
Person::Person() : id(0), name(""), password(""){} 
 
Person::Person(int id, string name, string password) : id(id), name(name), password(utils::Security::stringHash(password)) {} 
 
 
void Person::setName(string name) 
{ 
    this->name = name; 
} 
 
void Person::setID(int id) 
{ 
    this->id = id; 
} 
 
void Person::setPassword(string password) 
{ 
    this->password = utils::Security::stringHash(password); 
} 

void Person::setPasswordRaw(string hashedPassword)
{
    this->password = hashedPassword;
}
 
string Person::getName() const 
{ 
    return name; 
} 
 
string Person::getPassword() const 
{ 
    return password; 
} 
 
int Person::getID() const 
{ 
    return id; 
}