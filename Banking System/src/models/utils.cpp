#include "../../include/models/utils.h"
#include "../../include/models/Client.h"
#include "../../include/models/Employee.h"
#include "../../include/models/Admin.h"

namespace utils {


	static char formattingDelimiter = '-';

	// Validation Class Section
	// ===========================
	// 
	// Username validator
	bool Validation::validateName(std::string name) {
		int contigiousSpaceCount = 0;
		validationConstraints vldcons = get_ValidationStruct();

		// size constraints validation
		if (name.empty() || name.size() < vldcons.minNameSize || name.size() > vldcons.maxNameSize) return false;

		// invalidate prefix and postfix spaces
		if (name[0] == ' ' || name[name.size() - 1] == ' ') return false;

		// invalidate non alphabetic characters and non spaces
		for (char c : name) {
			if ((c > 'Z' && c < 'a') || (c < 'A' && c != ' ') || (c > 'z')) return false;

			// invalidate contigious space input block i.e "  "
			{
				if (c == ' ') {
					contigiousSpaceCount++;
				}
				else
				{
					contigiousSpaceCount = 0;
				}

				if (contigiousSpaceCount >= 2)
					return false;
			}
		}
		return true;
	}
	// =================================================================================================================

	// Password validator
	bool Validation::validatePassword(std::string password) {
		validationConstraints vldcons = get_ValidationStruct();

		// size constraints validation
		if (password.empty() || password.size() < vldcons.minPasswordSize || password.size() > vldcons.maxPasswordSize)
			return false;

		// invalidate spaces
		for (char c : password) {
			if (c == ' ') return false;
		}
		return true;
	}
	// =================================================================================================================

	// Balance validator
	bool Validation::validateBalance(double bal) {
		validationConstraints vldcons = get_ValidationStruct();

		if (bal < vldcons.minBalance)
			return false;

		return true;
	}
	// =================================================================================================================

	// Salary validator
	bool Validation::validateSalary(double sal) {
		validationConstraints vldcons = get_ValidationStruct();

		if (sal < vldcons.minSalary)
			return false;

		return true;
	}

	// =================================================================================================================

	// Validation Struct Instance
	Validation::validationConstraints Validation::get_ValidationStruct() {
		validationConstraints valStruct;
		return valStruct;
	}
	// =========================== Validation Class

	// Security Class Section
	// ===========================

	// ── SHA-256 (raw bytes → 32-byte digest) ─────────────────────────────────
	std::vector<uint8_t> Security::sha256(const uint8_t* data, size_t len) {
		AlgHandle alg;
		(void)BCryptOpenAlgorithmProvider(
			&alg.h, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
		ULONG obj_sz = 0, dummy = 0;
		(void)BCryptGetProperty(alg.h, BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<PUCHAR>(&obj_sz), sizeof(obj_sz), &dummy, 0);
		std::vector<uint8_t> obj_buf(obj_sz);
		HashHandle hash;
		(void)BCryptCreateHash(
			alg.h, &hash.h, obj_buf.data(), obj_sz, nullptr, 0, 0);
		(void)BCryptHashData(hash.h,
			const_cast<PUCHAR>(data), static_cast<ULONG>(len), 0);
		std::vector<uint8_t> digest(32);
		(void)BCryptFinishHash(hash.h, digest.data(), 32, 0);
		return digest;
	}

	// String Hash
	std::string Security::stringHash(const std::string& str) {
		const uint8_t* data = reinterpret_cast<const uint8_t*>(str.data());
		std::vector<uint8_t> digest = sha256(data, str.size());

		std::ostringstream oss;
		for (uint8_t byte : digest)
			oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);

		return oss.str(); // 64-char hex string
	}
	// =========================== Security Class


	// FileHelper Class Section
	// ===========================

	// getters
	FileHelper::databaseFileNames FileHelper::get_DB_Struct() {
		databaseFileNames tmp;
		return tmp;
	}

	// Misc

	void FileHelper::saveLast(std::string IDS_DatabaseFile, int id) {		// will save the current static id to the file
		// ofstream uses RAII -> Resource Acquisition Is Initialization which mean it's destructor will automatically close the file
		// once the variable (id_file) get's out of scope

		ofstream id_file(IDS_DatabaseFile,  ios::binary);
		if (!id_file) return;
		id_file.write(
			// & is because the write function needs the address itself of the variable to copy it's value as bytes inside the file
			// and the variable is just a simple integer not an array that is get's already decayed as a pointer on passing to a function
			// if we need to pass an array i.e int[] arr the code will be as follows:
			// id_file.write(
			//		(char*)arr,
			//		sizeof(arr)
			// )
			// (char*)arr,	don't mess this with the normal character behavior, this line ISN'T casting the integer array to characters
			// it instead is getting the bytes from the starting address of the array as (char) type is also used to get raw bytes not
			// just representing characters (abc-z)
		    reinterpret_cast<const char*>(&id),
		    sizeof(id)
		);
	}
	
