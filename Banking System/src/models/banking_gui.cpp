// =============================================================================
// banking_gui.cpp  --  ImGui banking system GUI
// Features: Segoe UI font, UTF-8 Unicode, login overlay, role-based views
//
// Roles:
//   Admin    -> Clients, Employees, Admin views
//   Employee -> Clients view only
// =============================================================================
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>

#include "../../include/models/banking_gui.h"
#include "../../include/models/Client.h"
#include "../../include/models/Employee.h"
#include "../../include/models/Admin.h"
#include "../../include/models/utils.h"

// =============================================================================
// Unicode glyph ranges loaded into the font atlas.
// Each entry is a [first, last] codepoint pair (inclusive); terminated by 0.
//
// NOTE: ImGui does not do bidirectional layout or Arabic shaping.
// Arabic characters will appear as individual unconnected glyphs.
// For correct RTL / ligature rendering a library like HarfBuzz is needed.
// =============================================================================
static const ImWchar k_font_ranges[] = {
    0x0020, 0x024F,  // Basic Latin + Latin-1 Supplement + Latin Extended A/B
    0x0370, 0x03FF,  // Greek and Coptic
    0x0400, 0x04FF,  // Cyrillic
    0x0600, 0x06FF,  // Arabic (basic block)
    0xFB50, 0xFDFF,  // Arabic Presentation Forms-A (shaped/connected forms)
    0xFE70, 0xFEFF,  // Arabic Presentation Forms-B
    0x2000, 0x206F,  // General Punctuation
    0x20A0, 0x20CF,  // Currency Symbols  (€ £ ¥ ₹ …)
    0,
};

// =============================================================================
// Banking system state
// =============================================================================
namespace bank_state {

    // ── Views and access roles ────────────────────────────────
    enum class View { Clients, Employees, Admin };
    enum class Role { None, Employee, Admin };

    static View        current_view = View::Clients;
    static Role        current_role = Role::None;
    static std::string current_username;

    static bool is_logged_in() { return current_role != Role::None; }

    // ── Data ──────────────────────────────────────────────────
    static std::vector<Client>   clients;
    static std::vector<Employee> employees;
    static std::vector<Admin>    admins;

    // Row selection
    static int selected_client = -1;
    static int selected_employee = -1;

    // Modal open flags (set true → OpenPopup on the same frame)
    static bool open_add_client = false;
    static bool open_deposit = false;
    static bool open_withdraw = false;
    static bool open_transfer = false;
    static bool open_add_employee = false;
    static bool open_set_salary = false;
    static bool open_add_admin = false;

    // Sidebar toast
    static char  status_msg[256] = {};
    static bool  status_is_error = false;
    static float status_timer = 0.f;

    static void set_status(const char* msg, bool error = false) {
        strncpy_s(status_msg, sizeof status_msg, msg, _TRUNCATE);
        status_is_error = error;
        status_timer = 3.5f;
    }

    static void do_logout() {
        current_role = Role::None;
        current_username.clear();
        current_view = View::Clients;
        selected_client = -1;
        selected_employee = -1;
    }
}

// =============================================================================
// Login helper — hashes the input password and compares to stored hash
// =============================================================================
static bool try_login(const char* name_in, const char* pass_in)
{
    const std::string pw_hash = utils::Security::stringHash(std::string(pass_in));

    for (auto& a : bank_state::admins) {
        if (a.getName() == name_in && a.getPassword() == pw_hash) {
            bank_state::current_role = bank_state::Role::Admin;
            bank_state::current_username = name_in;
            bank_state::current_view = bank_state::View::Clients;
            return true;
        }
    }
    for (auto& e : bank_state::employees) {
        if (e.getName() == name_in && e.getPassword() == pw_hash) {
            bank_state::current_role = bank_state::Role::Employee;
            bank_state::current_username = name_in;
            bank_state::current_view = bank_state::View::Clients;
            return true;
        }
    }
    return false;
}

// =============================================================================
// Helpers
// =============================================================================
static std::string fmt_money(double v) {
    std::ostringstream ss;
    ss << "$" << std::fixed << std::setprecision(2) << v;
    return ss.str();
}

static bool btn_green(const char* lbl, ImVec2 sz = { 0,0 }) {
    ImGui::PushStyleColor(ImGuiCol_Button, { 0.09f,0.47f,0.19f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.13f,0.61f,0.26f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.07f,0.35f,0.14f,1.f });
    bool r = ImGui::Button(lbl, sz);
    ImGui::PopStyleColor(3);
    return r;
}
static bool btn_red(const char* lbl, ImVec2 sz = { 0,0 }) {
    ImGui::PushStyleColor(ImGuiCol_Button, { 0.60f,0.12f,0.12f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.78f,0.18f,0.18f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.48f,0.09f,0.09f,1.f });
    bool r = ImGui::Button(lbl, sz);
    ImGui::PopStyleColor(3);
    return r;
}
static bool btn_amber(const char* lbl, ImVec2 sz = { 0,0 }) {
    ImGui::PushStyleColor(ImGuiCol_Button, { 0.56f,0.36f,0.04f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.72f,0.48f,0.06f,1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.44f,0.28f,0.03f,1.f });
    bool r = ImGui::Button(lbl, sz);
    ImGui::PopStyleColor(3);
    return r;
}

