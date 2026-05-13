
#pragma once
#include <string>
#include <vector>
using namespace std;

const double MAX_AMOUNT = 1000000.0;
const char   DATA_FILE[] = "expenses.dat";
const char   USERS_FILE[] = "users.dat";
const char   BUDGET_FILE[] = "budgets.dat";

enum Category {
    CAT_FOOD = 0, CAT_TRANSPORT, CAT_HOUSING, CAT_HEALTH,
    CAT_ENTERTAINMENT, CAT_SHOPPING, CAT_EDUCATION, CAT_OTHER,
    CAT_COUNT
};

struct Expense {
    int      id;
    string   username;
    string   description;
    double   amount;
    Category category;
    int      day, month, year;
    bool     active;
    bool     completed;   
};

struct User {
    string username;
    string passwordHash;
    string email;
    bool   active;
};

struct Budget {
    string username;
    int    month, year;
    double limit;
};


struct Notification {
    string message;
    bool   read;
};

string categoryName(Category cat);

int  loadExpenses(vector<Expense>& dest);
bool saveExpenses(const vector<Expense>& src);
bool appendExpense(const Expense& e);
bool updateExpense(const vector<Expense>& src);
bool deleteExpense(vector<Expense>& src, int idx);
int  nextExpenseId(const vector<Expense>& src);

int  loadUsers(vector<User>& dest);
bool saveUsers(const vector<User>& src);

int  loadBudgets(vector<Budget>& dest);
bool saveBudgets(const vector<Budget>& src);