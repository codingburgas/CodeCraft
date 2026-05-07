/*
 * logic.cpp
 * Logic layer - sorting, searching, recursion, CRUD, auth, budget.
 */
#include "../include/logic.h"
#include "../include/data.h"
#include <cctype>
using namespace std;

// ── AUTH ──────────────────────────────────────────────────────────────────

// djb2 hash - converts password to a number stored as text
string hashPassword(const string& p)
{
    unsigned long h = 5381;
    for (char c : p) h = h * 33 + (unsigned char)c;
    return to_string(h);
}

bool usernameExists(const vector<User>& users, const string& username)
{
    for (const auto& u : users)
        if (u.username == username && u.active) return true;
    return false;
}

bool registerUser(vector<User>& users, const string& username,
    const string& password, const string& email)
{
    if (!isValidUsername(username) || !isValidPassword(password) ||
        !isValidEmail(email) || usernameExists(users, username))
        return false;
    User u;
    u.username = username; u.passwordHash = hashPassword(password);
    u.email = email; u.active = true;
    users.push_back(u);
    return saveUsers(users);
}

bool loginUser(const vector<User>& users, const string& username, const string& password)
{
    string h = hashPassword(password);
    for (const auto& u : users)
        if (u.username == username && u.passwordHash == h && u.active) return true;
    return false;
}

bool changePassword(vector<User>& users, const string& username,
    const string& oldPass, const string& newPass)
{
    if (!isValidPassword(newPass)) return false;
    for (auto& u : users) {
        if (u.username == username && u.passwordHash == hashPassword(oldPass)) {
            u.passwordHash = hashPassword(newPass);
            return saveUsers(users);
        }
    }
    return false;
}

// ── SORTING ───────────────────────────────────────────────────────────────

// Bubble Sort by amount ascending - compares neighbours, swaps if wrong order
void bubbleSortByAmount(vector<Expense>& v)
{
    int n = (int)v.size();
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < n - i - 1; j++)
            if (v[j].amount > v[j + 1].amount) { swap(v[j], v[j + 1]); swapped = true; }
        if (!swapped) break;
    }
}

// Converts date to one integer for easy comparison e.g. 2024/05/01 -> 20240501
static int dateInt(const Expense& e) { return e.year * 10000 + e.month * 100 + e.day; }

static int partition(vector<Expense>& v, int low, int high)
{
    int pivot = dateInt(v[high]), i = low - 1;
    for (int j = low; j < high; j++)
        if (dateInt(v[j]) <= pivot) swap(v[++i], v[j]);
    swap(v[i + 1], v[high]);
    return i + 1;
}

// Quick Sort by date - divides around a pivot and sorts each half
void quickSortByDate(vector<Expense>& v, int low, int high)
{
    if (low < high) {
        int pi = partition(v, low, high);
        quickSortByDate(v, low, pi - 1);
        quickSortByDate(v, pi + 1, high);
    }
}
// Bubble Sort by category enum value
void bubbleSortByCategory(vector<Expense>& v)
{
    int n = (int)v.size();
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < n - i - 1; j++)
            if ((int)v[j].category > (int)v[j + 1].category)
            {
                swap(v[j], v[j + 1]); swapped = true;
            }
        if (!swapped) break;
    }
}

// ── SEARCHING ─────────────────────────────────────────────────────────────

// Linear search - checks every description for keyword (case-insensitive)
vector<int> linearSearch(const vector<Expense>& v, const string& keyword)
{
    vector<int> results;
    string kw = keyword;
    for (char& c : kw) c = tolower(c);
    for (int i = 0; i < (int)v.size(); i++) {
        string desc = v[i].description;
        for (char& c : desc) c = tolower(c);
        if (desc.find(kw) != string::npos) results.push_back(i);
    }
    return results;
}

// Binary search - requires sorted input; splits in half each step
int binarySearch(const vector<Expense>& sorted, double target)
{
    int low = 0, high = (int)sorted.size() - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (sorted[mid].amount == target) return mid;
        else if (sorted[mid].amount < target) low = mid + 1;
        else                                   high = mid - 1;
    }
    return -1;
}

// ── RECURSION ─────────────────────────────────────────────────────────────

// Sums all amounts from index i to end - base case: i reaches end
double recursiveTotal(const vector<Expense>& v, int i, int end)
{
    if (i >= end) return 0.0;
    return v[i].amount + recursiveTotal(v, i + 1, end);
}

// Sums amounts for a specific month/year, going backwards from i
double recursiveMonthlyTotal(const vector<Expense>& v, int i, int month, int year)
{
    if (i < 0) return 0.0;
    double add = (v[i].month == month && v[i].year == year) ? v[i].amount : 0.0;
    return add + recursiveMonthlyTotal(v, i - 1, month, year);
}

// Counts expenses in a given category, going backwards from i
int recursiveCountByCategory(const vector<Expense>& v, int i, Category cat)
{
    if (i < 0) return 0;
    return (v[i].category == cat ? 1 : 0) + recursiveCountByCategory(v, i - 1, cat);
}

// ── CRUD ──────────────────────────────────────────────────────────────────

