/*
 * data.cpp
 * Data layer - file I/O for expenses, users and budgets.
 */
#include "../include/data.h"
#include <fstream>
#include <sstream>
using namespace std;

string categoryName(Category cat)
{
    // Array indexed by the Category enum value
    static const char* names[] = {
        "Food","Transport","Housing","Health",
        "Entertainment","Shopping","Education","Other"
    };
    return (cat >= 0 && cat < CAT_COUNT) ? names[cat] : "Unknown";
}

// ── Expense serialisation ──────────────────────────────────────────────────

// Converts one Expense to a pipe-separated text line
static string expenseToLine(const Expense& e)
{
    ostringstream s;
    s << e.id << "|" << e.username << "|" << e.description << "|"
        << e.amount << "|" << (int)e.category << "|"
        << e.day << "|" << e.month << "|" << e.year << "|"
        << (int)e.active << "|" << (int)e.completed;
    return s.str();
}

// Parses one text line back into an Expense
static bool lineToExpense(const string& line, Expense& out)
{
    istringstream s(line);
    string t;
    try {
        getline(s, t, '|'); out.id = stoi(t);
        getline(s, t, '|'); out.username = t;
        getline(s, t, '|'); out.description = t;
        getline(s, t, '|'); out.amount = stod(t);
        getline(s, t, '|'); out.category = (Category)stoi(t);
        getline(s, t, '|'); out.day = stoi(t);
        getline(s, t, '|'); out.month = stoi(t);
        getline(s, t, '|'); out.year = stoi(t);
        getline(s, t, '|'); out.active = stoi(t);
        // completed field - default false for old records without this field
        if (getline(s, t, '|')) out.completed = stoi(t);
        else                   out.completed = false;
    }
    catch (...) { return false; }
    return true;
}

int loadExpenses(vector<Expense>& dest)
{
    dest.clear();
    ifstream f(DATA_FILE);
    string line;
    while (getline(f, line)) {
        Expense e;
        if (lineToExpense(line, e) && e.active) dest.push_back(e);
    }
    return (int)dest.size();
}

bool saveExpenses(const vector<Expense>& src)
{
    ofstream f(DATA_FILE, ios::trunc);
    for (const auto& e : src) f << expenseToLine(e) << "\n";
    return f.good();
}

bool appendExpense(const Expense& e)
{
    ofstream f(DATA_FILE, ios::app);
    f << expenseToLine(e) << "\n";
    return f.good();
}

bool updateExpense(const vector<Expense>& src) { return saveExpenses(src); }

bool deleteExpense(vector<Expense>& src, int idx)
{
    if (idx < 0 || idx >= (int)src.size()) return false;
    src[idx].active = false;
    return saveExpenses(src);
}

int nextExpenseId(const vector<Expense>& src)
{
    int maxId = 0;
    for (const auto& e : src) if (e.id > maxId) maxId = e.id;
    return maxId + 1;
}

// ── User serialisation ─────────────────────────────────────────────────────

static string userToLine(const User& u)
{
    ostringstream s;
    s << u.username << "|" << u.passwordHash << "|" << u.email << "|" << (int)u.active;
    return s.str();
}

static bool lineToUser(const string& line, User& out)
{
    istringstream s(line);
    string t;
    try {
        getline(s, t, '|'); out.username = t;
        getline(s, t, '|'); out.passwordHash = t;
        getline(s, t, '|'); out.email = t;
        getline(s, t, '|'); out.active = stoi(t);
    }
    catch (...) { return false; }
    return true;
}

int loadUsers(vector<User>& dest)
{
    dest.clear();
    ifstream f(USERS_FILE);
    string line;
    while (getline(f, line)) { User u; if (lineToUser(line, u)) dest.push_back(u); }
    return (int)dest.size();
}

bool saveUsers(const vector<User>& src)
{
    ofstream f(USERS_FILE, ios::trunc);
    for (const auto& u : src) f << userToLine(u) << "\n";
    return f.good();
}

// ── Budget serialisation ───────────────────────────────────────────────────

static string budgetToLine(const Budget& b)
{
    ostringstream s;
    s << b.username << "|" << b.month << "|" << b.year << "|" << b.limit;
    return s.str();
}

static bool lineToBudget(const string& line, Budget& out)
{
    istringstream s(line);
    string t;
    try {
        getline(s, t, '|'); out.username = t;
        getline(s, t, '|'); out.month = stoi(t);
        getline(s, t, '|'); out.year = stoi(t);
        getline(s, t, '|'); out.limit = stod(t);
    }
    catch (...) { return false; }
    return true;
}

int loadBudgets(vector<Budget>& dest)
{
    dest.clear();
    ifstream f(BUDGET_FILE);
    string line;
    while (getline(f, line)) { Budget b; if (lineToBudget(line, b)) dest.push_back(b); }
    return (int)dest.size();
}

bool saveBudgets(const vector<Budget>& src)
{
    ofstream f(BUDGET_FILE, ios::trunc);
    for (const auto& b : src) f << budgetToLine(b) << "\n";
    return f.good();
}