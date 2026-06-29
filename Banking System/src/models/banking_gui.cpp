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
    static std::string current_position;
    static int         current_id;

    static bool is_logged_in() { return current_role != Role::None; }

    // ── Data ──────────────────────────────────────────────────
    static std::vector<Client>   clients;
    static std::vector<Employee> employees;
    static std::vector<Admin>    admins;

    // Row selection
    static int selected_client = -1;
    static int selected_employee = -1;
    static int selected_admin = -1;

    // Modal open flags (set true → OpenPopup on the same frame)
    static bool open_add_client = false;
    static bool open_delete_client = false;
    static bool open_edit_client = false;
    static bool open_remove_all_clients = false;
    static bool open_delete_employee = false;
    static bool open_remove_all_employee = false;
    static bool open_remove_all_admin = false;
    static bool open_deposit = false;
    static bool open_withdraw = false;
    static bool open_transfer = false;
    static bool open_add_employee = false;
    static bool open_edit_employee = false;
    static bool open_add_admin = false;
    static bool open_edit_admin = false;
    static bool open_remove_admin = false;

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
        current_position.clear();
        current_view = View::Clients;
        selected_client = -1;
        selected_employee = -1;
        selected_admin = -1;
        current_id = -1;
    }
}

static bool isAdminUsernameExist(std::string name_in) {

    // prevent recording same username
    for (auto& check : bank_state::admins)
    {
        if (name_in == check.getName())
        {
            return true;
        }
    }
    return false;
}

static bool isEmployeeUsernameExist(std::string name_in) {
    // prevent recording same username
    for (auto& check2 : bank_state::employees)
    {
        if (name_in == check2.getName())
        {
            return true;
        }

    }
    return false;

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
            bank_state::current_position = "Administrator";
            bank_state::current_view = bank_state::View::Clients;
            bank_state::current_id = a.getID();
            return true;
        }
    }
    for (auto& e : bank_state::employees) {
        if (e.getName() == name_in && e.getPassword() == pw_hash) {
            bank_state::current_role = bank_state::Role::Employee;
            bank_state::current_username = name_in;
            bank_state::current_position = e.getPosition();
            bank_state::current_view = bank_state::View::Clients;
            bank_state::current_id = e.getID();
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

// Case-insensitive substring match against name, plus literal substring
// match against ID and (optionally) a numeric amount (balance/salary).
static bool matches_search(const char* search, int id, const std::string& name,
    double amount = 0.0, bool has_amount = false)
{
    if (!search || !search[0]) return true;

    std::string needle(search);
    std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);

    std::string lname(name);
    std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
    if (lname.find(needle) != std::string::npos) return true;

    std::string id_str = std::to_string(id);
    if (id_str.find(needle) != std::string::npos) return true;

    //if (has_amount) {
    //    std::ostringstream ss;
    //    ss << std::fixed << std::setprecision(2) << amount;
    //    if (ss.str().find(needle) != std::string::npos) return true;

    //    std::string amt_int = std::to_string((long long)amount);
    //    if (amt_int.find(needle) != std::string::npos) return true;
    //}
    return false;
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

ImFont* customFontSize_01;
ImFont* customFontSize_02;
ImFont* customFontSize_03;

// =============================================================================
// Login Screen
// =============================================================================
static void draw_login_screen(const ImVec2& display)
{
    constexpr float CW = 450.f;
    constexpr float CH = 440.f;

    ImGui::SetCursorPos({ (display.x - CW) * 0.5f,
                         (display.y - CH) * 0.5f });

    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.10f,0.13f,0.20f,1.f });
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
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
    ImGui::PopStyleVar(2);

    constexpr float FX = 20.f;                   // left margin
    const     float FW = CW - FX * 2.f;          // field width

    // ── No admins recorded at all - force first-run signup ─────
    if (bank_state::admins.empty()) {
        static char sname[64] = {};
        static char spass[64] = {};
        static char sconfirm[64] = {};
        static char serr[160] = {};

        ImGui::SetCursorPosX(FX);
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.3f,0.85f,0.5f,1.f });
        ImGui::TextWrapped("No administrator account exists yet. Create the first admin to get started.");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::SetCursorPosX(FX);
        ImGui::TextDisabled("Admin Name");
        ImGui::SetCursorPosX(FX);
        ImGui::SetNextItemWidth(FW);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.4392f, 0.4471f, 0.4549f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 0.0f, 0.0f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_InputTextCursor, { 0.0f, 0.0f, 0.0f, 1.0f });
        ImGui::PushFont(customFontSize_01);
        ImGui::InputText("##su_name", sname, sizeof sname);
        ImGui::PopFont();
        ImGui::Spacing();

        ImGui::SetCursorPosX(FX);
        ImGui::TextDisabled("Password");
        ImGui::SetCursorPosX(FX);
        ImGui::SetNextItemWidth(FW);
        ImGui::PushFont(customFontSize_01);
        ImGui::InputText("##su_pass", spass, sizeof spass, ImGuiInputTextFlags_Password);
        ImGui::PopFont();
        ImGui::Spacing();

        ImGui::SetCursorPosX(FX);
        ImGui::TextDisabled("Confirm Password");
        ImGui::SetCursorPosX(FX);
        ImGui::SetNextItemWidth(FW);
        ImGui::PushFont(customFontSize_01);
        bool confirm_enter = ImGui::InputText("##su_confirm", sconfirm, sizeof sconfirm,
            ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopFont();

        ImGui::PopStyleColor(3);

        ImGui::Spacing();
        if (serr[0]) {
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.40f,0.40f,1.f });
            float ew = ImGui::CalcTextSize(serr).x;
            ImGui::SetCursorPosX(std::clamp((CW - ew) * 0.5f, FX, CW - FX));
            ImGui::TextWrapped("%s", serr);
            ImGui::PopStyleColor();
        }
        else {
            ImGui::Dummy({ 0.f, ImGui::GetTextLineHeight() });
        }
        ImGui::Spacing();

        constexpr float BW = 190.f;
        ImGui::SetCursorPosX((CW - BW) * 0.5f);
        bool create_clicked = ImGui::Button("  Create Admin & Sign In  ", { BW, 34.f });

        if (create_clicked || confirm_enter) {
            std::string n(sname), p(spass), c(sconfirm);
            if (!utils::Validation::validateName(n))
                strncpy_s(serr, sizeof serr, "Invalid name.", _TRUNCATE);
            else if (!utils::Validation::validatePassword(p))
                strncpy_s(serr, sizeof serr, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
            else if (p != c)
                strncpy_s(serr, sizeof serr, "Passwords do not match.", _TRUNCATE);
            else if (isEmployeeUsernameExist(n) || isAdminUsernameExist(n))
                strncpy_s(serr, sizeof serr, "Username exists, please try again.", _TRUNCATE);
            else {
                bank_state::admins.emplace_back(n, p);

                utils::FileHelper::saveAdmin(bank_state::admins.back());
                utils::FileHelper::saveLast(utils::FileHelper::get_DB_Struct().employeeID_DB, Employee::getGlobalID());

                serr[0] = '\0';
                bank_state::current_role = bank_state::Role::Admin;
                bank_state::current_username = n;
                bank_state::current_position = "Administrator";
                bank_state::current_view = bank_state::View::Clients;
                bank_state::current_id = bank_state::admins.back().getID();

                memset(sname, 0, sizeof sname);
                memset(spass, 0, sizeof spass);
                memset(sconfirm, 0, sizeof sconfirm);
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
    }

    // ── Input fields ──────────────────────────────────────────
    // Admin Test

    static char uname[64] = { 'W','a','n','d','a'};
    static char upass[64] = { 'L','o','r','e','n','1','2','3'};

    // Employee Test
    //static char uname[64] = { 'E','m','m','a',' ','D','a','v','i','s'};
    //static char upass[64] = { 'e','m','p','p','a','s','s','0','0','1'};

    // Client Test
    //static char uname[64] = { 'A','l','i','c','e',' ','S','m','i','t','h'};
    //static char upass[64] = { 'p','a','s','s','w','o','r','d','0','0','1' };

    static char err[160] = {};

    ImGui::SetCursorPosX(FX);
    ImGui::TextDisabled("Username");
    ImGui::SetCursorPosX(FX);
    ImGui::SetNextItemWidth(FW);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.4392f, 0.4471f, 0.4549f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 0.0f, 0.0f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_InputTextCursor, { 0.0f, 0.0f, 0.0f, 1.0f });
    ImGui::PushFont(customFontSize_01);
    bool user_enter = ImGui::InputText("##ln_u", uname, sizeof uname,
        ImGuiInputTextFlags_EnterReturnsTrue);
    // Enter in username box → jump focus to password
    if (user_enter) ImGui::SetKeyboardFocusHere();

    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::SetCursorPosX(FX);
    ImGui::TextDisabled("Password");
    ImGui::SetCursorPosX(FX);
    ImGui::SetNextItemWidth(FW);
    ImGui::PushFont(customFontSize_01);
    bool pass_enter = ImGui::InputText("##ln_p", upass, sizeof upass,
        ImGuiInputTextFlags_Password |
        ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::PopStyleColor(3);
    ImGui::PopFont();

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
    ImGui::PushFont(customFontSize_01);
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.42f,0.50f,0.64f,1.f });
    ImGui::TextWrapped("Welcome to our simple, secure, and trusted banking system."
        "\nSign in to access your accounts, manage your finances, and enjoy a seamless banking experience.");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// =============================================================================
