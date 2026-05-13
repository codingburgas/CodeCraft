#pragma once
#include "logic.h"
#include <vector>
#include <string>
using namespace std;

enum Screen { SCREEN_LOGIN = 0, SCREEN_REGISTER, SCREEN_DASHBOARD };

struct AppState {
    vector<Expense>      allExpenses;
    vector<User>         users;
    vector<Budget>       budgets;
    vector<Notification> notifications;

    Screen currentScreen = SCREEN_LOGIN;
    string loggedInUser = "";
    vector<Expense> expenses;        

    char loginUser[64] = {};
    char loginPass[64] = {};
    char loginError[256] = {};

    char regUser[64] = {};
    char regPass[64] = {};
    char regPass2[64] = {};
    char regEmail[128] = {};
    char regError[256] = {};
    char regSuccess[256] = {};

    char  formDesc[256] = {};
    float formAmount = 0.0f;
    int   formCategory = 0;
    int   formDay = 1;
    int   formMonth = 1;
    int   formYear = 2024;

    int   sortMode = 0;
    int   filterMonth = 0;
    int   filterYear = 2024;
    int   filterCat = -1;
    int   statsYear = 2024;
    float filterMinAmt = 0.0f;
    float filterMaxAmt = 0.0f;
    bool  useAmountFilter = false;

    char        searchKeyword[256] = {};
    float       searchAmount = 0.0f;
    vector<int> linearResults;
    bool        linearDone = false;
    int         binaryResult = -1;
    bool        binaryDone = false;

    int  editIdx = -1;
    int  deleteIdx = -1;

    float budgetLimit = 0.0f;
    int   budgetMonth = 1;
    int   budgetYear = 2024;

    char cpOldPass[64] = {};
    char cpNewPass[64] = {};
    char cpNewPass2[64] = {};
    char cpMsg[256] = {};
    bool cpError = false;
    bool showSettings = false;
    bool showNotifCenter = false;
    bool switchToEdit = false;

    char statusMsg[512] = {};
    bool statusError = false;

    bool notif90Sent = false;
    bool notif100Sent = false;
};

void mainMenu();
void renderApp(AppState& state);
void renderLoginScreen(AppState& state);
void renderRegisterScreen(AppState& state);
void renderDashboard(AppState& state);
void renderHeader(AppState& state);
void renderExpenseTable(AppState& state);
void renderFormPanel(AppState& state);
void renderSearchPanel(AppState& state);
void renderStatsPanel(AppState& state);
void renderBudgetPanel(AppState& state);
void renderSettingsWindow(AppState& state);
void renderNotifCenter(AppState& state);
void renderStatusBar(const AppState& state);
void renderDeleteModal(AppState& state);
void addNotification(AppState& state, const string& msg);
void checkBudgetNotifications(AppState& state);
void refreshExpenses(AppState& state);