
#pragma once
#include "data.h"
#include <vector>
#include <string>
using namespace std;


string hashPassword(const string& password);
bool   registerUser(vector<User>& users, const string& username, const string& password, const string& email);
bool   loginUser(const vector<User>& users, const string& username, const string& password);
bool   usernameExists(const vector<User>& users, const string& username);
bool   changePassword(vector<User>& users, const string& username, const string& oldPass, const string& newPass);


void bubbleSortByAmount(vector<Expense>& v);
void quickSortByDate(vector<Expense>& v, int low, int high);
void bubbleSortByCategory(vector<Expense>& v);


vector<int> linearSearch(const vector<Expense>& v, const string& keyword);
int         binarySearch(const vector<Expense>& sorted, double target);


double recursiveTotal(const vector<Expense>& v, int i, int end);
double recursiveMonthlyTotal(const vector<Expense>& v, int i, int month, int year);
int    recursiveCountByCategory(const vector<Expense>& v, int i, Category cat);


bool addExpense(vector<Expense>& v, const string& user, const string& desc,
    double amount, Category cat, int day, int month, int year);
bool editExpense(vector<Expense>& v, int idx, const string& desc,
    double amount, Category cat, int day, int month, int year);
bool removeExpense(vector<Expense>& v, int idx);
bool toggleCompleted(vector<Expense>& v, int idx);   


vector<Expense> userExpenses(const vector<Expense>& all, const string& username);
vector<Expense> filterByMonth(const vector<Expense>& v, int month, int year);
vector<Expense> filterByCategory(const vector<Expense>& v, Category cat);
vector<Expense> filterByRange(const vector<Expense>& v, double minA, double maxA);


double totalExpenses(const vector<Expense>& v);
double avgExpense(const vector<Expense>& v);
double maxExpense(const vector<Expense>& v);
double minExpense(const vector<Expense>& v);
void   categoryTotals(const vector<Expense>& v, double totals[CAT_COUNT]);


bool   setBudget(vector<Budget>& b, const string& user, int month, int year, double limit);
double getBudget(const vector<Budget>& b, const string& user, int month, int year);
double getBudgetUsed(const vector<Expense>& v, const string& user, int month, int year);

bool isValidDate(int day, int month, int year);
bool isValidAmount(double amount);
bool isValidUsername(const string& s);
bool isValidPassword(const string& s);
bool isValidEmail(const string& s);


vector<Expense> loadAllExpenses();
vector<User>    loadAllUsers();
vector<Budget>  loadAllBudgets();