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
// ────────────────────────────────────────────────────────────────────────────
//  renderHeader
// ────────────────────────────────────────────────────────────────────────────
void renderHeader(AppState& state)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.15f, 1.0f));
    ImGui::BeginChild("##hdr", ImVec2(0, 52), false);

    ImGui::SetCursorPos(ImVec2(10, 8));
    ImGui::PushStyleColor(ImGuiCol_Text, COL_ACCENT);
    ImGui::SetWindowFontScale(1.25f);
    ImGui::Text("Expense Tracker");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Budget warning - uses completed expenses only
    double used = getBudgetUsed(state.allExpenses, state.loggedInUser,
        state.budgetMonth, state.budgetYear);
    double limit = getBudget(state.budgets, state.loggedInUser,
        state.budgetMonth, state.budgetYear);
    if (limit > 0.0 && used >= limit * 0.9) {
        ImGui::SameLine(190);
        ImGui::SetCursorPosY(14);
        ImGui::TextColored(used >= limit ? COL_ERROR : COL_WARNING,
            used >= limit ? "!! Budget exceeded !!" : "! Approaching budget limit");
    }

    double total = totalExpenses(state.expenses);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 360);
    ImGui::SetCursorPosY(10);
    ImGui::TextColored(COL_MUTED, "Total:"); ImGui::SameLine();
    ImGui::TextColored(COL_ACCENT, "$%.2f", total);
    ImGui::SameLine(0, 16);
    ImGui::TextColored(COL_MUTED, "User:"); ImGui::SameLine();
    ImGui::TextColored(COL_SUCCESS, "%s", state.loggedInUser.c_str());
    ImGui::SameLine(0, 12);

    int unread = unreadCount(state);
    char bellLbl[32];
    snprintf(bellLbl, sizeof(bellLbl),
        unread > 0 ? "Bell(%d)##nb" : "Bell##nb", unread);
    ImGui::PushStyleColor(ImGuiCol_Button,
        unread > 0 ? ImVec4(0.8f, 0.5f, 0.1f, 1.0f) : ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
    if (ImGui::SmallButton(bellLbl)) state.showNotifCenter = !state.showNotifCenter;
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
    if (ImGui::SmallButton("Settings")) state.showSettings = !state.showSettings;
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    if (ImGui::SmallButton("Logout")) {
        state.loggedInUser = "";
        state.currentScreen = SCREEN_LOGIN;
        state.expenses.clear();
        state.notifications.clear();
        state.editIdx = -1;
        state.deleteIdx = -1;
        state.switchToEdit = false;
        state.notif90Sent = false;
        state.notif100Sent = false;
        memset(state.statusMsg, 0, sizeof(state.statusMsg));
    }
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ────────────────────────────────────────────────────────────────────────────
//  renderDeleteModal
// ────────────────────────────────────────────────────────────────────────────
void renderDeleteModal(AppState& state)
{
    if (!ImGui::BeginPopupModal("Delete?", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize)) return;

    ImGui::Text("Are you sure you want to delete this record?");
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Yes, delete", ImVec2(120, 0))) {
        if (state.deleteIdx >= 0 && state.deleteIdx < (int)state.expenses.size()) {
            int targetId = state.expenses[state.deleteIdx].id;
            int globalIdx = -1;
            for (int k = 0; k < (int)state.allExpenses.size(); k++)
                if (state.allExpenses[k].id == targetId) { globalIdx = k; break; }
            if (globalIdx >= 0) {
                removeExpense(state.allExpenses, globalIdx);
                refreshExpenses(state);
                setStatus(state, "Expense deleted.");
            }
            state.deleteIdx = -1;
        }
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        state.deleteIdx = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// ────────────────────────────────────────────────────────────────────────────
//  renderExpenseTable
// ────────────────────────────────────────────────────────────────────────────
void renderExpenseTable(AppState& state)
{
    ImGui::Text("Sort:"); ImGui::SameLine();
    ImGui::RadioButton("Amount", &state.sortMode, 0); ImGui::SameLine();
    ImGui::RadioButton("Date", &state.sortMode, 1); ImGui::SameLine();
    ImGui::RadioButton("Category", &state.sortMode, 2);

    ImGui::SameLine(0, 16); ImGui::Text("Month:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("##fm", &state.filterMonth, MONTHS, 13);
    if (state.filterMonth > 0) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(72);
        ImGui::InputInt("##fy", &state.filterYear, 1, 10);
    }

    // Category filter using persistent CAT_NAMES
    ImGui::SameLine(0, 16); ImGui::Text("Cat:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(110);
    const char* catCombo[CAT_COUNT + 1];
    catCombo[0] = "All";
    for (int i = 0; i < CAT_COUNT; i++) catCombo[i + 1] = CAT_NAMES[i];
    int catFilter = state.filterCat + 1;
    ImGui::Combo("##fc", &catFilter, catCombo, CAT_COUNT + 1);
    state.filterCat = catFilter - 1;

    ImGui::SameLine(0, 16);
    ImGui::Checkbox("Amt range", &state.useAmountFilter);
    if (state.useAmountFilter) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(65);
        ImGui::InputFloat("##fmn", &state.filterMinAmt, 0, 0, "%.0f");
        ImGui::SameLine(); ImGui::Text("-"); ImGui::SameLine();
        ImGui::SetNextItemWidth(65);
        ImGui::InputFloat("##fmx", &state.filterMaxAmt, 0, 0, "%.0f");
    }

    vector<Expense> disp = state.expenses;
    if (state.filterMonth > 0)
        disp = filterByMonth(disp, state.filterMonth, state.filterYear);
    if (state.filterCat >= 0)
        disp = filterByCategory(disp, (Category)state.filterCat);
    if (state.useAmountFilter && state.filterMaxAmt > state.filterMinAmt)
        disp = filterByRange(disp, state.filterMinAmt, state.filterMaxAmt);

    if (state.sortMode == 0) bubbleSortByAmount(disp);
    else if (state.sortMode == 1 && !disp.empty())
        quickSortByDate(disp, 0, (int)disp.size() - 1);
    else if (state.sortMode == 2) bubbleSortByCategory(disp);

    ImGui::Separator();
    ImGui::TextColored(COL_MUTED, "%d record(s)", (int)disp.size());
    ImGui::Separator();

    const ImGuiTableFlags tflags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

    float tableH = ImGui::GetContentRegionAvail().y - 28.0f;
    if (ImGui::BeginTable("##tbl", 8, tflags, ImVec2(0, tableH))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 34);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Amount($)", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 112);
        ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 90);
        ImGui::TableSetupColumn("Done", ImGuiTableColumnFlags_WidthFixed, 44);
        ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthFixed, 38);
        ImGui::TableSetupColumn("Del", ImGuiTableColumnFlags_WidthFixed, 34);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)disp.size(); i++) {
            const Expense& e = disp[i];
            int userIdx = -1;
            for (int k = 0; k < (int)state.expenses.size(); k++)
                if (state.expenses[k].id == e.id) { userIdx = k; break; }

            ImGui::TableNextRow();
            ImVec4 textCol = e.completed
                ? COL_DONE : ImGui::GetStyleColorVec4(ImGuiCol_Text);

            ImGui::TableSetColumnIndex(0); ImGui::TextColored(textCol, "%d", e.id);
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(textCol, "%s", e.description.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::TextColored(textCol, "%.2f", e.amount);
            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(e.completed ? COL_DONE : CAT_COLS[e.category],
                "%s", CAT_NAMES[e.category]);
            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(textCol, "%02d/%02d/%04d", e.month, e.day, e.year);

            // Done toggle
            ImGui::TableSetColumnIndex(5);
            ImGui::PushID(e.id * 10 + 3);
            ImGui::PushStyleColor(ImGuiCol_Button,
                e.completed ? ImVec4(0.2f, 0.6f, 0.2f, 0.85f)
                : ImVec4(0.3f, 0.3f, 0.38f, 0.85f));
            if (ImGui::SmallButton(e.completed ? "Undo" : "Done") && userIdx >= 0) {
                int tid = state.expenses[userIdx].id;
                for (int k = 0; k < (int)state.allExpenses.size(); k++) {
                    if (state.allExpenses[k].id == tid) {
                        toggleCompleted(state.allExpenses, k);
                        refreshExpenses(state);
                        break;
                    }
                }
            }
            ImGui::PopStyleColor(); ImGui::PopID();

            // Edit
            ImGui::TableSetColumnIndex(6);
            ImGui::PushID(e.id * 10 + 1);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 0.85f));
            if (ImGui::SmallButton("Edit") && userIdx >= 0) {
                state.editIdx = userIdx;
                state.switchToEdit = true;
                snprintf(state.formDesc, sizeof(state.formDesc), "%s", e.description.c_str());
                state.formAmount = (float)e.amount;
                state.formCategory = (int)e.category;
                state.formDay = e.day;
                state.formMonth = e.month;
                state.formYear = e.year;
            }
            ImGui::PopStyleColor(); ImGui::PopID();

            // Delete
            ImGui::TableSetColumnIndex(7);
            ImGui::PushID(e.id * 10 + 2);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.85f));
            if (ImGui::SmallButton("Del") && userIdx >= 0) {
                state.deleteIdx = userIdx;
                ImGui::OpenPopup("Delete?");
            }
            ImGui::PopStyleColor(); ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // FIX: renderDeleteModal must be OUTSIDE the table loop.
    // Calling it inside the loop means each row creates a different popup
    // context - ImGui cannot find the popup and Del never works.
    renderDeleteModal(state);
}