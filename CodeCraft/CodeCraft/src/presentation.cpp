/*
 * presentation.cpp
 * Presentation layer - all Dear ImGui rendering.
 *
 * REQUIRES (copy manually into project folder):
 *   imgui.h/cpp, imgui_draw.cpp, imgui_tables.cpp, imgui_widgets.cpp,
 *   imgui_demo.cpp, imgui_internal.h, imconfig.h, imstb_*.h
 *   imgui_impl_glfw.h/cpp, imgui_impl_opengl3.h/cpp,
 *   imgui_impl_opengl3_loader.h, glfw3.h, glfw3dll.lib, glfw3.dll
 */
#include "../include/presentation.h"
#include "../include/logic.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "../imgui/glfw3.h"
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
using namespace std;

// ── Colours ────────────────────────────────────────────────────────────────
static const ImVec4 COL_ACCENT = ImVec4(0.30f, 0.65f, 1.00f, 1.0f);
static const ImVec4 COL_SUCCESS = ImVec4(0.20f, 0.80f, 0.40f, 1.0f);
static const ImVec4 COL_ERROR = ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
static const ImVec4 COL_MUTED = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
static const ImVec4 COL_WARNING = ImVec4(1.00f, 0.75f, 0.00f, 1.0f);
static const ImVec4 COL_PANEL = ImVec4(0.11f, 0.12f, 0.16f, 1.0f);
static const ImVec4 COL_DONE = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);

static const ImVec4 CAT_COLS[CAT_COUNT] = {
    ImVec4(0.96f,0.65f,0.14f,1.0f), // Food
    ImVec4(0.20f,0.72f,0.90f,1.0f), // Transport
    ImVec4(0.55f,0.36f,0.96f,1.0f), // Housing
    ImVec4(0.22f,0.84f,0.55f,1.0f), // Health
    ImVec4(0.94f,0.42f,0.64f,1.0f), // Entertainment
    ImVec4(0.97f,0.80f,0.20f,1.0f), // Shopping
    ImVec4(0.30f,0.78f,0.70f,1.0f), // Education
    ImVec4(0.70f,0.70f,0.70f,1.0f), // Other
};

// Persistent category name pointers - avoids dangling pointer from .c_str()
static const char* CAT_NAMES[CAT_COUNT] = {
    "Food","Transport","Housing","Health",
    "Entertainment","Shopping","Education","Other"
};

static const char* MONTHS[] = {
    "All","January","February","March","April","May","June",
    "July","August","September","October","November","December"
};

// ── Helpers ────────────────────────────────────────────────────────────────

static void setStatus(AppState& s, const char* msg, bool err = false)
{
    snprintf(s.statusMsg, sizeof(s.statusMsg), "%s", msg);
    s.statusError = err;
}

void addNotification(AppState& state, const string& msg)
{
    state.notifications.push_back({ msg, false });
}

// Fires budget notifications when 90% or 100% thresholds are crossed
void checkBudgetNotifications(AppState& state)
{
    if (state.loggedInUser.empty()) return;
    double used = getBudgetUsed(state.allExpenses, state.loggedInUser,
        state.budgetMonth, state.budgetYear);
    double limit = getBudget(state.budgets, state.loggedInUser,
        state.budgetMonth, state.budgetYear);
    if (limit <= 0.0) return;
    double pct = used / limit * 100.0;
    if (pct >= 90.0 && pct < 100.0 && !state.notif90Sent) {
        addNotification(state, "Warning: 90% of your monthly budget has been spent!");
        state.notif90Sent = true;
    }
    if (pct >= 100.0 && !state.notif100Sent) {
        addNotification(state, "Alert: Monthly budget exceeded!");
        state.notif100Sent = true;
    }
}

// Rebuilds the per-user expense list after any data change
void refreshExpenses(AppState& state)
{
    state.expenses = userExpenses(state.allExpenses, state.loggedInUser);
    checkBudgetNotifications(state);
}

static int unreadCount(const AppState& state)
{
    int n = 0;
    for (const auto& n2 : state.notifications) if (!n2.read) n++;
    return n;
}