static void stats_bar(const char* la, const char* va, ImVec4 ca,
    const char* lb, const char* vb, ImVec4 cb, float w)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.10f,0.13f,0.20f,1.f });
    ImGui::BeginChild("##stats_bar", { w, 54.f }, false);
    ImGui::SetCursorPos({ 12.f,10.f });
    ImGui::TextDisabled("%s", la); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ca);
    ImGui::Text("%s", va);
    ImGui::PopStyleColor();
    ImGui::SameLine(w * 0.42f);
    ImGui::TextDisabled("%s", lb); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, cb);
    ImGui::Text("%s", vb);
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// =============================================================================
// Login Screen
// =============================================================================
static void draw_login_screen(const ImVec2& display)
{
    constexpr float CW = 420.f;
    constexpr float CH = 372.f;

    ImGui::SetCursorPos({ (display.x - CW) * 0.5f,
                         (display.y - CH) * 0.5f });

    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.10f,0.13f,0.20f,1.f });
    ImGui::BeginChild("##login_card", { CW, CH }, true);

    // ── Logo / title ──────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Spacing();
    {
        const char* logo = "BankSys";
        ImGui::SetWindowFontScale(1.55f);
        float tw = ImGui::CalcTextSize(logo).x;
        ImGui::SetCursorPosX((CW - tw) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.40f,0.68f,1.f,1.f });
        ImGui::Text("%s", logo);
        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleColor();
    }
    {
        const char* sub = "Staff Sign-In Portal";
        float sw = ImGui::CalcTextSize(sub).x;
        ImGui::SetCursorPosX((CW - sw) * 0.5f);
        ImGui::TextDisabled("%s", sub);
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // ── Input fields ──────────────────────────────────────────
    static char uname[64] = {};
    static char upass[64] = {};
    static char err[160] = {};

    constexpr float FX = 20.f;                   // left margin
    const     float FW = CW - FX * 2.f;          // field width

    ImGui::SetCursorPosX(FX);
    ImGui::TextDisabled("Username");
    ImGui::SetCursorPosX(FX);
    ImGui::SetNextItemWidth(FW);
    bool user_enter = ImGui::InputText("##ln_u", uname, sizeof uname,
        ImGuiInputTextFlags_EnterReturnsTrue);
    // Enter in username box → jump focus to password
    if (user_enter) ImGui::SetKeyboardFocusHere();

    ImGui::Spacing();
    ImGui::SetCursorPosX(FX);
    ImGui::TextDisabled("Password");
    ImGui::SetCursorPosX(FX);
    ImGui::SetNextItemWidth(FW);
    bool pass_enter = ImGui::InputText("##ln_p", upass, sizeof upass,
        ImGuiInputTextFlags_Password |
        ImGuiInputTextFlags_EnterReturnsTrue);

    // ── Error message ─────────────────────────────────────────
    ImGui::Spacing();
    if (err[0]) {
        ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.40f,0.40f,1.f });
        float ew = ImGui::CalcTextSize(err).x;
        ImGui::SetCursorPosX(std::clamp((CW - ew) * 0.5f, FX, CW - FX));
        ImGui::Text("%s", err);
        ImGui::PopStyleColor();
    }
    else {
        ImGui::Dummy({ 0.f, ImGui::GetTextLineHeight() });
    }
    ImGui::Spacing();

    // ── Sign-In button (centred) ──────────────────────────────
    constexpr float BW = 150.f;
    ImGui::SetCursorPosX((CW - BW) * 0.5f);
    bool clicked = ImGui::Button("  Sign In  ", { BW, 34.f });

    // ── Attempt login ─────────────────────────────────────────
    if (clicked || pass_enter) {
        if (uname[0] == '\0')
            strncpy_s(err, sizeof err, "Please enter a username.", _TRUNCATE);
        else if (upass[0] == '\0')
            strncpy_s(err, sizeof err, "Please enter a password.", _TRUNCATE);
        else if (!try_login(uname, upass)) {
            strncpy_s(err, sizeof err, "Invalid credentials — please try again.", _TRUNCATE);
            memset(upass, 0, sizeof upass);
        }
        else {
            // Success: clear buffers; try_login already set role+view
            err[0] = '\0';
            memset(uname, 0, sizeof uname);
            memset(upass, 0, sizeof upass);
        }
    }

    // ── Demo credentials hint ─────────────────────────────────
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(FX);
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.42f,0.50f,0.64f,1.f });
    ImGui::TextWrapped("Demo  |  Admin: SuperAdmin / adminpass1");
    ImGui::SetCursorPosX(FX);
    ImGui::TextWrapped("       |  Staff: Emma Davis / emppass001");
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// =============================================================================
// Modals
// =============================================================================

