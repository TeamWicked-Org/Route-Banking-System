#include "../../include/models/utils.h"
#include "../../include/models/Client.h"
#include "../../include/models/Employee.h"
#include "../../include/models/Admin.h"

namespace utils {


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

	// set userType for clear all records function
	static FileHelper::userType        current_Type = FileHelper::userType::Client;

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
		    reinterpret_cast<const char*>(&id),			// & is because the write function needs the address itself of the variable to copy it's value as bytes inside the file
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
		databaseFileNames filesStruct = get_DB_Struct();
		ofstream info_file(filesStruct.clientDB, ios::app);
		if (!info_file) return;
		info_file << c.getID() << filesStruct.formattingDelimiter
				  << c.getName() << filesStruct.formattingDelimiter
				  << c.getPassword() << filesStruct.formattingDelimiter
				  << std::fixed << std::setprecision(2) << c.getBalance() << '\n';
	}
	
	void FileHelper::saveEmployee(Employee e) {
		databaseFileNames filesStruct = get_DB_Struct();
		ofstream info_file(filesStruct.employeeDB, ios::app);
		if (!info_file) return;
		info_file << e.getID() << filesStruct.formattingDelimiter
			<< e.getName() << filesStruct.formattingDelimiter
			<< e.getPassword() << filesStruct.formattingDelimiter
			<< std::fixed << std::setprecision(2) << e.getSalary() << '\n';

	}
	
	void FileHelper::fetchClients() {
		databaseFileNames filesStruct = get_DB_Struct();
		ifstream info_file(filesStruct.clientDB, ios::in);
		if (!info_file) { ofstream createIfNotFound(filesStruct.clientDB, ios::out); return; }
		string tmpLine;
		vector<Client> cTmpVec;
		while (getline(info_file, tmpLine))
		{
			cTmpVec.push_back(Parser::parseToClient(tmpLine));
		}
		Client::setClientList(cTmpVec);
		info_file.close();
		if (FileHelper::getLast(filesStruct.clientID_DB) != -1)
			Client::setGlobalID(FileHelper::getLast(filesStruct.clientID_DB));

	}
	void FileHelper::fetchEmployees() {

	}
	void FileHelper::fetchAdmins() {

	}
	void FileHelper::clearFile(std::string filename) {	// Clears specific file sent as parameter
		fstream file_to_clear(filename, ios::trunc);
		file_to_clear.close();
	}
	void FileHelper::clearAllRecords(userType uT) {		// Clears all User type Record Files
	
	
	}


	// =========================== FileHelper Class


	// Parser Class Section
	// ===========================

	char Parser::formattingDelimiter = '-';

	vector<std::string> Parser::split(std::string& line) {
		vector<std::string> tmpVec;
		stringstream ss(line);
		std::string token;
		while (getline(ss, token, Parser::formattingDelimiter))
		{
			tmpVec.push_back(token);
		}
		return tmpVec;

	}

	Client Parser::parseToClient(std::string& line) {
		vector<std::string> initStrVec = split(line);
		Client newClient(initStrVec[1], initStrVec[2]);
		newClient.setBalance(stod(initStrVec[3]));
		newClient.setID(stoi(initStrVec[0]));
		Client::decreaseStaticID();
		return newClient;
	}

	Employee Parser::parseToEmployee(std::string& line) {
		vector<std::string> initStrVec = split(line);
		Employee newEmp(initStrVec[1], initStrVec[2], initStrVec[3],0.0);
		newEmp.setSalary(stod(initStrVec[4]));
		newEmp.setID(stoi(initStrVec[0]));
		Employee::decreaseStaticID();
		return newEmp;
	}

	Admin Parser::parseToAdmin(std::string& line) {
		vector<std::string> initStrVec = split(line);
		Admin newAdmin(initStrVec[1], initStrVec[2]);
		newAdmin.setID(stoi(initStrVec[0]));
		Admin::decreaseStaticID();
		return newAdmin;
	}


	// =========================== Parser Class

}