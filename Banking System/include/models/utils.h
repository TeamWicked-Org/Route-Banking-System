#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>

// includes dependency library for #include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

// Utils Namespace
namespace utils {

	// Validation Class
	// ====================================
	class Validation final {
	private:
		struct validationConstraints {
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

		static std::string stringHash(std::string str);

	};
}  // namespace utils