// Modals
// =============================================================================

// ── Add Client ────────────────────────────────────────────────────────────────
static void modal_add_client()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(490, 320);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );

    if (bank_state::open_add_client) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Add Client"); bank_state::open_add_client = false; }
    if (!ImGui::BeginPopupModal("Add Client", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    static char name_buf[64] = {}; static char pass_buf[64] = {};
    static double init_bal = 1500.0; static char err[256] = {};
    ImGui::Indent();
    ImGui::TextDisabled("Create a new bank client account");
    ImGui::Separator(); ImGui::Spacing();

    auto vc = utils::Validation::get_ValidationStruct();
    ImGui::Text("Full Name:");    ImGui::SameLine(130.f); ImGui::SetNextItemWidth(210.f);
    ImGui::InputText("##c_name", name_buf, sizeof name_buf);
    ImGui::Text("Password:");     ImGui::SameLine(130.f); ImGui::SetNextItemWidth(210.f);
    ImGui::InputText("##c_pass", pass_buf, sizeof pass_buf, ImGuiInputTextFlags_Password);

    ImGui::Text("Init. Balance $:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(210.f);
    ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15.f);
    ImGui::InputDouble("##c_bal", &init_bal, 500.0, 1000.0, "%.2f");

    if (init_bal < 1.0) init_bal = 1.0;

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
    else
    {
        ImGui::Spacing();
        ImGui::TextWrapped("");
    }

    ImGui::Spacing(); ImGui::Separator();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.f);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 2) - 100.f);
    if (btn_green("Create", { 100.f,0.f })) {
        std::string n(name_buf), p(pass_buf);
        if (!utils::Validation::validateName(n))    strncpy_s(err, sizeof err, "Invalid name — check length and allowed characters.", _TRUNCATE);
        else if (!utils::Validation::validatePassword(p)) strncpy_s(err, sizeof err, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
        else if (!utils::Validation::validateBalance(init_bal)) strncpy_s(err, sizeof err, "Initial balance is below the minimum.", _TRUNCATE);
        else {
            bank_state::clients.emplace_back(n, p);
            bank_state::clients.back().setBalance(init_bal);
            utils::FileHelper::saveClient(bank_state::clients.back());
            utils::FileHelper::saveLast(utils::FileHelper::get_DB_Struct().clientID_DB, Client::getGlobalID());
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
    ImGui::Unindent();
    ImGui::EndPopup();

}

// ── Remove All Clients ────────────────────────────────────────────────────────────────
static void modal_remove_all_client()
{
    using namespace bank_state;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(400, 150);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (open_remove_all_clients) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Remove All Clients"); open_remove_all_clients = false; }
    if (!ImGui::BeginPopupModal("Remove All Clients", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;
    ImGui::Indent();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8549f, 0.3098f, 0.3098f, 1.0f));
    ImGui::TextWrapped("This is a destructive operation, once confirmed all clients data will be cleared!");
    ImGui::PopStyleColor();
    ImGui::Unindent();
    ImGui::Spacing();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.f);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 2) - 100.f);
    if (btn_red("Confirm", { 100.f,0.f })) {
        clients.clear();
        utils::FileHelper::removeAllClients();
        ImGui::CloseCurrentPopup();
        set_status("All Clients removed.");
    }
    ImGui::SameLine();
    if (btn_green("Cancel", { 100.f,0.f })) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::Unindent();
    ImGui::Unindent();
    ImGui::EndPopup();

}

