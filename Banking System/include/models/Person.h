#pragma once 
#include "utils.h" 
#include <iostream> 
#include <string> 
using namespace std; 
 
class Person 
{ 
private: 
    int id; 
    std::string name; 
    std::string password; 
 
public: 
  // Constructors 
    Person(); 
    Person(int id, std::string name, std::string password); 
 
  // Setters 
    void setName(std::string name); 
    void setID(int id); 
    void setPassword(std::string password); 
 
  // Getters 
    virtual std::string getName() const; 
    virtual std::string getPassword() const; 
    virtual int getID() const; 
 
  // Display : Pure Virtual 
    virtual void display() const = 0; 
 
}; 
 