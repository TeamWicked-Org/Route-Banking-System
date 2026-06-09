#include "include/models/Client.h"
#include <vector>
#include <random>

// Populate client ids
int Client::id = 0;

// Random Generator
double randomInt(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}



int main()
{

    //string passwordInput;
    //cin >> passwordInput;
    //while (!utils::Validation::validatePassword(passwordInput))
    //{
    //    cin >> passwordInput;
    //}


    // Ploymorphism Implementation
    vector<Client*> clientTestVector;
    int testClientsCount = 5;

    cout << "Clients Credentials:\n";
    cout << "=======================\n";
    for (int i = 0; i < testClientsCount; i++)
    {
        if (i % 2 == 0)
        {
            clientTestVector.push_back(new Client("Rahma0" + to_string(i + 1), "Bugatti0" + to_string(i + 1)));
        }
        else
        {
            clientTestVector.push_back(new Client("Mohamed0" + to_string(i+1), "Bugatti0" + to_string(i+1)));
        }
    }

    for (int i = 0; i < testClientsCount; i++)
    {
        clientTestVector[i]->display();
    }
    cout << "======================================================================================\n";
    cout << "Clients Balance Deposition & Printing\n";
    cout << "======================================\n";
    for (int i = 0; i < testClientsCount; i++)
    {

        clientTestVector[i]->setBalance(randomInt(10000,20000));
        clientTestVector[i]->checkBalance();
    }

    cout << "======================================================================================\n";
    cout << "Clients Balance After Some Transfers\n";
    cout << "=======================================\n";
    for (int i = 0; i < testClientsCount; i++)
    {
        if (i % 2 == 0)
        {
            clientTestVector[i]->transferTo(500.0, *clientTestVector[(i+1) % testClientsCount]);
        }
    }
    for (int i = 0; i < testClientsCount; i++)
    {
        clientTestVector[i]->checkBalance();
    }
    cout << "======================================================================================\n";
    // Memory Cleanup
    for (int i = 0; i < testClientsCount; i++)
    {
        Client* tmp;
        tmp = clientTestVector[i];
        clientTestVector.pop_back();
        delete tmp;
    }


    return 0;
}