// ── Deposit ───────────────────────────────────────────────────────────────────
static void modal_deposit()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(300, 190);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (bank_state::open_deposit) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Deposit"); bank_state::open_deposit = false; }
    if (!ImGui::BeginPopupModal("Deposit", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;
    ImGui::Indent();
    static double amount = 500.0;
    int idx = bank_state::selected_client;
    if (idx >= 0 && idx < (int)bank_state::clients.size()) {
        auto& c = bank_state::clients[idx];
        ImGui::Text("Client:  %s", c.getName().c_str());
        ImGui::Text("Balance: %s", fmt_money(c.getBalance()).c_str());
        ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Amount:"); ImGui::SameLine(0.f, 10.f);
        ImGui::Text("$"); ImGui::SameLine();
        ImGui::SetNextItemWidth(170.f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10.f); ImGui::SameLine();
        ImGui::InputDouble("##dep", &amount, 100.0, 1000.0, "%.2f");

        ImGui::Unindent();
        if (amount < 1.0) amount = 1.0;
        ImGui::Spacing();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 2) - 100.f);
        if (btn_green("Deposit", { 100.f,0.f })) {
            c.deposit(amount); bank_state::set_status("Deposit successful.");
            vector<pair<Client, int>> tmp;
            tmp.push_back({ c, 0 });
            utils::FileHelper::updateClient(tmp);
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

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(300, 220);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (bank_state::open_withdraw) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Withdraw"); bank_state::open_withdraw = false; }
    if (!ImGui::BeginPopupModal("Withdraw", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;
    ImGui::Indent();
    static double amount = 200.0; static char err[128] = {};
    int idx = bank_state::selected_client;
    if (idx >= 0 && idx < (int)bank_state::clients.size()) {
        auto& c = bank_state::clients[idx];
        ImGui::Text("Client:  %s", c.getName().c_str());
        ImGui::Text("Balance: %s", fmt_money(c.getBalance()).c_str());
        ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();
        ImGui::Text("Amount:"); ImGui::SameLine(0.f, 15.f);
        ImGui::Text("$"); ImGui::SameLine();
        ImGui::SetNextItemWidth(150.f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.f); ImGui::SameLine();
        ImGui::InputDouble("##wd", &amount, 100.0, 1000.0, "%.2f");
        if (amount < 1.0) amount = 1.0;
        if (err[0]) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
            ImGui::Text("  %s", err); ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Spacing();
            ImGui::TextWrapped("");
        }
        ImGui::Unindent();
        ImGui::Spacing();
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 2) - 100.f);
        if (btn_amber("Withdraw", { 100.f,0.f })) {
            if (amount > c.getBalance()) strncpy_s(err, sizeof err, "Insufficient balance.", _TRUNCATE);
            else {
                c.withdraw(amount); bank_state::set_status("Withdrawal successful.");
                vector<pair<Client, int>> tmp;
                tmp.push_back({ c, 0 });
                utils::FileHelper::updateClient(tmp);
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

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(400, 270);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (bank_state::open_transfer) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Transfer"); bank_state::open_transfer = false; }
    if (!ImGui::BeginPopupModal("Transfer", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    ImGui::Indent();
    static double amount = 200.0; static int target_idx = -1; static char err[128] = {};
    int src = bank_state::selected_client;
    if (src >= 0 && src < (int)bank_state::clients.size()) {
        auto& from = bank_state::clients[src];
        ImGui::Text("From:    %s", from.getName().c_str());
        ImGui::Text("Balance: %s", fmt_money(from.getBalance()).c_str());
        ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Amount:"); ImGui::SameLine(0.f, 10.f);
        ImGui::Text("$"); ImGui::SameLine();
        ImGui::SetNextItemWidth(210.f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10.f); ImGui::SameLine();
        ImGui::InputDouble("##tr_amt", &amount, 100.0, 1000.0, "%.2f");

        if (amount < 1.0) amount = 1.0;
        ImGui::Text("To:");    ImGui::SameLine(90.f); ImGui::SetNextItemWidth(210.f);
        std::string prev_label = (target_idx >= 0 && target_idx < (int)bank_state::clients.size())
            ? bank_state::clients[target_idx].getName()
            : "Select recipient...";
        // Show at most 5 items before scrolling
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(0, 0),
            ImVec2(FLT_MAX, ImGui::GetTextLineHeightWithSpacing() * 5)
        );
        if (ImGui::BeginCombo("##tr_to", prev_label.c_str())) {
            for (int i = 0; i < (int)bank_state::clients.size(); ++i) {
                if (i == src) continue;
                ImGui::PushID((i + 515) * 3);
                if (ImGui::Selectable(bank_state::clients[i].getName().c_str(), target_idx == i)) {
                    target_idx = i;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        if (err[0]) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
            ImGui::Text("  %s", err); ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Spacing();
            ImGui::TextWrapped("");
        }
        ImGui::Spacing();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 25.f);
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 2) - 100.f);
        if (ImGui::Button("Transfer", { 100.f,0.f })) {
            if (target_idx < 0 || target_idx >= (int)bank_state::clients.size())
                strncpy_s(err, sizeof err, "Select a recipient first.", _TRUNCATE);
            else if (amount > from.getBalance())
                strncpy_s(err, sizeof err, "Insufficient balance.", _TRUNCATE);
            else {
                from.transferTo(amount, bank_state::clients[target_idx]);
                vector<pair<Client, int>> tmp;
                tmp.push_back({ from, 0 });
                tmp.push_back({ bank_state::clients[target_idx], 0 });
                utils::FileHelper::updateClient(tmp);
                bank_state::set_status("Transfer completed.");
                amount = 200.0; target_idx = -1; err[0] = '\0'; ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
    }
    if (btn_red("Cancel", { 100.f,0.f })) { amount = 200.0; target_idx = -1; err[0] = '\0'; ImGui::CloseCurrentPopup(); }
    ImGui::Unindent();
    ImGui::EndPopup();
}

// ── Add Employee ──────────────────────────────────────────────────────────────
static void modal_add_employee()
{

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(380, 320);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (bank_state::open_add_employee) { ImGui::SetNextWindowSize(popup_size);  ImGui::OpenPopup("Add Employee"); bank_state::open_add_employee = false; }
    if (!ImGui::BeginPopupModal("Add Employee", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    static char name_buf[64] = {}; static char pass_buf[64] = {}; static char role_buf[48] = {};
    static double salary = 5000.0; static char err[256] = {};
    ImGui::Indent();
    ImGui::TextDisabled("Register a new bank employee");
    ImGui::Separator(); ImGui::Spacing();
    ImGui::Text("Full Name:"); ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##e_name", name_buf, sizeof name_buf);
    ImGui::Text("Password:"); ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##e_pass", pass_buf, sizeof pass_buf, ImGuiInputTextFlags_Password);
    ImGui::Text("Role:");     ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##e_role", role_buf, sizeof role_buf);


    ImGui::Text("Salary $:"); ImGui::SameLine(120.f);
    ImGui::SetNextItemWidth(210.f);
    ImGui::InputDouble("##e_sal", &salary, 500.0, 1000.0, "%.2f");

    if (salary < 1.0) salary = 1.0;

    if (err[0]) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
        ImGui::TextWrapped("  %s", err);
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::Spacing();
        ImGui::TextWrapped("");
    }
    ImGui::Spacing(); ImGui::Separator();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.f);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 4));
    if (btn_green("Add", { 100.f,0.f })) {
        std::string n(name_buf), p(pass_buf), r(role_buf);
        if (!utils::Validation::validateName(n))    strncpy_s(err, sizeof err, "Invalid name.", _TRUNCATE);
        else if (!utils::Validation::validatePassword(p)) strncpy_s(err, sizeof err, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
        else if (r.empty())                              strncpy_s(err, sizeof err, "Role cannot be empty.", _TRUNCATE);
        else if (!utils::Validation::validateSalary(salary)) strncpy_s(err, sizeof err, "Salary below minimum ($5000).", _TRUNCATE);
        else if (isEmployeeUsernameExist(n)) {
            strncpy_s(err, sizeof err, "Username exists, please try again.", _TRUNCATE);
        }
        else if (isAdminUsernameExist(n)) {
            strncpy_s(err, sizeof err, "Username exists, please try again.", _TRUNCATE);
        }
        else
        {
            bank_state::employees.emplace_back(n, p, r, salary);
            utils::FileHelper::saveEmployee(bank_state::employees.back());
            utils::FileHelper::saveLast(utils::FileHelper::get_DB_Struct().employeeID_DB, Employee::getGlobalID());
            memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf); memset(role_buf, 0, sizeof role_buf);
            salary = 5000.0; err[0] = '\0';
            bank_state::set_status("Employee added.");
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (btn_red("Cancel", { 100.f,0.f })) {
        memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf); memset(role_buf, 0, sizeof role_buf);
        salary = 5000.0; err[0] = '\0'; ImGui::CloseCurrentPopup();
    }
    ImGui::Unindent();
    //ImGui::Dummy(ImVec2(0.f, 10.f));
    ImGui::EndPopup();
}

// ── Remove All Employees ────────────────────────────────────────────────────────────────
static void modal_remove_all_employee()
{
    using namespace bank_state;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(400, 150);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (open_remove_all_employee) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Remove All Employees"); open_remove_all_employee = false; }
    if (!ImGui::BeginPopupModal("Remove All Employees", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;
    ImGui::Indent();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8549f, 0.3098f, 0.3098f, 1.0f));
    ImGui::TextWrapped("This is a destructive operation, once confirmed all employee data will be cleared!");
    ImGui::PopStyleColor();
    ImGui::Unindent();
    ImGui::Spacing();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.f);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 2) - 100.f);
    if (btn_red("Confirm", { 100.f,0.f })) {
        employees.clear();
        utils::FileHelper::clearInfoFile(utils::FileHelper::get_DB_Struct().employeeDB);
        ImGui::CloseCurrentPopup();
        set_status("All Employees removed.");
    }
    ImGui::SameLine();
    if (btn_green("Cancel", { 100.f,0.f })) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::Unindent();
    ImGui::Unindent();
    ImGui::EndPopup();

}

// ── Remove All Admin ────────────────────────────────────────────────────────────────
static void modal_remove_all_admin()
{
    using namespace bank_state;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(400, 180);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (open_remove_all_admin) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Remove All Admins"); open_remove_all_admin = false; }
    if (!ImGui::BeginPopupModal("Remove All Admins", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;
    ImGui::Indent();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8549f, 0.3098f, 0.3098f, 1.0f));
    ImGui::TextWrapped("This is a destructive operation, once confirmed all admin data will be cleared!");
    ImGui::PopStyleColor();
    ImGui::TextColored(ImVec4(0.1922f, 1.0f, 0.3176f, 1.0f),"(Logged-in admin is excluded)");
    ImGui::Unindent();
    ImGui::Spacing();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.f);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 2) - 100.f);
    if (btn_red("Confirm", { 100.f,0.f })) {
        Admin loggedAdmin;
        for (auto& admin : admins) {
            if (admin.getID() == current_id) {
                loggedAdmin = admin;
                break;
            }
        }
        admins.clear();
        admins.push_back(loggedAdmin);
        utils::FileHelper::clearInfoFile(utils::FileHelper::get_DB_Struct().adminDB);
        utils::FileHelper::saveAdmin(loggedAdmin);
        ImGui::CloseCurrentPopup();
        set_status("All Admins removed.");
    }
    ImGui::SameLine();
    if (btn_green("Cancel", { 100.f,0.f })) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::Unindent();
    ImGui::Unindent();
    ImGui::EndPopup();

}


