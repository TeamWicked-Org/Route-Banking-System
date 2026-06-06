#include <iostream>
#include <string>
#include "include/models/Person.h"
using namespace std;

int main()
{
    // Validation Name Tests
    bool hey = utils::Validation::validateName("Black Myth Wukong");
    cout << (hey ? "Name Valid!" : "Invalid Name!") << endl;

    hey = utils::Validation::validateName(" Black Myth Wukong");
    cout << (hey ? "Name Valid!" : "Invalid Name!") << endl;

    hey = utils::Validation::validateName("Black Myth Wukong ");
    cout << (hey ? "Name Valid!" : "Invalid Name!") << endl;

    hey = utils::Validation::validateName("         ");
    cout << (hey ? "Name Valid!" : "Invalid Name!") << endl;

    // Validation Password Tests
    hey = utils::Validation::validatePassword("Djkasbkj$@!#");
    cout << (hey ? "Password Valid!" : "Invalid Password!") << endl;

    hey = utils::Validation::validatePassword("Djkasbkj$@!# ");
    cout << (hey ? "Password Valid!" : "Invalid Password!") << endl;

    hey = utils::Validation::validatePassword(" Djkasbkj$@!#");
    cout << (hey ? "Password Valid!" : "Invalid Password!") << endl;

    hey = utils::Validation::validatePassword("Djkasbk j$@!#");
    cout << (hey ? "Password Valid!" : "Invalid Password!") << endl;

    // this hash is just to save password as hash 64-character string instead of plain text in database
    string hashTesting = utils::Security::stringHash("Secure Password!");
    cout << hashTesting << endl;

    return 0;
}