// ── Add Client ────────────────────────────────────────────────────────────────
static void modal_add_client()
{
    if (bank_state::open_add_client) { ImGui::OpenPopup("Add Client"); bank_state::open_add_client = false; }
    if (!ImGui::BeginPopupModal("Add Client", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;

    static char name_buf[64] = {}; static char pass_buf[64] = {};
    static double init_bal = 1500.0; static char err[256] = {};

    ImGui::TextDisabled("Create a new bank client account");
    ImGui::Separator(); ImGui::Spacing();

    auto vc = utils::Validation::get_ValidationStruct();
    ImGui::Text("Full Name:");    ImGui::SameLine(130.f); ImGui::SetNextItemWidth(210.f);
    ImGui::InputText("##c_name", name_buf, sizeof name_buf);
    ImGui::Text("Password:");     ImGui::SameLine(130.f); ImGui::SetNextItemWidth(210.f);
    ImGui::InputText("##c_pass", pass_buf, sizeof pass_buf, ImGuiInputTextFlags_Password);
    ImGui::Text("Init. Balance:"); ImGui::SameLine(130.f); ImGui::SetNextItemWidth(210.f);
    ImGui::InputDouble("##c_bal", &init_bal, 500.0, 1000.0, "$ %.2f");

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.50f,0.56f,0.66f,1.f });
    ImGui::TextWrapped("Name: %d-%d chars  |  Password: %d-%d chars, no spaces  |  Min balance: $%.0f",
        vc.minNameSize, vc.maxNameSize, vc.minPasswordSize, vc.maxPasswordSize, vc.minBalance);
    ImGui::PopStyleColor();

    if (err[0]) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
        ImGui::TextWrapped("  %s", err); ImGui::PopStyleColor();
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (btn_green("Create", { 100.f,0.f })) {
        std::string n(name_buf), p(pass_buf);
        if (!utils::Validation::validateName(n))    strncpy_s(err, sizeof err, "Invalid name — check length and allowed characters.", _TRUNCATE);
        else if (!utils::Validation::validatePassword(p)) strncpy_s(err, sizeof err, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
        else if (!utils::Validation::validateBalance(init_bal)) strncpy_s(err, sizeof err, "Initial balance is below the minimum.", _TRUNCATE);
        else {
            bank_state::clients.emplace_back(n, p);
            bank_state::clients.back().setBalance(init_bal);
            memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf);
            init_bal = 1500.0; err[0] = '\0';
            bank_state::set_status("Client created.");
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (btn_red("Cancel", { 100.f,0.f })) {
        memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf);
        init_bal = 1500.0; err[0] = '\0'; ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// ── Deposit ───────────────────────────────────────────────────────────────────
static void modal_deposit()
{
    if (bank_state::open_deposit) { ImGui::OpenPopup("Deposit"); bank_state::open_deposit = false; }
    if (!ImGui::BeginPopupModal("Deposit", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;

    static double amount = 500.0;
    int idx = bank_state::selected_client;
    if (idx >= 0 && idx < (int)bank_state::clients.size()) {
        auto& c = bank_state::clients[idx];
        ImGui::Text("Client:  %s", c.getName().c_str());
        ImGui::Text("Balance: %s", fmt_money(c.getBalance()).c_str());
        ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Amount:"); ImGui::SameLine(90.f); ImGui::SetNextItemWidth(170.f);
        ImGui::InputDouble("##dep", &amount, 100.0, 1000.0, "$ %.2f");
        if (amount < 0.01) amount = 0.01;
        ImGui::Spacing();
        if (btn_green("Deposit", { 100.f,0.f })) {
            c.deposit(amount); bank_state::set_status("Deposit successful.");
            amount = 500.0; ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
    }
    if (btn_red("Cancel", { 100.f,0.f })) { amount = 500.0; ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// ── Withdraw ──────────────────────────────────────────────────────────────────
static void modal_withdraw()
{
    if (bank_state::open_withdraw) { ImGui::OpenPopup("Withdraw"); bank_state::open_withdraw = false; }
    if (!ImGui::BeginPopupModal("Withdraw", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;

    static double amount = 200.0; static char err[128] = {};
    int idx = bank_state::selected_client;
    if (idx >= 0 && idx < (int)bank_state::clients.size()) {
        auto& c = bank_state::clients[idx];
        ImGui::Text("Client:  %s", c.getName().c_str());
        ImGui::Text("Balance: %s", fmt_money(c.getBalance()).c_str());
        ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Amount:"); ImGui::SameLine(90.f); ImGui::SetNextItemWidth(170.f);
        ImGui::InputDouble("##wd", &amount, 100.0, 1000.0, "$ %.2f");
        if (amount < 0.01) amount = 0.01;
        if (err[0]) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
            ImGui::Text("  %s", err); ImGui::PopStyleColor();
        }
        ImGui::Spacing();
        if (btn_amber("Withdraw", { 100.f,0.f })) {
            if (amount > c.getBalance()) strncpy_s(err, sizeof err, "Insufficient balance.", _TRUNCATE);
            else {
                c.withdraw(amount); bank_state::set_status("Withdrawal successful.");
                amount = 200.0; err[0] = '\0'; ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
    }
    if (btn_red("Cancel", { 100.f,0.f })) { amount = 200.0; err[0] = '\0'; ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// ── Transfer ──────────────────────────────────────────────────────────────────
static void modal_transfer()
{
    if (bank_state::open_transfer) { ImGui::OpenPopup("Transfer"); bank_state::open_transfer = false; }
    if (!ImGui::BeginPopupModal("Transfer", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;

    static double amount = 200.0; static int target_idx = -1; static char err[128] = {};
    int src = bank_state::selected_client;
    if (src >= 0 && src < (int)bank_state::clients.size()) {
        auto& from = bank_state::clients[src];
        ImGui::Text("From:    %s", from.getName().c_str());
        ImGui::Text("Balance: %s", fmt_money(from.getBalance()).c_str());
        ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Amount:"); ImGui::SameLine(90.f); ImGui::SetNextItemWidth(210.f);
        ImGui::InputDouble("##tr_amt", &amount, 100.0, 1000.0, "$ %.2f");
        if (amount < 0.01) amount = 0.01;
        ImGui::Text("To:");    ImGui::SameLine(90.f); ImGui::SetNextItemWidth(210.f);
        const char* prev = (target_idx >= 0 && target_idx < (int)bank_state::clients.size())
            ? bank_state::clients[target_idx].getName().c_str() : "Select recipient...";
        if (ImGui::BeginCombo("##tr_to", prev)) {
            for (int i = 0; i < (int)bank_state::clients.size(); ++i) {
                if (i == src) continue;
                if (ImGui::Selectable(bank_state::clients[i].getName().c_str(), target_idx == i))
                    target_idx = i;
            }
            ImGui::EndCombo();
        }
        if (err[0]) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
            ImGui::Text("  %s", err); ImGui::PopStyleColor();
        }
        ImGui::Spacing();
        if (ImGui::Button("Transfer", { 100.f,0.f })) {
            if (target_idx < 0 || target_idx >= (int)bank_state::clients.size())
                strncpy_s(err, sizeof err, "Select a recipient first.", _TRUNCATE);
            else if (amount > from.getBalance())
                strncpy_s(err, sizeof err, "Insufficient balance.", _TRUNCATE);
            else {
                from.transferTo(amount, bank_state::clients[target_idx]);
                bank_state::set_status("Transfer completed.");
                amount = 200.0; target_idx = -1; err[0] = '\0'; ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
    }
    if (btn_red("Cancel", { 100.f,0.f })) { amount = 200.0; target_idx = -1; err[0] = '\0'; ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// ── Add Employee ──────────────────────────────────────────────────────────────
static void modal_add_employee()
{
    if (bank_state::open_add_employee) { ImGui::OpenPopup("Add Employee"); bank_state::open_add_employee = false; }
    if (!ImGui::BeginPopupModal("Add Employee", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;

    static char name_buf[64] = {}; static char pass_buf[64] = {}; static char role_buf[48] = {};
    static double salary = 5000.0; static char err[256] = {};

    ImGui::TextDisabled("Register a new bank employee");
    ImGui::Separator(); ImGui::Spacing();
    ImGui::Text("Full Name:"); ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##e_name", name_buf, sizeof name_buf);
    ImGui::Text("Password:"); ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##e_pass", pass_buf, sizeof pass_buf, ImGuiInputTextFlags_Password);
    ImGui::Text("Role:");     ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##e_role", role_buf, sizeof role_buf);
    ImGui::Text("Salary:");   ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputDouble("##e_sal", &salary, 500.0, 1000.0, "$ %.2f");

    if (err[0]) { ImGui::Spacing(); ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f }); ImGui::TextWrapped("  %s", err); ImGui::PopStyleColor(); }
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (btn_green("Add", { 100.f,0.f })) {
        std::string n(name_buf), p(pass_buf), r(role_buf);
        if (!utils::Validation::validateName(n))    strncpy_s(err, sizeof err, "Invalid name.", _TRUNCATE);
        else if (!utils::Validation::validatePassword(p)) strncpy_s(err, sizeof err, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
        else if (r.empty())                              strncpy_s(err, sizeof err, "Role cannot be empty.", _TRUNCATE);
        else if (!utils::Validation::validateSalary(salary)) strncpy_s(err, sizeof err, "Salary below minimum ($5000).", _TRUNCATE);
        else {
            bank_state::employees.emplace_back(n, p, r, salary);
            memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf); memset(role_buf, 0, sizeof role_buf);
            salary = 5000.0; err[0] = '\0'; bank_state::set_status("Employee added."); ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (btn_red("Cancel", { 100.f,0.f })) {
        memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf); memset(role_buf, 0, sizeof role_buf);
        salary = 5000.0; err[0] = '\0'; ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// ── Set Salary ────────────────────────────────────────────────────────────────
static void modal_set_salary()
{
    if (bank_state::open_set_salary) { ImGui::OpenPopup("Update Salary"); bank_state::open_set_salary = false; }
    if (!ImGui::BeginPopupModal("Update Salary", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;

    static double new_sal = 5000.0; static char err[128] = {};
    int idx = bank_state::selected_employee;
    if (idx >= 0 && idx < (int)bank_state::employees.size()) {
        auto& e = bank_state::employees[idx];
        ImGui::Text("Employee: %s", e.getName().c_str());
        ImGui::Text("Current:  %s", fmt_money(e.getSalary()).c_str());
        ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("New Salary:"); ImGui::SameLine(110.f); ImGui::SetNextItemWidth(170.f);
        ImGui::InputDouble("##ns", &new_sal, 500.0, 1000.0, "$ %.2f");
        if (err[0]) { ImGui::Spacing(); ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f }); ImGui::Text("  %s", err); ImGui::PopStyleColor(); }
        ImGui::Spacing();
        if (btn_amber("Update", { 100.f,0.f })) {
            if (!utils::Validation::validateSalary(new_sal)) strncpy_s(err, sizeof err, "Salary below minimum ($5000).", _TRUNCATE);
            else { e.setSalary(new_sal); bank_state::set_status("Salary updated."); new_sal = 5000.0; err[0] = '\0'; ImGui::CloseCurrentPopup(); }
        }
        ImGui::SameLine();
    }
    if (btn_red("Cancel", { 100.f,0.f })) { new_sal = 5000.0; err[0] = '\0'; ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}

// ── Add Admin ─────────────────────────────────────────────────────────────────
static void modal_add_admin()
{
    if (bank_state::open_add_admin) { ImGui::OpenPopup("Add Administrator"); bank_state::open_add_admin = false; }
    if (!ImGui::BeginPopupModal("Add Administrator", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) return;

    static char name_buf[64] = {}; static char pass_buf[64] = {}; static char err[256] = {};
    ImGui::TextDisabled("Grant administrator access");
    ImGui::Separator(); ImGui::Spacing();
    ImGui::Text("Full Name:"); ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##a_name", name_buf, sizeof name_buf);
    ImGui::Text("Password:"); ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##a_pass", pass_buf, sizeof pass_buf, ImGuiInputTextFlags_Password);
    if (err[0]) { ImGui::Spacing(); ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f }); ImGui::TextWrapped("  %s", err); ImGui::PopStyleColor(); }
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    if (btn_green("Create", { 100.f,0.f })) {
        std::string n(name_buf), p(pass_buf);
        if (!utils::Validation::validateName(n))    strncpy_s(err, sizeof err, "Invalid name.", _TRUNCATE);
        else if (!utils::Validation::validatePassword(p)) strncpy_s(err, sizeof err, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
        else {
            bank_state::admins.emplace_back(n, p);
            memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf);
            err[0] = '\0'; bank_state::set_status("Administrator added."); ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (btn_red("Cancel", { 100.f,0.f })) {
        memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf);
        err[0] = '\0'; ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// =============================================================================
// Views
// =============================================================================

// ── Client Management ─────────────────────────────────────────────────────────
static void view_clients(float w, float h)
{
    using namespace bank_state;
    double total_bal = 0.0;
    for (const auto& c : clients) total_bal += c.getBalance();
    stats_bar("Total Clients:", std::to_string(clients.size()).c_str(), { 0.40f,0.70f,1.f,1.f },
        "Total on Deposit:", fmt_money(total_bal).c_str(), { 0.30f,0.85f,0.50f,1.f }, w);
    ImGui::Spacing();

    int delete_idx = -1;
    float table_h = h - 168.f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.06f,0.07f,0.11f,1.f });
    ImGui::BeginChild("##ct", { w,table_h }, true);
    if (ImGui::BeginTable("tbl_clients", 4,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupColumn("Balance", ImGuiTableColumnFlags_WidthStretch, 0.65f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 296.f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)clients.size(); ++i) {
            ImGui::TableNextRow(ImGuiTableRowFlags_None, 26.f);
            bool sel = (selected_client == i);
            ImGui::TableSetColumnIndex(0);
            char lbl[24]; snprintf(lbl, sizeof lbl, "%d##cs%d", clients[i].getID(), i);
            if (ImGui::Selectable(lbl, sel,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, { 0,24.f }))
                selected_client = i;
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", clients[i].getName().c_str());
            ImGui::TableSetColumnIndex(2);
            double bal = clients[i].getBalance();
            ImGui::PushStyleColor(ImGuiCol_Text, bal < 1500.0
                ? ImVec4{ 1.f,0.40f,0.40f,1.f } : ImVec4{ 0.30f,0.85f,0.50f,1.f });
            ImGui::Text("%s", fmt_money(bal).c_str());
            ImGui::PopStyleColor();
            ImGui::TableSetColumnIndex(3);
            ImGui::PushID(i);
            if (btn_green("Deposit", { 72.f,0.f })) { selected_client = i; open_deposit = true; }
            ImGui::SameLine();
            if (btn_amber("Withdraw", { 76.f,0.f })) { selected_client = i; open_withdraw = true; }
            ImGui::SameLine();
            if (ImGui::Button("Transfer", { 76.f,0.f })) { selected_client = i; open_transfer = true; }
            ImGui::SameLine();
            if (btn_red("X", { 26.f,0.f })) delete_idx = i;
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (delete_idx >= 0) {
        clients.erase(clients.begin() + delete_idx);
        if (selected_client >= (int)clients.size()) selected_client = (int)clients.size() - 1;
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    if (btn_green("+ Add Client", { 130.f,32.f })) open_add_client = true;

    modal_add_client(); modal_deposit(); modal_withdraw(); modal_transfer();
}

// ── Employee Management ───────────────────────────────────────────────────────
static void view_employees(float w, float h)
{
    using namespace bank_state;
    double total_payroll = 0.0;
    for (const auto& e : employees) total_payroll += e.getSalary();
    stats_bar("Total Employees:", std::to_string(employees.size()).c_str(), { 0.40f,0.70f,1.f,1.f },
        "Monthly Payroll:", fmt_money(total_payroll).c_str(), { 1.f,0.76f,0.30f,1.f }, w);
    ImGui::Spacing();

    float table_h = h - 168.f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.06f,0.07f,0.11f,1.f });
    ImGui::BeginChild("##et", { w,table_h }, true);
    if (ImGui::BeginTable("tbl_employees", 4,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Salary", ImGuiTableColumnFlags_WidthFixed, 120.f);
        ImGui::TableHeadersRow();
        for (int i = 0; i < (int)employees.size(); ++i) {
            ImGui::TableNextRow(ImGuiTableRowFlags_None, 26.f);
            ImGui::TableSetColumnIndex(0);
            char lbl[24]; snprintf(lbl, sizeof lbl, "%d##es%d", employees[i].getID(), i);
            if (ImGui::Selectable(lbl, selected_employee == i,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, { 0,24.f }))
                selected_employee = i;
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", employees[i].getName().c_str());
            ImGui::TableSetColumnIndex(2); ImGui::TextDisabled("%s", employees[i].getRole().c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.30f,0.85f,0.50f,1.f });
            ImGui::Text("%s", fmt_money(employees[i].getSalary()).c_str());
            ImGui::PopStyleColor();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    if (btn_green("+ Add Employee", { 145.f,32.f })) open_add_employee = true;
    if (selected_employee >= 0 && selected_employee < (int)employees.size()) {
        ImGui::SameLine();
        if (btn_amber("Edit Salary", { 115.f,32.f })) open_set_salary = true;
        ImGui::SameLine();
        if (btn_red("Remove", { 90.f,32.f })) {
            employees.erase(employees.begin() + selected_employee);
            if (selected_employee >= (int)employees.size()) selected_employee = (int)employees.size() - 1;
            set_status("Employee removed.");
        }
    }
    modal_add_employee(); modal_set_salary();
}

// ── Administration Dashboard ──────────────────────────────────────────────────
static void view_admin(float w, float /*h*/)
{
    using namespace bank_state;
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.65f,0.78f,1.f,1.f });
    ImGui::SetWindowFontScale(1.08f); ImGui::Text("System Overview");
    ImGui::SetWindowFontScale(1.f); ImGui::PopStyleColor();
    ImGui::Separator(); ImGui::Spacing();

    double total_bal = 0.0, total_payroll = 0.0;
    for (const auto& c : clients)   total_bal += c.getBalance();
    for (const auto& e : employees) total_payroll += e.getSalary();

    float cw = (w - 24.f) / 3.f;
    struct Card { const char* title; std::string val; ImVec4 col; };
    Card cards[] = {
        {"Total Clients",    std::to_string(clients.size()),  {0.40f,0.70f,1.f,1.f}  },
        {"Total on Deposit", fmt_money(total_bal),            {0.30f,0.85f,0.50f,1.f} },
        {"Monthly Payroll",  fmt_money(total_payroll),        {1.f,0.76f,0.30f,1.f}  },
    };
    for (int i = 0; i < 3; ++i) {
        if (i > 0) ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.10f,0.13f,0.20f,1.f });
        ImGui::BeginChild(cards[i].title, { cw,80.f }, true);
        ImGui::SetCursorPos({ 10.f,8.f }); ImGui::TextDisabled("%s", cards[i].title);
        ImGui::SetCursorPos({ 10.f,34.f });
        ImGui::SetWindowFontScale(1.28f);
        ImGui::PushStyleColor(ImGuiCol_Text, cards[i].col);
        ImGui::Text("%s", cards[i].val.c_str()); ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.f);
        ImGui::EndChild(); ImGui::PopStyleColor();
    }

    ImGui::Spacing(); ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.65f,0.78f,1.f,1.f });
    ImGui::SetWindowFontScale(1.05f); ImGui::Text("Administrators");
    ImGui::SetWindowFontScale(1.f); ImGui::PopStyleColor();
    ImGui::Separator(); ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.06f,0.07f,0.11f,1.f });
    ImGui::BeginChild("##admin_tbl", { w,170.f }, true);
    if (ImGui::BeginTable("tbl_admins", 2,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60.f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        for (auto& a : admins) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", a.getID());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", a.getName().c_str());
        }
        ImGui::EndTable();
    }
    ImGui::EndChild(); ImGui::PopStyleColor();

    ImGui::Spacing();
    if (btn_green("+ Add Administrator", { 170.f,32.f })) open_add_admin = true;

    ImGui::Spacing(); ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.65f,0.78f,1.f,1.f });
    ImGui::SetWindowFontScale(1.05f); ImGui::Text("Employee Roster");
    ImGui::SetWindowFontScale(1.f); ImGui::PopStyleColor();
    ImGui::Separator(); ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.06f,0.07f,0.11f,1.f });
    ImGui::BeginChild("##emp_ro", { w,160.f }, true);
    if (ImGui::BeginTable("tbl_emp_ro", 3,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableHeadersRow();
        for (auto& e : employees) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", e.getID());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", e.getName().c_str());
            ImGui::TableSetColumnIndex(2); ImGui::TextDisabled("%s", e.getRole().c_str());
        }
        ImGui::EndTable();
    }
    ImGui::EndChild(); ImGui::PopStyleColor();

    modal_add_admin();
}

// =============================================================================
// init / shutdown
// =============================================================================
static bool g_initialized = false;

bool banking_gui::init(HWND hwnd, ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // ── Style ─────────────────────────────────────────────────
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6.f;  s.FrameRounding = 4.f;  s.PopupRounding = 4.f;
    s.ScrollbarRounding = 4.f; s.GrabRounding = 4.f;  s.TabRounding = 4.f;
    s.WindowBorderSize = 0.f; s.FrameBorderSize = 0.f;
    s.WindowPadding = { 12.f,12.f }; s.FramePadding = { 8.f,5.f }; s.ItemSpacing = { 8.f,6.f };
    s.ScrollbarSize = 12.f;

    auto& c = s.Colors;
    c[ImGuiCol_WindowBg] = { 0.08f,0.10f,0.14f,1.f };
    c[ImGuiCol_ChildBg] = { 0.06f,0.07f,0.10f,1.f };
    c[ImGuiCol_PopupBg] = { 0.09f,0.11f,0.16f,0.98f };
    c[ImGuiCol_FrameBg] = { 0.11f,0.13f,0.18f,1.f };
    c[ImGuiCol_FrameBgHovered] = { 0.14f,0.17f,0.25f,1.f };
    c[ImGuiCol_FrameBgActive] = { 0.17f,0.21f,0.30f,1.f };
    c[ImGuiCol_TitleBg] = { 0.05f,0.07f,0.10f,1.f };
    c[ImGuiCol_TitleBgActive] = { 0.08f,0.10f,0.16f,1.f };
    c[ImGuiCol_Button] = { 0.13f,0.34f,0.70f,1.f };
    c[ImGuiCol_ButtonHovered] = { 0.19f,0.44f,0.86f,1.f };
    c[ImGuiCol_ButtonActive] = { 0.10f,0.26f,0.58f,1.f };
    c[ImGuiCol_Header] = { 0.13f,0.34f,0.70f,0.5f };
    c[ImGuiCol_HeaderHovered] = { 0.13f,0.34f,0.70f,0.8f };
    c[ImGuiCol_HeaderActive] = { 0.13f,0.34f,0.70f,1.f };
    c[ImGuiCol_Border] = { 0.20f,0.24f,0.36f,1.f };
    c[ImGuiCol_Separator] = { 0.20f,0.24f,0.36f,0.8f };
    c[ImGuiCol_ScrollbarBg] = { 0.05f,0.06f,0.08f,1.f };
    c[ImGuiCol_ScrollbarGrab] = { 0.13f,0.34f,0.70f,0.7f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.19f,0.44f,0.86f,1.f };
    c[ImGuiCol_TableHeaderBg] = { 0.10f,0.13f,0.20f,1.f };
    c[ImGuiCol_TableBorderLight] = { 0.15f,0.18f,0.28f,1.f };
    c[ImGuiCol_TableBorderStrong] = { 0.20f,0.24f,0.36f,1.f };
    c[ImGuiCol_TableRowBg] = { 0.f,0.f,0.f,0.f };
    c[ImGuiCol_TableRowBgAlt] = { 1.f,1.f,1.f,0.030f };
    c[ImGuiCol_CheckMark] = { 0.40f,0.70f,1.f,1.f };

    // ── Font loading ──────────────────────────────────────────
    // Load Segoe UI (present on every modern Windows installation).
    // k_font_ranges covers Latin, Greek, Cyrillic, Arabic, and common
    // currency / punctuation symbols for broad UTF-8 text display.
    // If the file is not found the built-in ImGui bitmap font is used.
    io.Fonts->Clear();
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = false;

    ImFont* seg_font = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\segoeui.ttf",
        15.0f, &cfg, k_font_ranges);

    if (!seg_font) {
        // Fallback: built-in pixel font (ASCII only, no UTF-8 extension)
        io.Fonts->AddFontDefault();
    }
    // The D3D11 backend rebuilds the font atlas on the first NewFrame() call.

    // ── Backend init ──────────────────────────────────────────
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dev, ctx);

    // ── Seed demo data ────────────────────────────────────────
    bank_state::clients.emplace_back("Alice Smith", "password001"); bank_state::clients.back().setBalance(8500.0);
    bank_state::clients.emplace_back("Bob Johnson", "password002"); bank_state::clients.back().setBalance(22000.0);
    bank_state::clients.emplace_back("Carol White", "password003"); bank_state::clients.back().setBalance(3200.0);
    bank_state::clients.emplace_back("Daniel Brown", "password004"); bank_state::clients.back().setBalance(15750.0);

    bank_state::employees.emplace_back("Emma Davis", "emppass001", "Teller", 6500.0);
    bank_state::employees.emplace_back("Frank Miller", "emppass002", "Senior Teller", 8000.0);
    bank_state::employees.emplace_back("Grace Wilson", "emppass003", "Branch Manager", 12000.0);

    bank_state::admins.emplace_back("SuperAdmin", "adminpass1");

    g_initialized = true;
    return true;
}

void banking_gui::shutdown(HWND /*hwnd*/)
{
    if (!g_initialized) return;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_initialized = false;
}

// =============================================================================
// tick()  --  called every frame
// =============================================================================
void banking_gui::tick()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2   display = io.DisplaySize;

    if (bank_state::status_timer > 0.f)
        bank_state::status_timer -= io.DeltaTime;

    // Full-screen host window (transparent to mouse when showing login)
    ImGui::SetNextWindowPos({ 0.f,0.f });
    ImGui::SetNextWindowSize(display);
    ImGui::Begin("##host", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize);

    // ─────────────────────────────────────────────────────────
    // A) LOGIN SCREEN (not yet authenticated)
    // ─────────────────────────────────────────────────────────
    if (!bank_state::is_logged_in()) {
        draw_login_screen(display);
        ImGui::End();
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        return;
    }

    // ─────────────────────────────────────────────────────────
    // B) MAIN BANKING UI (authenticated)
    // ─────────────────────────────────────────────────────────

    // Guard: if an Employee somehow ends up on a restricted view, reset it.
    if (bank_state::current_role == bank_state::Role::Employee &&
        bank_state::current_view != bank_state::View::Clients)
        bank_state::current_view = bank_state::View::Clients;

    const float sw = 185.f;   // sidebar width

    // ── Sidebar ───────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.05f,0.06f,0.09f,1.f });
    ImGui::BeginChild("##sidebar", { sw, display.y - 20.f }, false);

    // Logo
    ImGui::SetCursorPos({ 14.f,14.f });
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.40f,0.68f,1.f,1.f });
    ImGui::SetWindowFontScale(1.22f);
    ImGui::Text("BankSys");
    ImGui::SetWindowFontScale(1.f);
    ImGui::PopStyleColor();
    ImGui::SetCursorPosX(14.f);
    ImGui::TextDisabled("Banking System");

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    // Logged-in user info
    ImGui::SetCursorPosX(14.f);
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.78f,0.88f,1.f,1.f });
    ImGui::Text("%.20s", bank_state::current_username.c_str());
    ImGui::PopStyleColor();
    ImGui::SetCursorPosX(14.f);
    ImGui::TextDisabled("%s",
        bank_state::current_role == bank_state::Role::Admin
        ? "Administrator" : "Employee");

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    // Nav buttons — filtered by role
    struct NavItem { const char* label; bank_state::View view; };
    constexpr NavItem nav[] = {
        { " Clients",   bank_state::View::Clients   },
        { " Employees", bank_state::View::Employees },
        { " Admin",     bank_state::View::Admin     },
    };
    for (const auto& n : nav) {
        // Employees can only see the Clients view
        if (bank_state::current_role == bank_state::Role::Employee &&
            n.view != bank_state::View::Clients) continue;

        bool active = (bank_state::current_view == n.view);
        ImGui::PushStyleColor(ImGuiCol_Button,
            active ? ImVec4{ 0.13f,0.34f,0.70f,1.f }
        : ImVec4{ 0.09f,0.11f,0.17f,1.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            active ? ImVec4{ 0.19f,0.44f,0.86f,1.f }
        : ImVec4{ 0.12f,0.16f,0.24f,1.f });
        if (ImGui::Button(n.label, { sw - 16.f, 38.f }))
            bank_state::current_view = n.view;
        ImGui::PopStyleColor(2);
        ImGui::Spacing();
    }

    // Toast (fades out with alpha)
    if (bank_state::status_timer > 0.f) {
        float alpha = (bank_state::status_timer < 1.f) ? bank_state::status_timer : 1.f;
        ImGui::SetCursorPosY(display.y - 104.f);
        ImGui::Separator(); ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text,
            bank_state::status_is_error
            ? ImVec4{ 1.f,0.4f,0.4f,alpha }
        : ImVec4{ 0.3f,0.85f,0.5f,alpha });
        ImGui::PushTextWrapPos(sw - 8.f);
        ImGui::SetCursorPosX(8.f);
        ImGui::TextWrapped("%s", bank_state::status_msg);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    // Logout button pinned to the bottom of the sidebar
    ImGui::SetCursorPosY(display.y - 56.f);
    ImGui::Separator(); ImGui::Spacing();
    ImGui::SetCursorPosX(8.f);
    if (btn_red("  Logout  ", { sw - 16.f, 30.f }))
        bank_state::do_logout();

    ImGui::EndChild();
    ImGui::PopStyleColor();  // sidebar ChildBg

    // ── Main content area ─────────────────────────────────────
    ImGui::SameLine();
    float cw = display.x - sw - 16.f;
    float ch = display.y - 20.f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.08f,0.10f,0.14f,1.f });
    ImGui::BeginChild("##content", { cw,ch }, false);
    ImGui::Spacing();
    ImGui::SetCursorPosX(6.f);

    // View title
    static const char* titles[] = {
        "Client Management", "Employee Management", "Administration"
    };
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.86f,0.91f,1.f,1.f });
    ImGui::SetWindowFontScale(1.18f);
    ImGui::Text("%s", titles[(int)bank_state::current_view]);
    ImGui::SetWindowFontScale(1.f);
    ImGui::PopStyleColor();
    ImGui::Spacing();

    switch (bank_state::current_view) {
    case bank_state::View::Clients:   view_clients(cw - 10.f, ch); break;
    case bank_state::View::Employees: view_employees(cw - 10.f, ch); break;
    case bank_state::View::Admin:     view_admin(cw - 10.f, ch); break;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();  // content ChildBg

    ImGui::End();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}