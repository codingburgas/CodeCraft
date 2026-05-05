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

// ────────────────────────────────────────────────────────────────────────────
//  renderFormPanel  (shared by Add and Edit tabs)
// ────────────────────────────────────────────────────────────────────────────
void renderFormPanel(AppState& state)
{
    bool editing = (state.editIdx >= 0);
    ImGui::TextColored(COL_ACCENT, editing ? "Edit Expense" : "Add Expense");
    ImGui::Separator();

    ImGui::Text("Description:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##desc", state.formDesc, sizeof(state.formDesc));

    ImGui::Text("Amount ($):");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputFloat("##amt", &state.formAmount, 0.01f, 1.0f, "%.2f");

    ImGui::Text("Category:");
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##cat", &state.formCategory, CAT_NAMES, CAT_COUNT);

    // FIX: wider date fields so numbers are fully visible
    ImGui::Text("Date:");
    ImGui::SetNextItemWidth(76);
    ImGui::InputInt("Day##d", &state.formDay, 1, 0);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(76);
    ImGui::InputInt("Mo##m", &state.formMonth, 1, 0);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(88);
    ImGui::InputInt("Year##y", &state.formYear, 1, 10);

    ImGui::Spacing(); ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Button,
        editing ? ImVec4(0.15f, 0.55f, 0.15f, 1.0f)
        : ImVec4(0.15f, 0.45f, 0.90f, 1.0f));
    if (ImGui::Button(editing ? "Save Changes##s" : "Add Expense##s", ImVec2(-1, 34))) {
        string   desc(state.formDesc);
        double   amt = (double)state.formAmount;
        Category cat = (Category)state.formCategory;
        bool     ok = false;

        if (editing) {
            int targetId = state.expenses[state.editIdx].id;
            int globalIdx = -1;
            for (int k = 0; k < (int)state.allExpenses.size(); k++)
                if (state.allExpenses[k].id == targetId) { globalIdx = k; break; }
            if (globalIdx >= 0) {
                ok = editExpense(state.allExpenses, globalIdx, desc, amt, cat,
                    state.formDay, state.formMonth, state.formYear);
                if (ok) refreshExpenses(state);
            }
            setStatus(state, ok ? "Expense updated." : "Error - check your input.", !ok);
            if (ok) { state.editIdx = -1; state.switchToEdit = false; }
        }
        else {
            // FIX: addExpense call with all required parameters
            ok = addExpense(state.allExpenses, state.loggedInUser,
                desc, amt, cat,
                state.formDay, state.formMonth, state.formYear);
            if (ok) {
                refreshExpenses(state);
                checkBudgetNotifications(state);
            }
            setStatus(state, ok ? "Expense added." : "Error - check your input.", !ok);
        }

        if (ok) {
            // Reset form fields after successful save
            memset(state.formDesc, 0, sizeof(state.formDesc));
            state.formAmount = 0.0f;
            state.formCategory = 0;
            state.formDay = 1;
            state.formMonth = 1;
            state.formYear = 2024;
        }
    }
    ImGui::PopStyleColor();

    if (editing) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        if (ImGui::Button("Cancel##c", ImVec2(-1, 28))) {
            state.editIdx = -1;
            state.switchToEdit = false;
        }
        ImGui::PopStyleColor();
    }
}

