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
