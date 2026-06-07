#include "../../include/models/Person.h"

Person::Person()
{
    id = 0;
    name = "";
    password = "";
}

Person::Person(int id, std::string name, std::string password)
{
    this->id = id;
    setName(name);
    setPassword(password);
}

void Person::setName(std::string name)
{
    if (utils::Validation::validateName(name))
        this->name = name;
}

void Person::setPassword(std::string password)
{
    if (utils::Validation::validatePassword(password))
    {
        this->password = utils::Security::stringHash(password);
    }
}

int Person::getId() const
{
    return id;
}

std::string Person::getName() const
{
    return name;
}

std::string Person::getPassword() const
{
    return password;
}

void Person::display() const
{
    std::cout << "ID: " << id << std::endl;
    std::cout << "Name: " << name << std::endl;
}

Person::~Person()
{
}