// ── Edit Employee ────────────────────────────────────────────────────────────────
static void modal_edit_employee()
{

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(420, 400);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (bank_state::open_edit_employee) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Edit Employee"); bank_state::open_edit_employee = false; }
    if (!ImGui::BeginPopupModal("Edit Employee", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    static char name_buf[64] = {}; static char pass_buf[64] = {}; static char pass_Conf[64] = {}; static char role_buf[48] = {};
    static double new_sal = 5000.0; static char err[256] = {};
    int idx = bank_state::selected_employee;
    if (idx >= 0 && idx < (int)bank_state::employees.size()) {
        auto& e = bank_state::employees[idx];
        ImGui::Indent();
        ImGui::Text("Employee: %s", e.getName().c_str());
        ImGui::Text("Position: %s", e.getPosition().c_str());
        ImGui::Text("Salary:  %s", fmt_money(e.getSalary()).c_str());
        ImGui::Separator(); ImGui::Spacing();

        ImGui::PushFont(customFontSize_01);
        ImGui::TextColored(ImVec4(0.6941f, 0.749f, 0.9922f, 1.0f), "Edit employee data: ");
        ImGui::PopFont();
        ImGui::Spacing();
        ImGui::Text("New Full Name:"); ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##edit_e_name", name_buf, sizeof name_buf);
        ImGui::Text("New Password:"); ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##edit_e_pass", pass_buf, sizeof pass_buf, ImGuiInputTextFlags_Password);

        ImGui::Text("Confirm Password"); ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f);
        ImGui::PushFont(customFontSize_01);
        bool confirm_enter = ImGui::InputText("##su_confirm", pass_Conf, sizeof pass_Conf,
            ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopFont();
        ImGui::Text("New Role:");     ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##edit_e_role", role_buf, sizeof role_buf);
        ImGui::Text("New Salary:"); ImGui::SameLine(160.f);
        ImGui::SetNextItemWidth(170.f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.f);
        ImGui::InputDouble("##ns", &new_sal, 500.0, 1000.0, "%.2f");
        if (new_sal < 1.0) new_sal = 1.0;

        if (err[0]) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
            ImGui::TextWrapped("  %s", err);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Spacing();
            ImGui::TextWrapped("");
        }

        ImGui::Separator();

        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 4));
        if (btn_amber("Update", { 100.f,0.f })) {
            std::string n(name_buf), p(pass_buf), r(role_buf), c(pass_Conf);
            if (!n.empty() && !utils::Validation::validateName(n))    strncpy_s(err, sizeof err, "Invalid name.", _TRUNCATE);
            else if (!p.empty() && !utils::Validation::validatePassword(p)) strncpy_s(err, sizeof err, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
            //else if (!r.empty())                              strncpy_s(err, sizeof err, "Role cannot be empty.", _TRUNCATE);
            else if (!utils::Validation::validateSalary(new_sal)) strncpy_s(err, sizeof err, "Salary below minimum ($5000).", _TRUNCATE);
            else if (p != c)  strncpy_s(err, sizeof err, "Passwords do not match.", _TRUNCATE);
            else if (isEmployeeUsernameExist(n)) {
                strncpy_s(err, sizeof err, "Username exists, please try again.", _TRUNCATE);
            }
            else if (isAdminUsernameExist(n)) {
                strncpy_s(err, sizeof err, "Username exists, please try again.", _TRUNCATE);
            }
            else
            {

                if (!n.empty())
                    e.setName(n);

                if (!p.empty())
                    e.setPassword(p);

                if (!r.empty())
                    e.setPosition(r);

                e.setSalary(new_sal);

                vector<pair<Employee, int>> tmpEmp;
                tmpEmp.push_back({e,0});
                utils::FileHelper::updateEmployee(tmpEmp);
                memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf); memset(role_buf, 0, sizeof role_buf);
                new_sal = 5000.0; err[0] = '\0';
                bank_state::set_status("Employee Updated.");
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
    }
    if (btn_red("Cancel", { 100.f,0.f })) { new_sal = 5000.0; err[0] = '\0'; ImGui::CloseCurrentPopup(); }
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::EndPopup();
}

// ── Edit Client ────────────────────────────────────────────────────────────────
static void modal_edit_client()
{

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(420, 350);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (bank_state::open_edit_client) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Edit Client"); bank_state::open_edit_client = false; }
    if (!ImGui::BeginPopupModal("Edit Client", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    static char name_buf[64] = {}; static char pass_buf[64] = {}; static char pass_Conf[64] = {};
    static double new_bal = 1500.0; static char err[256] = {};
    int idx = bank_state::selected_client;
    if (idx >= 0 && idx < (int)bank_state::clients.size()) {
        auto& client = bank_state::clients[idx];
        ImGui::Indent();
        ImGui::Text("Client: %s", client.getName().c_str());
        ImGui::Text("Balance:  %s", fmt_money(client.getBalance()).c_str());
        ImGui::Separator(); ImGui::Spacing();

        ImGui::PushFont(customFontSize_01);
        ImGui::TextColored(ImVec4(0.6941f, 0.749f, 0.9922f, 1.0f), "Edit client data: ");
        ImGui::PopFont();
        ImGui::Spacing();
        ImGui::Text("New Full Name:"); ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##edit_e_name", name_buf, sizeof name_buf);
        ImGui::Text("New Password:"); ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##edit_e_pass", pass_buf, sizeof pass_buf, ImGuiInputTextFlags_Password);

        ImGui::Text("Confirm Password"); ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f);
        ImGui::PushFont(customFontSize_01);
        bool confirm_enter = ImGui::InputText("##su_confirm", pass_Conf, sizeof pass_Conf,
            ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopFont();
        
        ImGui::Text("New Balance:"); ImGui::SameLine(160.f);
        ImGui::SetNextItemWidth(170.f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.f);
        ImGui::InputDouble("##ns", &new_bal, 500.0, 1000.0, "%.2f");
        if (new_bal < 1.0) new_bal = 1.0;

        if (err[0]) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
            ImGui::TextWrapped("  %s", err);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Spacing();
            ImGui::TextWrapped("");
        }

        ImGui::Separator();

        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 4));
        if (btn_amber("Update", { 100.f,0.f })) {
            std::string n(name_buf), p(pass_buf), c(pass_Conf);
            if (!n.empty() && !utils::Validation::validateName(n))    strncpy_s(err, sizeof err, "Invalid name.", _TRUNCATE);
            else if (!p.empty() && !utils::Validation::validatePassword(p)) strncpy_s(err, sizeof err, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
            else if (!utils::Validation::validateBalance(new_bal)) strncpy_s(err, sizeof err, "Balance below minimum ($1500).", _TRUNCATE);
            else if (p != c)  strncpy_s(err, sizeof err, "Passwords do not match.", _TRUNCATE);
            else
            {

                if (!n.empty())
                    client.setName(n);

                if (!p.empty())
                    client.setPassword(p);

                client.setBalance(new_bal);

                vector<pair<Client, int>> tmpCli;
                tmpCli.push_back({ client,0});
                utils::FileHelper::updateClient(tmpCli);
                memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf);
                new_bal = 1500.0; err[0] = '\0';
                bank_state::set_status("Client Updated.");
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
    }
    if (btn_red("Cancel", { 100.f,0.f })) { new_bal = 1500.0; err[0] = '\0'; ImGui::CloseCurrentPopup(); }
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::EndPopup();
}

// ── Edit Admin ────────────────────────────────────────────────────────────────
static void modal_edit_admin()
{

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(420, 310);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (bank_state::open_edit_admin) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Edit Admin"); bank_state::open_edit_admin = false; }
    if (!ImGui::BeginPopupModal("Edit Admin", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    static char name_buf[64] = {}; static char pass_buf[64] = {}; static char pass_Conf[64] = {}; static char err[256] = {};
    int idx = bank_state::selected_admin;
    if (idx >= 0 && idx < (int)bank_state::admins.size()) {
        auto& a = bank_state::admins[idx];
        ImGui::Indent();
        ImGui::Text("Admin: %s", a.getName().c_str());
        ImGui::Text("Position: %s", a.getPosition().c_str());
        ImGui::Separator(); ImGui::Spacing();

        ImGui::PushFont(customFontSize_01);
        ImGui::TextColored(ImVec4(0.6941f, 0.749f, 0.9922f, 1.0f), "Edit admin data: ");
        ImGui::PopFont();
        ImGui::Spacing();
        ImGui::Text("New Full Name:"); ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##edit_e_name", name_buf, sizeof name_buf);
        ImGui::Text("New Password:"); ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##edit_e_pass", pass_buf, sizeof pass_buf, ImGuiInputTextFlags_Password);

        ImGui::Text("Confirm Password"); ImGui::SameLine(160.f); ImGui::SetNextItemWidth(210.f);
        ImGui::PushFont(customFontSize_01);
        bool confirm_enter = ImGui::InputText("##su_confirm", pass_Conf, sizeof pass_Conf,
            ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopFont();
        
        if (err[0]) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
            ImGui::TextWrapped("  %s", err);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Spacing();
            ImGui::TextWrapped("");
        }

        ImGui::Separator();

        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 4));
        if (btn_amber("Update", { 100.f,0.f })) {
            std::string n(name_buf), p(pass_buf), c(pass_Conf);
            if (!n.empty() && !utils::Validation::validateName(n))    strncpy_s(err, sizeof err, "Invalid name.", _TRUNCATE);
            else if (!p.empty() && !utils::Validation::validatePassword(p)) strncpy_s(err, sizeof err, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
            else if (p != c)  strncpy_s(err, sizeof err, "Passwords do not match.", _TRUNCATE);
            else if (isEmployeeUsernameExist(n)) {
                strncpy_s(err, sizeof err, "Username exists, please try again.", _TRUNCATE);
            }
            else if (isAdminUsernameExist(n)) {
                strncpy_s(err, sizeof err, "Username exists, please try again.", _TRUNCATE);
            }
            else
            {

                if (!n.empty())
                    a.setName(n);

                if (!p.empty())
                    a.setPassword(p);

                vector<pair<Admin, int>> tmpAdm;
                tmpAdm.push_back({ a,0 });
                utils::FileHelper::updateAdmin(tmpAdm);
                if (a.getID() == bank_state::current_id)
                    bank_state::current_username = a.getName();
                memset(name_buf, 0, sizeof name_buf); memset(pass_buf, 0, sizeof pass_buf);
                err[0] = '\0';
                bank_state::set_status("Admin Updated.");
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
    }
    if (btn_red("Cancel", { 100.f,0.f })) { err[0] = '\0'; ImGui::CloseCurrentPopup(); }
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::EndPopup();
}

// ── Add Admin ─────────────────────────────────────────────────────────────────
static void modal_add_admin()
{

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(350, 250);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (bank_state::open_add_admin) { ImGui::SetNextWindowSize(ImVec2(350, 250)); ImGui::OpenPopup("Add Administrator"); bank_state::open_add_admin = false; }
    if (!ImGui::BeginPopupModal("Add Administrator", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    static char name_buf[64] = {}; static char pass_buf[64] = {}; static char err[256] = {};
    ImGui::Indent();
    ImGui::TextDisabled("Grant administrator access");
    ImGui::Separator(); ImGui::Spacing();
    ImGui::Text("Full Name:"); ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##a_name", name_buf, sizeof name_buf);
    ImGui::Text("Password:"); ImGui::SameLine(120.f); ImGui::SetNextItemWidth(210.f); ImGui::InputText("##a_pass", pass_buf, sizeof pass_buf, ImGuiInputTextFlags_Password);
    if (err[0]) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, { 1.f,0.4f,0.4f,1.f });
        ImGui::TextWrapped("  %s", err);
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::Spacing();
        ImGui::TextWrapped("");
    }
    ImGui::Spacing(); ImGui::Separator();
    ImGui::Unindent();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.f);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x / 2) - 100.f);
    if (btn_green("Create", { 100.f,0.f })) {
        std::string n(name_buf), p(pass_buf);
        if (!utils::Validation::validateName(n))    strncpy_s(err, sizeof err, "Invalid name.", _TRUNCATE);
        else if (!utils::Validation::validatePassword(p)) strncpy_s(err, sizeof err, "Invalid password — 8-20 chars, no spaces.", _TRUNCATE);
        else if (isEmployeeUsernameExist(n)) {
            strncpy_s(err, sizeof err, "Username exists, please try again.", _TRUNCATE);
        }
        else if (isAdminUsernameExist(n)) {
            strncpy_s(err, sizeof err, "Username exists, please try again.", _TRUNCATE);
        }
        else {
            bank_state::admins.emplace_back(n, p);
            utils::FileHelper::saveAdmin(bank_state::admins.back());
            utils::FileHelper::saveLast(utils::FileHelper::get_DB_Struct().employeeID_DB, Employee::getGlobalID());
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

static void modal_confirm_client_deletion(int clientIdx)
{

    using namespace bank_state;
    if (clientIdx < 0 || clientIdx >= (int)clients.size()) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(300, 200);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (open_delete_client) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Delete Client"); open_delete_client = false; }
    if (!ImGui::BeginPopupModal("Delete Client", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    ImGui::Indent();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Client Info:");
    ImGui::Indent();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "ID: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%d", clients[clientIdx].getID());
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Name: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", clients[clientIdx].getName());
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Balance: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", fmt_money(clients[clientIdx].getBalance()));

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.f);
    if (btn_red("Confirm", { 100.f,0.f })) {
        vector<pair<Client, int>> clientPlaceholder;
        clientPlaceholder.push_back({ clients[clientIdx],0 });
        utils::FileHelper::removeClient(clientPlaceholder);
        clients.erase(clients.begin() + clientIdx);    // Memory removal
        if (selected_client >= (int)clients.size()) selected_client = (int)clients.size() - 1;
        clientIdx = -1;
        ImGui::CloseCurrentPopup();
        set_status("Client removed.");
        //Client::decreaseStaticID();
    }
    ImGui::SameLine();
    if (btn_green("Cancel", { 100.f,0.f })) {
        clientIdx = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::Unindent();
    ImGui::Unindent();
    ImGui::EndPopup();

}

static void modal_confirm_employee_deletion(int employeeIdx)
{

    using namespace bank_state;
    if (employeeIdx < 0 || employeeIdx >= (int)employees.size()) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(300, 200);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (open_delete_employee) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Delete Employee"); open_delete_employee = false; }
    if (!ImGui::BeginPopupModal("Delete Employee", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    ImGui::Indent();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Employee Info:");
    ImGui::Indent();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "ID: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%d", employees[employeeIdx].getID());
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Name: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", employees[employeeIdx].getName());
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Salary: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", fmt_money(employees[employeeIdx].getSalary()));

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.f);
    if (btn_red("Confirm", { 100.f,0.f })) {
        vector<pair<Employee, int>> employeePlaceholder;
        employeePlaceholder.push_back({ employees[employeeIdx],0 });
        utils::FileHelper::removeEmployee(employeePlaceholder);
        employees.erase(employees.begin() + employeeIdx);
        if (selected_employee >= (int)employees.size()) selected_employee = (int)employees.size() - 1;
        employeeIdx = -1;
        ImGui::CloseCurrentPopup();
        set_status("Employee removed.");
        //Employee::decreaseStaticID();
    }
    ImGui::SameLine();
    if (btn_green("Cancel", { 100.f,0.f })) {
        employeeIdx = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::Unindent();
    ImGui::Unindent();
    ImGui::EndPopup();

}

static void modal_confirm_admin_deletion(int adminIdx)
{

    using namespace bank_state;
    if (adminIdx < 0 || adminIdx >= (int)admins.size()) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display = io.DisplaySize;
    ImVec2 popup_size = ImVec2(300, 170);

    ImGui::SetNextWindowPos(
        ImVec2(
            (io.DisplaySize.x - popup_size.x) * 0.5f,
            (io.DisplaySize.y - popup_size.y) * 0.5f
        ),
        ImGuiCond_Always
    );
    if (open_remove_admin) { ImGui::SetNextWindowSize(popup_size); ImGui::OpenPopup("Delete Admin"); open_remove_admin = false; }
    if (!ImGui::BeginPopupModal("Delete Admin", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) return;

    ImGui::Indent();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Admin Info:");
    ImGui::Indent();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "ID: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%d", admins[adminIdx].getID());
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Name: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", admins[adminIdx].getName());

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.f);
    if (btn_red("Confirm", { 100.f,0.f })) {
        vector<pair<Admin, int>> adminPlaceholder;
        adminPlaceholder.push_back({ admins[adminIdx],0 });
        utils::FileHelper::removeAdmin(adminPlaceholder);
        admins.erase(admins.begin() + adminIdx);
        if (selected_admin >= (int)admins.size()) selected_admin = (int)admins.size() - 1;
        adminIdx = -1;
        ImGui::CloseCurrentPopup();
        set_status("Admin removed.");
        //Employee::decreaseStaticID();
    }
    ImGui::SameLine();
    if (btn_green("Cancel", { 100.f,0.f })) {
        adminIdx = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::Unindent();
    ImGui::Unindent();
    ImGui::EndPopup();

}



// =============================================================================
// Views
// =============================================================================

// Row centering helper
void TableCenteredY(float row_height)
{
    float h = ImGui::GetFrameHeight();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (row_height - h) * 0.5f);
}


// ── Client Management ─────────────────────────────────────────────────────────
static void view_clients(float w, float h)
{
    using namespace bank_state;
    double total_bal = 0.0;
    for (const auto& c : clients) total_bal += c.getBalance();
    stats_bar("Total Clients:", std::to_string(clients.size()).c_str(), { 0.40f,0.70f,1.f,1.f },
        "Total on Deposit:", fmt_money(total_bal).c_str(), { 0.30f,0.85f,0.50f,1.f }, w);
    ImGui::Spacing();

    // ── Search bar ───────────────────────────────────────────
    static char client_search[64] = {};
    ImGui::SetNextItemWidth(280.f);
    ImGui::InputTextWithHint("##client_search", "Search by ID or name...",
        client_search, sizeof client_search);
    if (client_search[0]) {
        ImGui::SameLine();
        if (ImGui::Button("Clear##cs")) client_search[0] = '\0';
    }
    ImGui::Spacing();

    static int delete_idx = -1;
    float table_h = h - 168.f - 36.f;   // shrink table a bit to make room for the bar
    float row_h = 35.f;

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
            if (!matches_search(client_search, clients[i].getID(), clients[i].getName(),
                clients[i].getBalance(), true))
                continue;

            ImGui::TableNextRow(ImGuiTableRowFlags_None, row_h);
            bool sel = (selected_client == i);
            ImGui::TableSetColumnIndex(0);
            char lbl[24]; snprintf(lbl, sizeof lbl, "%d##cs%d", clients[i].getID(), i);
            if (ImGui::Selectable(lbl, sel,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, { 0,row_h }))
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
            TableCenteredY(row_h);
            ImGui::PushID(i);
            if (btn_green("Deposit", { 72.f,0.f })) { selected_client = i; open_deposit = true; }
            ImGui::SameLine();
            if (btn_amber("Withdraw", { 76.f,0.f })) { selected_client = i; open_withdraw = true; }
            ImGui::SameLine();
            if (ImGui::Button("Transfer", { 76.f,0.f })) { selected_client = i; open_transfer = true; }
            ImGui::SameLine();
            if (btn_red("X", { 26.f,0.f })) { delete_idx = i; open_delete_client = true; }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    if (btn_green("Add Client", { 130.f,32.f })) open_add_client = true;

    ImGui::SameLine();
    if (selected_client >= 0)
        if (btn_amber("Edit Client", { 135.f,32.f })) open_edit_client = true;

    ImGui::SameLine();
    if (clients.size() >= 1 && btn_red("Remove all Clients", { 160.f,32.f })) open_remove_all_clients = true;

    modal_add_client(); modal_edit_client(); modal_deposit(); modal_withdraw(); modal_transfer(); modal_confirm_client_deletion(delete_idx); modal_remove_all_client();
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

    // ── Search bar ───────────────────────────────────────────
    static char employee_search[64] = {};
    ImGui::SetNextItemWidth(280.f);
    ImGui::InputTextWithHint("##employee_search", "Search by ID or name...",
        employee_search, sizeof employee_search);
    if (employee_search[0]) {
        ImGui::SameLine();
        if (ImGui::Button("Clear##es")) employee_search[0] = '\0';
    }
    ImGui::Spacing();

    float table_h = h - 168.f - 36.f;
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
        ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Salary", ImGuiTableColumnFlags_WidthFixed, 120.f);
        ImGui::TableHeadersRow();
        for (int i = 0; i < (int)employees.size(); ++i) {
            if (!matches_search(employee_search, employees[i].getID(), employees[i].getName(),
                employees[i].getSalary(), true))
                continue;

            ImGui::TableNextRow(ImGuiTableRowFlags_None, 26.f);
            ImGui::TableSetColumnIndex(0);
            char lbl[24]; snprintf(lbl, sizeof lbl, "%d##es%d", employees[i].getID(), i);
            if (ImGui::Selectable(lbl, selected_employee == i,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, { 0,24.f }))
                selected_employee = i;
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", employees[i].getName().c_str());
            ImGui::TableSetColumnIndex(2); ImGui::TextDisabled("%s", employees[i].getPosition().c_str());
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
    if (btn_green("Add Employee", { 145.f,32.f })) open_add_employee = true;
    if (selected_employee >= 0 && selected_employee < (int)employees.size()) {
        ImGui::SameLine();
        if (btn_amber("Edit Employee", { 130.f,32.f })) open_edit_employee = true;
        ImGui::SameLine();
        if (btn_red("Remove", { 90.f,32.f }))  open_delete_employee = true;
    }
    ImGui::SameLine();
    if (employees.size() >= 1 && btn_red("Remove All Employees", { 170.f,32.f })) open_remove_all_employee = true;
    modal_add_employee(); modal_edit_employee(); modal_confirm_employee_deletion(selected_employee); modal_remove_all_employee();
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

    // ── Search bar ───────────────────────────────────────────
    static char admin_search[64] = {};
    ImGui::SetNextItemWidth(280.f);
    ImGui::InputTextWithHint("##admin_search", "Search by ID or name...",
        admin_search, sizeof admin_search);
    if (admin_search[0]) {
        ImGui::SameLine();
        if (ImGui::Button("Clear##ads")) admin_search[0] = '\0';
    }
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.06f,0.07f,0.11f,1.f });
    ImGui::BeginChild("##admin_tbl", { w,170.f }, true);
    if (ImGui::BeginTable("tbl_admins", 2,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60.f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();


        for (int i = 0; i < (int)admins.size(); ++i) {
            if (!matches_search(admin_search, admins[i].getID(), admins[i].getName()))
                continue;
            ImGui::TableNextRow(ImGuiTableRowFlags_None, 26.f);
            ImGui::TableSetColumnIndex(0);
            char lbl[24]; snprintf(lbl, sizeof lbl, "%d##as%d", admins[i].getID(), i);
            if (ImGui::Selectable(lbl, selected_admin == i ,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, { 0,24.f }))
                    selected_admin = i;
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", admins[i].getName().c_str());
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.30f,0.85f,0.50f,1.f });
            ImGui::PopStyleColor();
        }


        //for (auto& a : admins) {
        //    ImGui::TableNextRow();
        //    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", a.getID());
        //    ImGui::TableSetColumnIndex(1); ImGui::Text("%s", a.getName().c_str());
        //}
        ImGui::EndTable();
    }
    ImGui::EndChild(); ImGui::PopStyleColor();

    ImGui::Spacing();
    if (btn_green("+ Add Administrator", { 170.f,32.f })) open_add_admin = true;
    if (selected_admin >= 0 && selected_admin < (int)admins.size()) {
        ImGui::SameLine();
        if (btn_amber("Edit Admin", { 115.f,32.f })) open_edit_admin = true;
        ImGui::SameLine();
        if (admins[selected_admin].getID() != current_id) if(btn_red("Remove Admin", { 110.f,32.f }))  open_remove_admin = true;
        
    }

    ImGui::SameLine();
    if (admins.size() >= 2 && btn_red("Remove All Admins", { 170.f,32.f })) open_remove_all_admin = true;
    ImGui::Spacing(); ImGui::Spacing();

    // Employee Roster Removed for reduduncy
    // =====================================
    //ImGui::PushStyleColor(ImGuiCol_Text, { 0.65f,0.78f,1.f,1.f });
    //ImGui::SetWindowFontScale(1.05f); ImGui::Text("Employee Roster");
    //ImGui::SetWindowFontScale(1.f); ImGui::PopStyleColor();
    //ImGui::Separator(); ImGui::Spacing();

    //ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.06f,0.07f,0.11f,1.f });
    //ImGui::BeginChild("##emp_ro", { w,160.f }, true);
    //if (ImGui::BeginTable("tbl_emp_ro", 3,
    //    ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
    //    ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
    //{
    //    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 52.f);
    //    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.f);
    //    ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthStretch, 0.6f);
    //    ImGui::TableHeadersRow();
    //    for (auto& e : employees) {
    //        ImGui::TableNextRow();
    //        ImGui::TableSetColumnIndex(0); ImGui::Text("%d", e.getID());
    //        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", e.getName().c_str());
    //        ImGui::TableSetColumnIndex(2); ImGui::TextDisabled("%s", e.getPosition().c_str());
    //    }
    //    ImGui::EndTable();
    //}
    //ImGui::EndChild(); ImGui::PopStyleColor();

    modal_add_admin(); modal_edit_admin(); modal_confirm_admin_deletion(selected_admin); modal_remove_all_admin();
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
    s.WindowPadding = { 0.f,0.f }; s.FramePadding = { 8.f,5.f }; s.ItemSpacing = { 8.f,6.f };
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
        18.0f, &cfg, k_font_ranges);

    customFontSize_01 = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 20.0f);
    customFontSize_02 = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 24.0f);
    customFontSize_03 = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 28.0f);

    if (!seg_font) {
        // Fallback: built-in pixel font (ASCII only, no UTF-8 extension)
        io.Fonts->AddFontDefault();
    }
    // The D3D11 backend rebuilds the font atlas on the first NewFrame() call.

    // ── Backend init ──────────────────────────────────────────
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dev, ctx);

    // ── Seed demo data ────────────────────────────────────────

    utils::FileHelper::fetchClients();
    utils::FileHelper::fetchEmployees();
    utils::FileHelper::fetchAdmins();
    bank_state::clients = Client::getClientList();
    bank_state::employees = Employee::getEmployeeList();
    bank_state::admins = Admin::getAdminList();

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
void banking_gui::tick(ID3D11ShaderResourceView* bgImg = nullptr, int img_width = 0, int img_height = 0)
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
        //ImGui::Image((void*)bgImg, ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight()));
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

    const float sw = 220.f;   // sidebar width

    // ── Sidebar ───────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.05f,0.06f,0.09f,1.f });
    ImGui::BeginChild("##sidebar", { sw, display.y - 0.f }, false);

    // Logo
    ImGui::SetCursorPos({ 14.f,14.f });
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.40f,0.68f,1.f,1.f });
    ImGui::SetWindowFontScale(1.22f);
    ImGui::Text("BankSys");
    ImGui::SetWindowFontScale(1.f);
    ImGui::PopStyleColor();
    ImGui::SetCursorPosX(14.f);
    ImGui::TextDisabled("TeamWicked Banking System");

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    // Logged-in user info
    ImGui::SetCursorPosX(14.f);
    ImGui::SetWindowFontScale(1.22f);
    ImGui::Text("User Info:");
    ImGui::SetWindowFontScale(1.f);
    ImGui::SetCursorPosX(24.f);
    ImGui::PushStyleColor(ImGuiCol_Text, { 0.78f,0.88f,1.f,1.f });
    ImGui::Text("Username: %.20s", bank_state::current_username.c_str());
    ImGui::PopStyleColor();
    ImGui::SetCursorPosX(24.f);
    ImGui::TextDisabled("Role: %s",
        bank_state::current_role == bank_state::Role::Admin
        ? "Administrator" : "Employee");
    ImGui::SetCursorPosX(24.f);
    ImGui::TextDisabled("Position: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored(
        bank_state::current_role == bank_state::Role::Admin ? ImVec4(0.9608f, 0.3059f, 0.3059f, 1.0f) :
        ImVec4(0.5294f, 0.9137f, 0.3569f, 1.0f), "%s", bank_state::current_position.c_str());

    ImGui::SetCursorPosX(24.f);
    ImGui::TextDisabled("User ID: ");
    ImGui::SameLine(0.f, 2.f);
    ImGui::TextColored( ImVec4(0.8157f, 0.502f, 0.9255f, 1.0f), "%d", bank_state::current_id);
    ImGui::SetCursorPosX(14.f);
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
        ImGui::SetCursorPosX(14.f);
        if (ImGui::Button(n.label, { sw - 28.f, 38.f }))
            bank_state::current_view = n.view;
        ImGui::PopStyleColor(2);
        ImGui::Spacing();
    }

    // Toast (fades out with alpha)
    if (bank_state::status_timer > 0.f) {
        float alpha = (bank_state::status_timer < 1.f) ? bank_state::status_timer : 1.f;
        ImGui::SetCursorPosY(display.y - 200.f);
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
    ImGui::SetCursorPosY(display.y - 70.f);
    ImGui::Separator(); ImGui::Spacing();
    ImGui::SetCursorPosX(8.f);
    if (btn_red("  Logout  ", { sw - 16.f, 35.f }))
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