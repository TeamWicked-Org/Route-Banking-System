#include <iostream>
#include <string>
#include "include/models/Person.h"
using namespace std;

int main()
{
    // Validation Name Tests
    bool hey = Validation::validateName("Black Myth Wukong");
    cout << (hey ? "Name Valid!" : "Invalid Name!") << endl;

    hey = Validation::validateName(" Black Myth Wukong");
    cout << (hey ? "Name Valid!" : "Invalid Name!") << endl;

    hey = Validation::validateName("Black Myth Wukong ");
    cout << (hey ? "Name Valid!" : "Invalid Name!") << endl;

    hey = Validation::validateName("         ");
    cout << (hey ? "Name Valid!" : "Invalid Name!") << endl;

    // Validation Password Tests
    hey = Validation::validatePassword("Djkasbkj$@!#");
    cout << (hey ? "Password Valid!" : "Invalid Password!") << endl;

    hey = Validation::validatePassword("Djkasbkj$@!# ");
    cout << (hey ? "Password Valid!" : "Invalid Password!") << endl;

    hey = Validation::validatePassword(" Djkasbkj$@!#");
    cout << (hey ? "Password Valid!" : "Invalid Password!") << endl;

    hey = Validation::validatePassword("Djkasbk j$@!#");
    cout << (hey ? "Password Valid!" : "Invalid Password!") << endl;

    return 0;
}