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
