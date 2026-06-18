#pragma once 
#include "utils.h" 
#include <iostream> 
#include <string> 
using namespace std; 

class Person 
{ 
private:
    int id;
    string name;
    string password;
 
public:
  // Constructors 
    Person();
    Person(int id, string name, string password); // Parameterized
 
  // Setters 
    void setName(string name);
    void setID(int id); 
    void setPassword(string password); 
 
  // Getters 
    string getName() const; 
    string getPassword() const; 
    int getID() const; 
 
  // Display : Pure Virtual 
    virtual void display() const = 0;
 
}; 
 