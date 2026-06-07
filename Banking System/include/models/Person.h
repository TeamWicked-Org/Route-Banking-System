#pragma once
#include "utils.h"
#include <string>

class Person
{
protected:
    int id;
    std::string name;
    std::string password;

public:
  // Constructors
    Person();
    Person(int id, std::string name, std::string password);

  // Setters
    void setName(std::string name);
    void setPassword(std::string password);

  // Getters
    int getId() const;
    std::string getName() const;
    std::string getPassword() const;

  // Display
    virtual void display() const;

  // Destructor
    virtual ~Person();
};