// ────────────────────────────────────────────────────────────────────────────
//  renderLoginScreen
// ────────────────────────────────────────────────────────────────────────────
void renderLoginScreen(AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(380, 290), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_PANEL);
    ImGui::Begin("##login", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::SetCursorPosX((380 - ImGui::CalcTextSize("Expense Tracker").x) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, COL_ACCENT);
    ImGui::SetWindowFontScale(1.4f);
    ImGui::Text("Expense Tracker");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    ImGui::Text("Username:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##lu", state.loginUser, sizeof(state.loginUser));
    ImGui::Text("Password:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##lp", state.loginPass, sizeof(state.loginPass),
        ImGuiInputTextFlags_Password);
    ImGui::Spacing();

    if (state.loginError[0] != '\0')
        ImGui::TextColored(COL_ERROR, "%s", state.loginError);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.90f, 1.0f));
    if (ImGui::Button("Log In", ImVec2(-1, 34))) {
        if (loginUser(state.users, state.loginUser, state.loginPass)) {
            state.loggedInUser = state.loginUser;
            state.currentScreen = SCREEN_DASHBOARD;
            state.notif90Sent = false;
            state.notif100Sent = false;
            refreshExpenses(state);
            addNotification(state, string("Welcome back, ") + state.loggedInUser + "!");
            memset(state.loginUser, 0, sizeof(state.loginUser));
            memset(state.loginPass, 0, sizeof(state.loginPass));
            memset(state.loginError, 0, sizeof(state.loginError));
        }
        else {
            snprintf(state.loginError, sizeof(state.loginError),
                "Invalid username or password.");
        }
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Don't have an account?");
    ImGui::SameLine();
    if (ImGui::SmallButton("Register here")) {
        state.currentScreen = SCREEN_REGISTER;
        memset(state.loginError, 0, sizeof(state.loginError));
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

// ────────────────────────────────────────────────────────────────────────────
//  renderRegisterScreen
// ────────────────────────────────────────────────────────────────────────────
void renderRegisterScreen(AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 370), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_PANEL);
    ImGui::Begin("##register", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::PushStyleColor(ImGuiCol_Text, COL_ACCENT);
    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("Create Account");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    ImGui::Text("Username (3-20 chars, letters/numbers/_):");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##ru", state.regUser, sizeof(state.regUser));
    ImGui::Text("Email:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##re", state.regEmail, sizeof(state.regEmail));
    ImGui::Text("Password (min 6 characters):");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##rp", state.regPass, sizeof(state.regPass),
        ImGuiInputTextFlags_Password);
    ImGui::Text("Confirm Password:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##rp2", state.regPass2, sizeof(state.regPass2),
        ImGuiInputTextFlags_Password);

    ImGui::Spacing();
    if (state.regError[0] != '\0') ImGui::TextColored(COL_ERROR, "%s", state.regError);
    if (state.regSuccess[0] != '\0') ImGui::TextColored(COL_SUCCESS, "%s", state.regSuccess);
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.15f, 1.0f));
    if (ImGui::Button("Create Account", ImVec2(-1, 34))) {
        memset(state.regError, 0, sizeof(state.regError));
        memset(state.regSuccess, 0, sizeof(state.regSuccess));
        if (string(state.regPass) != string(state.regPass2)) {
            snprintf(state.regError, sizeof(state.regError), "Passwords do not match.");
        }
        else if (usernameExists(state.users, state.regUser)) {
            snprintf(state.regError, sizeof(state.regError), "Username already taken.");
        }
        else if (registerUser(state.users, state.regUser, state.regPass, state.regEmail)) {
            snprintf(state.regSuccess, sizeof(state.regSuccess),
                "Account created! You can now log in.");
            memset(state.regUser, 0, sizeof(state.regUser));
            memset(state.regPass, 0, sizeof(state.regPass));
            memset(state.regPass2, 0, sizeof(state.regPass2));
            memset(state.regEmail, 0, sizeof(state.regEmail));
        }
        else {
            snprintf(state.regError, sizeof(state.regError),
                "Registration failed. Check all fields.");
        }
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Already have an account?");
    ImGui::SameLine();
    if (ImGui::SmallButton("Log in here")) {
        state.currentScreen = SCREEN_LOGIN;
        memset(state.regError, 0, sizeof(state.regError));
        memset(state.regSuccess, 0, sizeof(state.regSuccess));
    }
    ImGui::End();
    ImGui::PopStyleColor();
}