// ────────────────────────────────────────────────────────────────────────────
//  renderSearchPanel
// ────────────────────────────────────────────────────────────────────────────
void renderSearchPanel(AppState& state)
{
    ImGui::TextColored(COL_ACCENT, "Linear Search by Description");
    ImGui::Separator();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90);
    ImGui::InputText("##kw", state.searchKeyword, sizeof(state.searchKeyword));
    ImGui::SameLine();
    if (ImGui::Button("Search##l", ImVec2(-1, 0))) {
        state.linearResults = linearSearch(state.expenses, state.searchKeyword);
        state.linearDone = true;
    }
    if (state.linearDone) {
        if (state.linearResults.empty()) {
            ImGui::TextColored(COL_ERROR, "No results found.");
        }
        else {
            ImGui::TextColored(COL_SUCCESS, "Found: %d record(s)", (int)state.linearResults.size());
            for (int idx : state.linearResults) {
                const Expense& e = state.expenses[idx];
                ImGui::BulletText("%s  $%.2f  (%s)  %02d/%02d/%d",
                    e.description.c_str(), e.amount,
                    CAT_NAMES[e.category], e.month, e.day, e.year);
            }
        }
    }

    ImGui::Spacing(); ImGui::Separator();
    ImGui::TextColored(COL_ACCENT, "Binary Search by Exact Amount");
    ImGui::Separator();

    ImGui::SetNextItemWidth(140);
    ImGui::InputFloat("$##bs", &state.searchAmount, 0.01f, 1.0f, "%.2f");
    ImGui::SameLine();
    if (ImGui::Button("Search##b", ImVec2(-1, 0))) {
        vector<Expense> sorted = state.expenses;
        bubbleSortByAmount(sorted);
        int found = binarySearch(sorted, (double)state.searchAmount);
        state.binaryResult = -1;
        if (found >= 0)
            for (int k = 0; k < (int)state.expenses.size(); k++)
                if (state.expenses[k].id == sorted[found].id)
                {
                    state.binaryResult = k; break;
                }
        state.binaryDone = true;
    }
    if (state.binaryDone) {
        if (state.binaryResult < 0)
            ImGui::TextColored(COL_ERROR,
                "No expense with amount $%.2f.", state.searchAmount);
        else {
            const Expense& e = state.expenses[state.binaryResult];
            ImGui::TextColored(COL_SUCCESS,
                "Found: %s  $%.2f  (%s)  %02d/%02d/%d",
                e.description.c_str(), e.amount,
                CAT_NAMES[e.category], e.month, e.day, e.year);
        }
    }
}

// ────────────────────────────────────────────────────────────────────────────
//  renderStatsPanel
// ────────────────────────────────────────────────────────────────────────────
void renderStatsPanel(AppState& state)
{
    ImGui::TextColored(COL_ACCENT, "Statistics");
    ImGui::Separator();

    if (state.expenses.empty()) {
        ImGui::TextColored(COL_MUTED, "No expenses to analyse.");
        return;
    }

    double total = totalExpenses(state.expenses);
    double avg = avgExpense(state.expenses);
    double highest = maxExpense(state.expenses);
    double lowest = minExpense(state.expenses);

    ImGui::Text("Total:   "); ImGui::SameLine(); ImGui::TextColored(COL_ACCENT, "$%.2f", total);
    ImGui::Text("Average: "); ImGui::SameLine(); ImGui::TextColored(COL_ACCENT, "$%.2f", avg);
    ImGui::Text("Highest: "); ImGui::SameLine(); ImGui::TextColored(COL_ERROR, "$%.2f", highest);
    ImGui::Text("Lowest:  "); ImGui::SameLine(); ImGui::TextColored(COL_SUCCESS, "$%.2f", lowest);
    ImGui::Text("Count:   "); ImGui::SameLine(); ImGui::TextColored(COL_ACCENT, "%d records", (int)state.expenses.size());

    ImGui::Spacing();
    ImGui::Text("By category:");
    ImGui::Separator();

    double catTot[CAT_COUNT] = {};
    categoryTotals(state.expenses, catTot);
    for (int i = 0; i < CAT_COUNT; i++) {
        if (catTot[i] < 0.001) continue;
        float frac = (float)(catTot[i] / total);
        int   cnt = recursiveCountByCategory(
            state.expenses, (int)state.expenses.size() - 1, (Category)i);
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "$%.2f (%d)", catTot[i], cnt);
        ImGui::TextColored(CAT_COLS[i], "%-14s", CAT_NAMES[i]);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, CAT_COLS[i]);
        ImGui::ProgressBar(frac, ImVec2(-1, 13), lbl);
        ImGui::PopStyleColor();
    }

    // FIX: Monthly breakdown uses statsYear (separate from table filterYear)
    ImGui::Spacing(); ImGui::Separator();
    ImGui::Text("Monthly breakdown:");

    // FIX: wider input field so year value is fully visible
    ImGui::SetNextItemWidth(90);
    ImGui::InputInt("Year##sy", &state.statsYear, 1, 10);
    ImGui::Separator();

    double monthTotals[13] = {};
    for (int m = 1; m <= 12; m++)
        monthTotals[m] = recursiveMonthlyTotal(
            state.expenses, (int)state.expenses.size() - 1, m, state.statsYear);

    double maxMonth = 0.0;
    for (int m = 1; m <= 12; m++)
        if (monthTotals[m] > maxMonth) maxMonth = monthTotals[m];

    bool anyMonth = false;
    for (int m = 1; m <= 12; m++) {
        if (monthTotals[m] < 0.001) continue;
        anyMonth = true;
        float frac = (maxMonth > 0.0) ? (float)(monthTotals[m] / maxMonth) : 0.0f;
        char lbl[32];
        snprintf(lbl, sizeof(lbl), "$%.2f", monthTotals[m]);
        // FIX: month name padded to consistent width so bar always starts same position
        ImGui::TextColored(COL_ACCENT, "%-11s", MONTHS[m]);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COL_ACCENT);
        ImGui::ProgressBar(frac, ImVec2(-1, 13), lbl);
        ImGui::PopStyleColor();
    }
    if (!anyMonth)
        ImGui::TextColored(COL_MUTED, "No expenses in %d.", state.statsYear);
}