bool addExpense(vector<Expense>& v, const string& user, const string& desc,
    double amount, Category cat, int day, int month, int year)
{
    if (desc.empty() || !isValidAmount(amount) || !isValidDate(day, month, year))
        return false;
    Expense e;
    e.id = nextExpenseId(v); e.username = user; e.description = desc;
    e.amount = amount; e.category = cat;
    e.day = day; e.month = month; e.year = year;
    e.active = true; e.completed = false;
    v.push_back(e);
    return appendExpense(e);
}

bool editExpense(vector<Expense>& v, int idx, const string& desc,
    double amount, Category cat, int day, int month, int year)
{
    if (idx < 0 || idx >= (int)v.size()) return false;
    if (desc.empty() || !isValidAmount(amount) || !isValidDate(day, month, year))
        return false;
    v[idx].description = desc; v[idx].amount = amount; v[idx].category = cat;
    v[idx].day = day; v[idx].month = month; v[idx].year = year;
    return updateExpense(v);
}

bool removeExpense(vector<Expense>& v, int idx)
{
    if (idx < 0 || idx >= (int)v.size()) return false;
    bool ok = deleteExpense(v, idx);
    if (ok) v.erase(v.begin() + idx);
    return ok;
}
// Toggles the completed flag and saves to disk
bool toggleCompleted(vector<Expense>& v, int idx)
{
    if (idx < 0 || idx >= (int)v.size()) return false;
    v[idx].completed = !v[idx].completed;
    return updateExpense(v);
}

// ── FILTERING ─────────────────────────────────────────────────────────────

vector<Expense> userExpenses(const vector<Expense>& all, const string& username)
{
    vector<Expense> r;
    for (const auto& e : all) if (e.username == username) r.push_back(e);
    return r;
}

vector<Expense> filterByMonth(const vector<Expense>& v, int month, int year)
{
    vector<Expense> r;
    for (const auto& e : v) if (e.month == month && e.year == year) r.push_back(e);
    return r;
}

vector<Expense> filterByCategory(const vector<Expense>& v, Category cat)
{
    vector<Expense> r;
    for (const auto& e : v) if (e.category == cat) r.push_back(e);
    return r;
}

vector<Expense> filterByRange(const vector<Expense>& v, double minA, double maxA)
{
    vector<Expense> r;
    for (const auto& e : v) if (e.amount >= minA && e.amount <= maxA) r.push_back(e);
    return r;
}

// ── STATISTICS ────────────────────────────────────────────────────────────

double totalExpenses(const vector<Expense>& v)
{
    return recursiveTotal(v, 0, (int)v.size());
}

double avgExpense(const vector<Expense>& v)
{
    return v.empty() ? 0.0 : totalExpenses(v) / v.size();
}

double maxExpense(const vector<Expense>& v)
{
    double m = 0.0;
    for (const auto& e : v) if (e.amount > m) m = e.amount;
    return m;
}

double minExpense(const vector<Expense>& v)
{
    if (v.empty()) return 0.0;
    double m = v[0].amount;
    for (const auto& e : v) if (e.amount < m) m = e.amount;
    return m;
}

void categoryTotals(const vector<Expense>& v, double totals[CAT_COUNT])
{
    for (int i = 0; i < CAT_COUNT; i++) totals[i] = 0.0;
    for (const auto& e : v) totals[(int)e.category] += e.amount;
}

// ── BUDGET ────────────────────────────────────────────────────────────────

bool setBudget(vector<Budget>& b, const string& user, int month, int year, double limit)
{
    if (limit <= 0.0) return false;
    for (auto& x : b)
        if (x.username == user && x.month == month && x.year == year)
        {
            x.limit = limit; return saveBudgets(b);
        }
    b.push_back({ user, month, year, limit });
    return saveBudgets(b);
}

double getBudget(const vector<Budget>& b, const string& user, int month, int year)
{
    for (const auto& x : b)
        if (x.username == user && x.month == month && x.year == year) return x.limit;
    return 0.0;
}

// Sums all expenses for the user in the given month/year
double getBudgetUsed(const vector<Expense>& v, const string& user, int month, int year)
{
    double total = 0.0;
    for (const auto& e : v)
        if (e.username == user && e.month == month && e.year == year)
            total += e.amount;
    return total;
}

// ── VALIDATION ────────────────────────────────────────────────────────────

bool isValidDate(int day, int month, int year)
{
    if (year < 2000 || year > 2100 || month < 1 || month > 12 || day < 1) return false;
    const int dim[] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
    int maxDay = dim[month];
    if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) maxDay = 29;
    return day <= maxDay;
}

bool isValidAmount(double a) { return a > 0.0 && a <= MAX_AMOUNT; }
bool isValidPassword(const string& s) { return s.size() >= 6; }

bool isValidUsername(const string& s)
{
    if (s.size() < 3 || s.size() > 20) return false;
    for (char c : s) if (!isalnum(c) && c != '_') return false;
    return true;
}

bool isValidEmail(const string& s)
{
    size_t at = s.find('@');
    if (at == string::npos || at == 0) return false;
    size_t dot = s.find('.', at);
    return dot != string::npos && dot > at + 1 && dot < s.size() - 1;
}

// ── LOADERS ───────────────────────────────────────────────────────────────

vector<Expense> loadAllExpenses() { vector<Expense> v; loadExpenses(v); return v; }
vector<User>    loadAllUsers() { vector<User>    v; loadUsers(v);    return v; }
vector<Budget>  loadAllBudgets() { vector<Budget>  v; loadBudgets(v);  return v; }