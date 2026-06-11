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
    Person(int id, string name, string password); 
 
  // Setters 
    void setName(string name);
    void setID(int id); 
    void setPassword(string password); 
 
  // Getters 
    virtual string getName() const; 
    virtual string getPassword() const; 
    virtual int getID() const; 
 
  // Display : Pure Virtual 
    virtual void display() const = 0; 
 
}; 
 