	int FileHelper::getLast(std::string IDS_DatabaseFile) {		// get's the last saved id to set the static id in memory to match it

		ifstream id_file(IDS_DatabaseFile, ios::binary);
		if (!id_file) return -1;
		int tmpID{};
		id_file.read(
			reinterpret_cast<char*>(&tmpID),
			sizeof(tmpID)
		);
		return tmpID;

	}
	
	void FileHelper::saveClient(Client c) {
		// get's database info from the file helper
		databaseFileNames filesStruct = get_DB_Struct();

		// opens the info DB in append mode to add a client at the end
		ofstream info_file(filesStruct.clientDB, ios::app);

		// do nothing if the file can't be opened
		if (!info_file) return;

		// saves the client in this format :
		// ID-Name-Password-Balance		then at last add a new line for later appending in file
		// 
		// On splitting we get this form:
		// idx[0] --> ID
		// idx[1] --> Name
		// idx[2] --> Password
		// idx[3] --> Balance
		info_file << c.getID() << utils::formattingDelimiter
				  << c.getName() << utils::formattingDelimiter
				  << c.getPassword() << utils::formattingDelimiter
				  << std::fixed << std::setprecision(2) << c.getBalance() << '\n';
	}
	
	void FileHelper::saveEmployee(Employee e) {
		// get's database info from the file helper
		databaseFileNames filesStruct = get_DB_Struct();

		// opens the info DB in append mode to add a employee at the end
		ofstream info_file(filesStruct.employeeDB, ios::app);

		// do nothing if the file can't be opened
		if (!info_file) return;

		// saves the client in this format :
		// ID-Name-Password-Position-Salary		then at last add a new line for later appending in file
		// 
		// On splitting we get this form:
		// idx[0] --> ID
		// idx[1] --> Name
		// idx[2] --> Password
		// idx[3] --> Position
		// idx[4] --> Salary
		info_file << e.getID() << utils::formattingDelimiter
			<< e.getName() << utils::formattingDelimiter
			<< e.getPassword() << utils::formattingDelimiter
			<< e.getRole() << utils::formattingDelimiter
			<< std::fixed << std::setprecision(2) << e.getSalary() << '\n';

	}
	
	
	void FileHelper::saveAdmin(Admin a) {
		// get's database info from the file helper
		databaseFileNames filesStruct = get_DB_Struct();

		// opens the info DB in append mode to add a employee at the end
		ofstream info_file(filesStruct.adminDB, ios::app);

		// do nothing if the file can't be opened
		if (!info_file) return;

		// saves the client in this format :
		// ID-Name-Password-Position-Salary		then at last add a new line for later appending in file
		// 
		// On splitting we get this form:
		// idx[0] --> ID
		// idx[1] --> Name
		// idx[2] --> Password
		// idx[3] --> Position
		// idx[4] --> Salary
		info_file << a.getID() << utils::formattingDelimiter
			<< a.getName() << utils::formattingDelimiter
			<< a.getPassword() << utils::formattingDelimiter
			<< a.getRole() << utils::formattingDelimiter
			<< std::fixed << std::setprecision(2) << a.getSalary() << '\n';

	}


	// Passed client is already updated
	void FileHelper::updateClient(vector<pair<Client, int>>& cVec) {
		// get's database info from the file helper
		databaseFileNames filesStruct = get_DB_Struct();

		// opens the info DB in append mode to add a employee at the end
		ifstream info_file(filesStruct.clientDB, ios::in);

		// do nothing if the file can't be opened
		if (!info_file) return;

		vector<string> lines;		//  used to rebuild the full file  (Old File Copy --> To Be Modified)
		string line;
		int counter = 0;
		while (getline(info_file,line))
		{
			lines.push_back(line);
			for (auto& item : cVec) {
				if (Parser::split(line)[0] == to_string(item.first.getID()))
				{
					item.second = counter;
				}
			}
			counter++;
		}

		info_file.close();
		stringstream s;

		for (auto& item : cVec) {
			s.str("");		// String stream must be cleared on every loop cycle
			s << item.first.getID() << "-" << item.first.getName() << "-" << item.first.getPassword() << "-" << std::fixed << std::setprecision(2) << item.first.getBalance(); // updated Client Data
			lines[item.second] = s.str();
		}

		// We have updated the old file copy, time to overwrite it on disk


		ofstream info_file2(filesStruct.clientDB,  ios::trunc);
		if (!info_file2)
			return;


		for (size_t i = 0; i < lines.size(); i++)
		{
			info_file2 << lines[i];
			info_file2 << "\n";
		}
		info_file2.close();
	}
	