// ────────────────────────────────────────────────────────────────────────────
//  renderBudgetPanel
// ────────────────────────────────────────────────────────────────────────────
void renderBudgetPanel(AppState& state)
{
    ImGui::TextColored(COL_ACCENT, "Monthly Budget");
    ImGui::Separator();

    // Budget note: only COMPLETED (Done) expenses count toward budget usage
    ImGui::TextColored(COL_MUTED,
        "Note: only expenses marked as Done count toward budget.");
    ImGui::Spacing();

    ImGui::Text("Select month:");
    int bm = state.budgetMonth - 1;
    ImGui::SetNextItemWidth(115);
    ImGui::Combo("##bm", &bm, MONTHS + 1, 12);
    state.budgetMonth = bm + 1;
    ImGui::SameLine();
    // FIX: wider year input
    ImGui::SetNextItemWidth(88);
    ImGui::InputInt("##by", &state.budgetYear, 1, 10);

    double used = getBudgetUsed(state.allExpenses, state.loggedInUser,
        state.budgetMonth, state.budgetYear);
    double limit = getBudget(state.budgets, state.loggedInUser,
        state.budgetMonth, state.budgetYear);

    ImGui::Spacing();

    if (limit > 0.0) {
        float frac = (float)(used / limit);
        if (frac > 1.0f) frac = 1.0f;

        ImGui::Text("Spent (Done): $%.2f  /  Limit: $%.2f  (%.1f%%)",
            used, limit, used / limit * 100.0);

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
            used >= limit ? COL_ERROR :
            used >= limit * 0.9 ? COL_WARNING : COL_SUCCESS);
        ImGui::ProgressBar(frac, ImVec2(-1, 22));
        ImGui::PopStyleColor();

        if (used >= limit)
            ImGui::TextColored(COL_ERROR,
                "!! Budget exceeded by $%.2f !!", used - limit);
        else if (used >= limit * 0.9)
            ImGui::TextColored(COL_WARNING,
                "! %.1f%% used - $%.2f remaining", used / limit * 100.0, limit - used);
        else
            ImGui::TextColored(COL_SUCCESS,
                "%.1f%% used - $%.2f remaining", used / limit * 100.0, limit - used);
    }
    else {
        ImGui::TextColored(COL_MUTED, "No budget set for %s %d.",
            MONTHS[state.budgetMonth], state.budgetYear);
    }

    ImGui::Spacing(); ImGui::Separator();
    ImGui::Text("Set new limit ($):");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputFloat("##bl", &state.budgetLimit, 1.0f, 10.0f, "%.2f");

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.90f, 1.0f));
    if (ImGui::Button("Set Budget", ImVec2(-1, 30))) {
        bool ok = setBudget(state.budgets, state.loggedInUser,
            state.budgetMonth, state.budgetYear,
            (double)state.budgetLimit);
        if (ok) {
            state.notif90Sent = false;
            state.notif100Sent = false;
            checkBudgetNotifications(state);
        }
        setStatus(state, ok ? "Budget saved." : "Error - limit must be > 0.", !ok);
    }
    ImGui::PopStyleColor();
}
// ────────────────────────────────────────────────────────────────────────────
//  renderNotifCenter
// ────────────────────────────────────────────────────────────────────────────
void renderNotifCenter(AppState& state)
{
    if (!state.showNotifCenter) return;

    ImGui::SetNextWindowSize(ImVec2(390, 300), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_PANEL);
    ImGui::Begin("Notification Center", &state.showNotifCenter,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
    if (ImGui::SmallButton("Mark all read"))
        for (auto& n : state.notifications) n.read = true;
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear all"))
        state.notifications.clear();
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (state.notifications.empty()) {
        ImGui::TextColored(COL_MUTED, "No notifications.");
    }
    else {
        for (int i = (int)state.notifications.size() - 1; i >= 0; i--) {
            auto& n = state.notifications[i];
            ImGui::PushID(i);
            ImGui::TextColored(n.read ? COL_MUTED : COL_WARNING,
                n.read ? "  %s" : "* %s", n.message.c_str());
            if (!n.read) {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 36);
                if (ImGui::SmallButton("Read")) n.read = true;
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

// ────────────────────────────────────────────────────────────────────────────
//  renderSettingsWindow
// ────────────────────────────────────────────────────────────────────────────
void renderSettingsWindow(AppState& state)
{
    if (!state.showSettings) return;

    ImGui::SetNextWindowSize(ImVec2(360, 280), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_PANEL);
    ImGui::Begin("Settings", &state.showSettings,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(COL_ACCENT, "Account: %s", state.loggedInUser.c_str());
    ImGui::Spacing(); ImGui::Separator();
    ImGui::TextColored(COL_ACCENT, "Change Password");
    ImGui::Separator();

    ImGui::Text("Current password:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##cp1", state.cpOldPass, sizeof(state.cpOldPass),
        ImGuiInputTextFlags_Password);
    ImGui::Text("New password (min 6 chars):");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##cp2", state.cpNewPass, sizeof(state.cpNewPass),
        ImGuiInputTextFlags_Password);
    ImGui::Text("Confirm new password:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##cp3", state.cpNewPass2, sizeof(state.cpNewPass2),
        ImGuiInputTextFlags_Password);

    ImGui::Spacing();
    if (state.cpMsg[0] != '\0')
        ImGui::TextColored(state.cpError ? COL_ERROR : COL_SUCCESS,
            "%s", state.cpMsg);
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.90f, 1.0f));
    if (ImGui::Button("Change Password", ImVec2(-1, 30))) {
        memset(state.cpMsg, 0, sizeof(state.cpMsg));
        if (string(state.cpNewPass) != string(state.cpNewPass2)) {
            snprintf(state.cpMsg, sizeof(state.cpMsg), "Passwords do not match.");
            state.cpError = true;
        }
        else if (changePassword(state.users, state.loggedInUser,
            state.cpOldPass, state.cpNewPass)) {
            snprintf(state.cpMsg, sizeof(state.cpMsg), "Password changed successfully.");
            state.cpError = false;
            memset(state.cpOldPass, 0, sizeof(state.cpOldPass));
            memset(state.cpNewPass, 0, sizeof(state.cpNewPass));
            memset(state.cpNewPass2, 0, sizeof(state.cpNewPass2));
            addNotification(state, "Your password was changed successfully.");
        }
        else {
            snprintf(state.cpMsg, sizeof(state.cpMsg),
                "Error - check current password.");
            state.cpError = true;
        }
    }
    ImGui::PopStyleColor();
    ImGui::End();
    ImGui::PopStyleColor();
}

// ────────────────────────────────────────────────────────────────────────────
//  renderStatusBar
// ────────────────────────────────────────────────────────────────────────────
void renderStatusBar(const AppState& state)
{
    if (state.statusMsg[0] == '\0') return;
    ImGui::Separator();
    ImGui::TextColored(state.statusError ? COL_ERROR : COL_SUCCESS,
        "%s", state.statusMsg);
}

// ────────────────────────────────────────────────────────────────────────────
//  renderDashboard
// ────────────────────────────────────────────────────────────────────────────
void renderDashboard(AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##dash", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar(2);

    renderHeader(state);

    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("##left", ImVec2(avail.x * 0.58f, avail.y - 26), false);
    renderExpenseTable(state);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##right", ImVec2(0, avail.y - 26), false);
    if (ImGui::BeginTabBar("##tabs")) {

        // FIX: Add and Edit are separate tabs.
        // When Edit is clicked switchToEdit=true which sets SetSelected on
        // the Edit tab so it activates automatically on the next frame.
        ImGuiTabItemFlags editFlags = 0;
        if (state.switchToEdit && state.editIdx >= 0) {
            editFlags = ImGuiTabItemFlags_SetSelected;
            state.switchToEdit = false;  // consume flag - switch only once
        }

        // Add tab - always available, shows empty form
        if (ImGui::BeginTabItem(" Add ")) {
            // Reset editIdx if user manually clicks Add tab
            if (state.editIdx >= 0) {
                state.editIdx = -1;
                state.switchToEdit = false;
                memset(state.formDesc, 0, sizeof(state.formDesc));
                state.formAmount = 0.0f;
                state.formCategory = 0;
                state.formDay = 1;
                state.formMonth = 1;
                state.formYear = 2024;
            }
            renderFormPanel(state);
            ImGui::EndTabItem();
        }

        // Edit tab - activated automatically when Edit button is pressed
        if (ImGui::BeginTabItem(" Edit ", nullptr, editFlags)) {
            if (state.editIdx >= 0) {
                renderFormPanel(state);
            }
            else {
                ImGui::Spacing();
                ImGui::TextColored(COL_MUTED,
                    "Click Edit on an expense in the table to edit it.");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(" Search ")) {
            renderSearchPanel(state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(" Statistics ")) {
            renderStatsPanel(state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(" Budget ")) {
            renderBudgetPanel(state);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    renderStatusBar(state);
    ImGui::End();

    renderSettingsWindow(state);
    renderNotifCenter(state);
}

// ────────────────────────────────────────────────────────────────────────────
//  renderApp
// ────────────────────────────────────────────────────────────────────────────
void renderApp(AppState& state)
{
    switch (state.currentScreen) {
    case SCREEN_LOGIN:     renderLoginScreen(state);    break;
    case SCREEN_REGISTER:  renderRegisterScreen(state); break;
    case SCREEN_DASHBOARD: renderDashboard(state);      break;
    }
}

// ────────────────────────────────────────────────────────────────────────────
//  mainMenu
// ────────────────────────────────────────────────────────────────────────────
void mainMenu()
{
    if (!glfwInit()) return;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720,
        "Expense Tracker", nullptr, nullptr);
    if (!window) { glfwTerminate(); return; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 5);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    AppState state;
    state.allExpenses = loadAllExpenses();
    state.users = loadAllUsers();
    state.budgets = loadAllBudgets();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderApp(state);

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}