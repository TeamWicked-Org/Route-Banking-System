#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#include <string>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <iostream> 
#include <filesystem> 
using namespace std;

// includes dependency library for #include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

// Forward declaration to build the functions signature
class Client;
class Employee;
class Admin;

// Utils Namespace
namespace utils {


	// Validation Class
	// ====================================
	class Validation final {
	private:
		const struct validationConstraints {
			// name constraints
			int minNameSize = 3;
			int maxNameSize = 20;

			// password constraints
			int minPasswordSize = 8;
			int maxPasswordSize = 20;

			// cash constraints
			double minBalance = 1500;
			double minSalary = 5000;
		};

	public:
		// delete constructors to prevent instance creations
		Validation() = delete;

		// delete copy constructor
		Validation(const Validation&) = delete;

		// delete assingment operator
		Validation& operator=(const Validation&) = delete;

		static bool validateName(std::string name);
		static bool validatePassword(std::string pass);
		static bool validateBalance(double bal);
		static bool validateSalary(double sal);

		static validationConstraints get_ValidationStruct();
	};

	//
	// Security Class
	// ====================================

	class Security final {
	private:
		static std::vector<uint8_t> sha256(const uint8_t* data, size_t len);

	public:

        struct AlgHandle {
            BCRYPT_ALG_HANDLE h = nullptr;
            AlgHandle() = default;
            AlgHandle(const AlgHandle&) = delete;
            AlgHandle& operator=(const AlgHandle&) = delete;
            AlgHandle(AlgHandle&& o) noexcept : h(o.h) { o.h = nullptr; }
            AlgHandle& operator=(AlgHandle&& o) noexcept {
                if (this != &o) {
                    if (h) BCryptCloseAlgorithmProvider(h, 0);
                    h = o.h; o.h = nullptr;
                }
                return *this;
            }
            ~AlgHandle() { if (h) BCryptCloseAlgorithmProvider(h, 0); }
        };

        struct HashHandle {
            BCRYPT_HASH_HANDLE h = nullptr;
            HashHandle() = default;
            HashHandle(const HashHandle&) = delete;
            HashHandle& operator=(const HashHandle&) = delete;
            HashHandle(HashHandle&& o) noexcept : h(o.h) { o.h = nullptr; }
            HashHandle& operator=(HashHandle&& o) noexcept {
                if (this != &o) {
                    if (h) BCryptDestroyHash(h);
                    h = o.h; o.h = nullptr;
                }
                return *this;
            }
            ~HashHandle() { if (h) BCryptDestroyHash(h); }
        };

		// delete constructors to prevent instance creations
		Security() = delete;

		// delete copy constructor
		Security(const Security&) = delete;

		// delete assingment operator
		Security& operator=(const Security&) = delete;

		static std::string stringHash(const std::string& str);

	};



	//
	// FileHelper Class
	// ====================================

	class FileHelper final {

	private:
		struct databaseFileNames {
			// Clients
			std::string clientDB = "Clients.txt";
			std::string clientID_DB = "Clients_ids.bin";

			// Employees
			std::string employeeDB = "Employees.txt";
			std::string employeeID_DB = "Employees_ids.bin";

			// Admins
			std::string adminDB = "Admins.txt";

		};


	public:

		// delete constructors to prevent instance creations
		FileHelper() = delete;

		// delete copy constructor
		FileHelper(const FileHelper&) = delete;

		// delete assingment operator
		FileHelper& operator=(const FileHelper&) = delete;

		// getters
		static databaseFileNames get_DB_Struct();

		// Misc

		static void saveLast(std::string IDS_DatabaseFile, int id);		// will save the current static id to the file (will use binary mode as a simple anti-modification layer)

		static int getLast(std::string IDS_DatabaseFile);	// get's the last saved id to set the static id in memory to match it

		static void saveClient(Client c);

		static void saveEmployee(Employee e);

		static void saveAdmin(Admin a);

		static void updateClient(vector<pair<Client,int>>& cVec);

		static void updateEmployee(vector<pair<Employee,int>>& eVec);

		static void updateAdmin(vector<pair<Admin,int>>& aVec);

		static void removeAllClients();
		static void removeAllEmployees();
		static void removeAllAdmins();

		static void removeClient(vector<pair<Client,int>> cVec);
		static void removeEmployee(vector<pair<Employee, int>> eVec);
		static void removeAdmin(vector<pair<Admin, int>> aVec);


		static void fetchClients();
		static void fetchEmployees();
		static void fetchAdmins();
		static void clearInfoFile(std::string filename);			// Clears specific file sent as parameter
		static void clearIDFile(std::string filename);			// Clears specific file sent as parameter


	};


	//
	// Parser Class
	// ====================================

	class Parser {
	private:

	public:

		// delete constructors to prevent instance creations
		Parser() = delete;

		// delete copy constructor
		Parser(const Parser&) = delete;

		// delete assingment operator
		Parser& operator=(const Parser&) = delete;


		static vector<std::string> split(std::string& line);
		static Client parseToClient(std::string& line);
		static Employee parseToEmployee(std::string& line);
		static Admin parseToAdmin(std::string& line);



	};

}  // namespace utils