	// Passed employee is already updated
	void FileHelper::updateEmployee(vector<pair<Employee, int>>& eVec) {
		// get's database info from the file helper
		databaseFileNames filesStruct = get_DB_Struct();

		// opens the info DB in append mode to add a employee at the end
		ifstream info_file(filesStruct.employeeDB, ios::in);

		// do nothing if the file can't be opened
		if (!info_file) return;

		vector<string> lines;		//  used to rebuild the full file  (Old File Copy --> To Be Modified)
		string line;
		int counter = 0;
		while (getline(info_file,line))
		{
			lines.push_back(line);
			for (auto& item : eVec) {
				if (Parser::split(line)[0] == to_string(item.first.getID()))
				{
					item.second = counter;
				}
			}
			counter++;
		}

		info_file.close();
		stringstream s;

		for (auto& item : eVec) {
			s.str("");		// String stream must be cleared on every loop cycle
			s << item.first.getID() << "-" << item.first.getName() << "-" << item.first.getPassword() << "-" << item.first.getRole() << "-" << std::fixed << std::setprecision(2) << item.first.getSalary(); // updated Employee Data
			lines[item.second] = s.str();
		}

		// We have updated the old file copy, time to overwrite it on disk


		ofstream info_file2(filesStruct.employeeDB,  ios::trunc);
		if (!info_file2)
			return;


		for (size_t i = 0; i < lines.size(); i++)
		{
			info_file2 << lines[i];
			info_file2 << "\n";
		}

		info_file2.close();
	}
	

	void FileHelper::fetchClients() {
		// get's database info from the file helper
		databaseFileNames filesStruct = get_DB_Struct();

		// if info file not found --> create it
		if (!std::filesystem::exists(filesStruct.clientDB)) {
			ofstream tmp(filesStruct.clientDB);
			tmp.close();
		}

		// opens the info DB in read mode to fetch all saved clients data
		ifstream info_file(filesStruct.clientDB, ios::in);

		// if file exists but can't be opened --> return
		if (!info_file) return; 
		string tmpLine;
		vector<Client> cTmpVec;
		while (getline(info_file, tmpLine))
		{
			cTmpVec.push_back(Parser::parseToClient(tmpLine));
		}
		Client::setClientList(cTmpVec);
		info_file.close();

		// if ID file not found --> create it and write -1 to it
		if (!std::filesystem::exists(filesStruct.clientID_DB)) {
			ofstream tmp(filesStruct.clientID_DB);
			tmp.close();
			FileHelper::saveLast(filesStruct.clientID_DB, -1);
		}

		// if ID file exists --> read the id and save it to memory static shared id if the function didn't return -1
		if (FileHelper::getLast(filesStruct.clientID_DB) != -1)
			Client::setGlobalID(FileHelper::getLast(filesStruct.clientID_DB));

	}
	void FileHelper::fetchEmployees() {
		// get's database info from the file helper
		databaseFileNames filesStruct = get_DB_Struct();

		// if info file not found --> create it
		if (!std::filesystem::exists(filesStruct.employeeDB)) {
			ofstream tmp(filesStruct.employeeDB);
			tmp.close();
		}

		// opens the info DB in read mode to fetch all saved employees data
		ifstream info_file(filesStruct.employeeDB, ios::in);

		// if file exists but can't be opened --> return
		if (!info_file) return;
		string tmpLine;
		vector<Employee> eTmpVec;
		while (getline(info_file, tmpLine))
		{
			eTmpVec.push_back(Parser::parseToEmployee(tmpLine));
		}
		Employee::setEmployeeList(eTmpVec);
		info_file.close();

		// if ID file not found --> create it and write -1 to it
		if (!std::filesystem::exists(filesStruct.employeeID_DB)) {
			ofstream tmp(filesStruct.employeeID_DB);
			tmp.close();
			FileHelper::saveLast(filesStruct.employeeID_DB, -1);
		}

		// if ID file exists --> read the id and save it to memory static shared id if the function didn't return -1
		if (FileHelper::getLast(filesStruct.employeeID_DB) != -1)
			Employee::setGlobalID(FileHelper::getLast(filesStruct.employeeID_DB));

	}
	void FileHelper::fetchAdmins() {
		// get's database info from the file helper
		databaseFileNames filesStruct = get_DB_Struct();

		// if info file not found --> create it
		if (!std::filesystem::exists(filesStruct.adminDB)) {
			ofstream tmp(filesStruct.adminDB);
			tmp.close();
		}

		// opens the info DB in read mode to fetch all saved employees data
		ifstream info_file(filesStruct.adminDB, ios::in);

		// if file exists but can't be opened --> return
		if (!info_file) return;
		string tmpLine;
		vector<Admin> aTmpVec;
		while (getline(info_file, tmpLine))
		{
			aTmpVec.push_back(Parser::parseToAdmin(tmpLine));
		}
		Admin::setAdminList(aTmpVec);
		info_file.close();

	}
	void FileHelper::clearInfoFile(std::string filename) {	// Clears specific file sent as parameter
		ofstream file_to_clear(filename, ios::trunc);
		file_to_clear.close();
	}
	void FileHelper::clearIDFile(std::string filename) {	// Clears specific file sent as parameter
		FileHelper::saveLast(filename, -1);
	}



