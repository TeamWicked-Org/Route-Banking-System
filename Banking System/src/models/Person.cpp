#include "../../include/models/Person.h" 
 
// Constructors initializer list 
Person::Person() : id(0), name(""), password(""){} 
 
Person::Person(int id, std::string name, std::string password) : id(id), name(name), password(utils::Security::stringHash(password)) {} 
 
 
void Person::setName(std::string name) 
{ 
    this->name = name; 
} 
 
void Person::setID(int id) 
{ 
    this->id = id; 
} 
 
void Person::setPassword(std::string password) 
{ 
    this->password = utils::Security::stringHash(password); 
} 
 
std::string Person::getName() const 
{ 
    return name; 
} 
 
std::string Person::getPassword() const 
{ 
    return password; 
} 
 
int Person::getID() const 
{ 
    return id; 
}