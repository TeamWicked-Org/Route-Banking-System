#include "../../include/models/utils.h"

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
}