	// =========================== FileHelper Class


	// Parser Class Section
	// =====================


	vector<std::string> Parser::split(std::string& line) {
		// Create tmp vector of type string
		vector<std::string> tmpVec;

		// create string stream variable from the input string
		stringstream ss(line);

		// create token string variable to store each single read string from the line separated by the delimiter i.e 1-Mohamed-51asd6as-25415
		// so on looping token is set to 1 --> Mohamed --> 51asd6as --> 25415
		std::string token;
		while (getline(ss, token, utils::formattingDelimiter))
		{
			// push each single token to the vector
			tmpVec.push_back(token);
		}
		// returns the vector that's built in the following form:
		// tmpVec[0] = 1
		// tmpVec[1] = Mohamed
		// tmpVec[2] = 51asd6as
		// tmpVec[3] = 25415
		return tmpVec;
	}

	Client Parser::parseToClient(std::string& line) {
		// calls the split function to receive a string vector
		vector<std::string> initStrVec = split(line);

		// creates a new client using it's paramiterized constructor
		// construct with a dummy password (constructor will hash it, then we overwrite below)
		Client newClient(initStrVec[1], "placeholder");


		//	the Client's class paramiterized constructor automatically calls Person's Constructor with an increased static id
		//	Client::Client(string name, string password) : Person(++Client::id, name, password) {
		//		balance = 0;
		//	}
		//  and balance is automatically set to 0 so we need to change all the default values and decrease the global shared id
		//	on creating a new Client here
		
		newClient.setPasswordRaw(initStrVec[2]);	// restore the real stored hash, unhashed-again
		newClient.setBalance(stod(initStrVec[3]));
		newClient.setID(stoi(initStrVec[0]));
		Client::decreaseStaticID();
		return newClient;
	}

	Employee Parser::parseToEmployee(std::string& line) {
		// calls the split function to receive a string vector
		vector<std::string> initStrVec = split(line);

		// creates a new employee using it's paramiterized constructor
		Employee newEmp(initStrVec[1], "placeholder", initStrVec[3],0.0);

		//	the Employee's class paramiterized constructor automatically calls Person's Constructor with an increased static id
		//	Employee::Employee(string name, string password,
		//		string position, double salary)
		//		: Person(++Employee::id, name, password),
		//		position(position), salary(salary) {}
		// 
		//  so we need to change all values and decrease the global shared id
		//	on creating a new Employee here
		newEmp.setPasswordRaw(initStrVec[2]);	// restore the real stored hash, unhashed-again
		newEmp.setSalary(stod(initStrVec[4]));
		newEmp.setID(stoi(initStrVec[0]));
		Employee::decreaseStaticID();
		return newEmp;
	}

	Admin Parser::parseToAdmin(std::string& line) {
		// calls the split function to receive a string vector
		vector<std::string> initStrVec = split(line);

		// creates a new admin using it's paramiterized constructor
		Admin newAdmin(initStrVec[1], "placeholder");

		// Admin uses the same employee structure but with fixed position (Administrator) and 0.0 Salary
		// so all we need is setting the ID and decrease the Employee shared static ID by 1
		newAdmin.setPasswordRaw(initStrVec[2]);	// restore the real stored hash, unhashed-again
		newAdmin.setID(stoi(initStrVec[0]));
		Employee::decreaseStaticID();
		return newAdmin;
	}


	// =========================== Parser Class

}