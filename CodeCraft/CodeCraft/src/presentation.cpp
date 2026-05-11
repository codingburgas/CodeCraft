/*
 * presentation.cpp
 * Presentation layer - Dear ImGui UI.
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
#include <cmath>
using namespace std;

// ── Colour palette (dark professional theme) ───────────────────────────────
static const ImVec4 COL_BG_DEEP = ImVec4(0.06f, 0.07f, 0.10f, 1.0f);
static const ImVec4 COL_BG_PANEL = ImVec4(0.10f, 0.11f, 0.15f, 1.0f);
static const ImVec4 COL_BG_WIDGET = ImVec4(0.14f, 0.15f, 0.20f, 1.0f);
static const ImVec4 COL_ACCENT = ImVec4(0.25f, 0.60f, 1.00f, 1.0f);
static const ImVec4 COL_ACCENT2 = ImVec4(0.40f, 0.75f, 1.00f, 1.0f);
static const ImVec4 COL_SUCCESS = ImVec4(0.18f, 0.80f, 0.42f, 1.0f);
static const ImVec4 COL_ERROR = ImVec4(0.95f, 0.30f, 0.30f, 1.0f);
static const ImVec4 COL_MUTED = ImVec4(0.50f, 0.52f, 0.58f, 1.0f);
static const ImVec4 COL_WARNING = ImVec4(1.00f, 0.72f, 0.00f, 1.0f);
static const ImVec4 COL_DONE = ImVec4(0.35f, 0.37f, 0.42f, 1.0f);
static const ImVec4 COL_SEPARATOR = ImVec4(0.20f, 0.21f, 0.28f, 1.0f);

static const ImVec4 COL_BTN_PRIMARY = ImVec4(0.15f, 0.42f, 0.90f, 1.0f);
static const ImVec4 COL_BTN_SUCCESS = ImVec4(0.12f, 0.52f, 0.22f, 1.0f);
static const ImVec4 COL_BTN_DANGER = ImVec4(0.75f, 0.18f, 0.18f, 1.0f);
static const ImVec4 COL_BTN_NEUTRAL = ImVec4(0.22f, 0.24f, 0.30f, 1.0f);
static const ImVec4 COL_BTN_EDIT = ImVec4(0.18f, 0.44f, 0.80f, 0.90f);
static const ImVec4 COL_BTN_WARN = ImVec4(0.75f, 0.45f, 0.08f, 1.0f);

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

static const char* CAT_NAMES[CAT_COUNT] = {
    "Food","Transport","Housing","Health",
    "Entertainment","Shopping","Education","Other"
};

static const char* MONTHS[] = {
    "All","January","February","March","April","May","June",
    "July","August","September","October","November","December"
};

// ── Animation helpers ──────────────────────────────────────────────────────

// Returns a value [0..1] that pulses with time - use for glowing effects
static float pulse(float speed = 2.0f, float minVal = 0.7f)
{
    float t = (float)ImGui::GetTime();
    return minVal + (1.0f - minVal) * (0.5f + 0.5f * sinf(t * speed));
}

// Lerp between two colours using a 0..1 factor
static ImVec4 lerpColor(const ImVec4& a, const ImVec4& b, float t)
{
    return ImVec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t);
}

// Slightly brighten a colour for hover state
static ImVec4 brighten(const ImVec4& c, float f = 1.25f)
{
    return ImVec4(
        fminf(c.x * f, 1.0f),
        fminf(c.y * f, 1.0f),
        fminf(c.z * f, 1.0f),
        c.w);
}

// Push a full styled button (normal + hovered + active colours)
static void pushBtnStyle(const ImVec4& base)
{
    ImGui::PushStyleColor(ImGuiCol_Button, base);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, brighten(base, 1.20f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, brighten(base, 0.85f));
}
static void popBtnStyle() { ImGui::PopStyleColor(3); }

// Animated underline accent - draws a horizontal gradient line under cursor
static void accentLine(float width = -1.0f)
{
    float w = (width < 0) ? ImGui::GetContentRegionAvail().x : width;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float  h = 2.0f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 colL = ImGui::ColorConvertFloat4ToU32(COL_ACCENT);
    ImU32 colR = ImGui::ColorConvertFloat4ToU32(ImVec4(0.25f, 0.60f, 1.00f, 0.0f));
    dl->AddRectFilledMultiColor(p, ImVec2(p.x + w, p.y + h), colL, colR, colR, colL);
    ImGui::Dummy(ImVec2(w, h));
}

// Draw a small coloured dot (category indicator)
static void colorDot(const ImVec4& col, float r = 4.0f)
{
    ImVec2 cp = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled(
        ImVec2(cp.x + r, cp.y + r + 2.0f), r,
        ImGui::ColorConvertFloat4ToU32(col), 12);
    ImGui::Dummy(ImVec2(r * 2.0f + 4.0f, r * 2.0f));
}

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

// ── Section header helper ──────────────────────────────────────────────────
static void sectionHeader(const char* title, const ImVec4& col = COL_ACCENT)
{
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::Text("%s", title);
    ImGui::PopStyleColor();
    accentLine();
    ImGui::Spacing();
}

// ────────────────────────────────────────────────────────────────────────────
//  renderLoginScreen
// ────────────────────────────────────────────────────────────────────────────
void renderLoginScreen(AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();
    float W = io.DisplaySize.x, H = io.DisplaySize.y;
    float t = (float)ImGui::GetTime();

    // ── Full-screen custom background ─────────────────────────────────────────
    ImDrawList* bg = ImGui::GetBackgroundDrawList();

    // Deep navy gradient
    bg->AddRectFilledMultiColor(
        ImVec2(0, 0), ImVec2(W, H),
        IM_COL32(6, 8, 18, 255), IM_COL32(8, 12, 28, 255),
        IM_COL32(10, 8, 20, 255), IM_COL32(6, 8, 18, 255));

    // Animated concentric rings
    for (int i = 0; i < 4; i++) {
        float r = 200.0f + i * 90.0f + 20.0f * sinf(t * 0.4f + i);
        float a = (int)(22 - i * 4 + 6 * sinf(t * 0.5f + i));
        bg->AddCircle(ImVec2(W * 0.5f, H * 0.5f), r,
            IM_COL32(40, 90, 200, (int)a), 64, 1.2f);
    }

    // Moving particles
    for (int i = 0; i < 18; i++) {
        float px = W * (0.05f + 0.055f * i + 0.012f * sinf(t * 0.3f + i * 1.3f));
        float py = H * (0.1f + 0.05f * i + 0.015f * cosf(t * 0.25f + i * 0.9f));
        float pr = 1.5f + 1.0f * sinf(t + i);
        int   pa = (int)(30 + 25 * sinf(t * 0.6f + i));
        bg->AddCircleFilled(ImVec2(px, py), pr, IM_COL32(80, 140, 255, pa));
    }

    // Diagonal light streak
    {
        float sx = W * (0.3f + 0.05f * sinf(t * 0.2f));
        ImU32 strk = IM_COL32(60, 110, 220, (int)(18 + 10 * sinf(t * 0.3f)));
        bg->AddLine(ImVec2(sx, 0), ImVec2(sx + 300, H), strk, 60.0f);
    }

    // ── Card window ───────────────────────────────────────────────────────────
    float CW = 400.0f, CH = 340.0f;
    float cx = (W - CW) * 0.5f, cy = (H - CH) * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(cx, cy), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(CW, CH), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.10f, 0.15f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.50f, 0.95f, 0.50f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32, 26));
    ImGui::Begin("##login", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar(2);

    // Custom top accent bar inside card
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float ap = pulse(1.4f, 0.5f);
        ImU32 aL = IM_COL32((int)(40 * ap), (int)(100 * ap), (int)(240 * ap), 220);
        ImU32 aR = IM_COL32((int)(20 * ap), (int)(180 * ap), (int)(140 * ap), 180);
        ImU32 aT = IM_COL32(0, 0, 0, 0);
        dl->AddRectFilledMultiColor(wp, ImVec2(wp.x + CW, wp.y + 4), aL, aR, aR, aL);
        // Corner glow dots
        dl->AddCircleFilled(ImVec2(wp.x + 1, wp.y + 1), 6, IM_COL32(60, 140, 255, 80));
        dl->AddCircleFilled(ImVec2(wp.x + CW - 1, wp.y + 1), 6, IM_COL32(40, 200, 160, 60));
        (void)aT;
    }

    // Title
    ImGui::Spacing();
    float tp2 = pulse(1.3f, 0.75f);
    ImVec4 tC = lerpColor(COL_ACCENT, COL_ACCENT2, tp2);
    const char* title = "Expense Tracker";
    float tw = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((CW - tw) * 0.5f);
    ImGui::SetWindowFontScale(1.50f);
    ImGui::PushStyleColor(ImGuiCol_Text, tC);
    ImGui::Text("%s", title);
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);

    // Tagline
    const char* sub = "Track. Analyse. Save.";
    float sw = ImGui::CalcTextSize(sub).x;
    ImGui::SetCursorPosX((CW - sw) * 0.5f);
    ImGui::TextColored(COL_MUTED, "%s", sub);

    ImGui::Spacing();
    accentLine();
    ImGui::Spacing();

    // Fields
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.13f, 0.15f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.17f, 0.20f, 0.30f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));

    ImGui::TextColored(COL_MUTED, "Username");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##lu", state.loginUser, sizeof(state.loginUser));
    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Password");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##lp", state.loginPass, sizeof(state.loginPass),
        ImGuiInputTextFlags_Password);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    if (state.loginError[0] != '\0') {
        ImGui::Spacing();
        ImGui::TextColored(COL_ERROR, "  %s", state.loginError);
    }
    ImGui::Spacing();

    // Big login button
    pushBtnStyle(COL_BTN_PRIMARY);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 9));
    if (ImGui::Button("Log In", ImVec2(-1, 0))) {
        if (loginUser(state.users, state.loginUser, state.loginPass)) {
            state.loggedInUser = state.loginUser;
            state.currentScreen = SCREEN_DASHBOARD;
            state.notif90Sent = state.notif100Sent = false;
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
    ImGui::PopStyleVar(2);
    popBtnStyle();

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Don't have an account?");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, COL_ACCENT);
    if (ImGui::SmallButton("Register here")) {
        state.currentScreen = SCREEN_REGISTER;
        memset(state.loginError, 0, sizeof(state.loginError));
    }
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor(2);
}

// ────────────────────────────────────────────────────────────────────────────
//  renderRegisterScreen
// ────────────────────────────────────────────────────────────────────────────
void renderRegisterScreen(AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();

    // Same animated background
    {
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        float t = (float)ImGui::GetTime();
        for (int i = 0; i < 6; i++) {
            float x = io.DisplaySize.x * (0.1f + 0.15f * i + 0.02f * sinf(t * 0.3f + i));
            float y = io.DisplaySize.y * (0.2f + 0.1f * i + 0.03f * cosf(t * 0.4f + i));
            ImU32 c = IM_COL32(20, 60, 140, (int)(18 + 8 * sinf(t + i)));
            ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(x, y), 60.0f, c);
        }
    }
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420, 400), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_BG_PANEL);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.44f, 0.80f, 0.45f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.2f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28, 22));
    ImGui::Begin("##register", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar(2);

    ImGui::SetWindowFontScale(1.30f);
    ImGui::PushStyleColor(ImGuiCol_Text, COL_ACCENT);
    ImGui::Text("Create Account");
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);
    accentLine();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_BG_WIDGET);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.20f, 0.27f, 1.0f));

    ImGui::TextColored(COL_MUTED, "Username (3-20 chars, letters/numbers/_)");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##ru", state.regUser, sizeof(state.regUser));

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Email");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##re", state.regEmail, sizeof(state.regEmail));

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Password (min 6 characters)");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##rp", state.regPass, sizeof(state.regPass),
        ImGuiInputTextFlags_Password);

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Confirm Password");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##rp2", state.regPass2, sizeof(state.regPass2),
        ImGuiInputTextFlags_Password);
    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    if (state.regError[0] != '\0') ImGui::TextColored(COL_ERROR, "  %s", state.regError);
    if (state.regSuccess[0] != '\0') ImGui::TextColored(COL_SUCCESS, "  %s", state.regSuccess);
    ImGui::Spacing();

    pushBtnStyle(COL_BTN_SUCCESS);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Button("Create Account", ImVec2(-1, 36))) {
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
    ImGui::PopStyleVar();
    popBtnStyle();

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Already have an account?");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, COL_ACCENT);
    if (ImGui::SmallButton("Log in here")) {
        state.currentScreen = SCREEN_LOGIN;
        memset(state.regError, 0, sizeof(state.regError));
        memset(state.regSuccess, 0, sizeof(state.regSuccess));
    }
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor(2);
}

// ────────────────────────────────────────────────────────────────────────────
//  renderHeader
// ────────────────────────────────────────────────────────────────────────────
void renderHeader(AppState& state)
{
    const float HDR_H = 58.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::BeginChild("##hdr", ImVec2(0, HDR_H), false);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2      wpos = ImGui::GetWindowPos();
    float       W = ImGui::GetWindowWidth();

    // ── Gradient background ──────────────────────────────────────────────────
    ImU32 bgL = IM_COL32(14, 20, 38, 255);
    ImU32 bgR = IM_COL32(10, 14, 24, 255);
    dl->AddRectFilledMultiColor(wpos, ImVec2(wpos.x + W, wpos.y + HDR_H),
        bgL, bgR, bgR, bgL);

    // Animated blue glow strip at the bottom of header
    float gp = pulse(1.5f, 0.4f);
    ImU32 glowC = IM_COL32((int)(40 * gp), (int)(100 * gp), (int)(220 * gp), (int)(180 * gp));
    ImU32 glowT = IM_COL32(0, 0, 0, 0);
    dl->AddRectFilledMultiColor(
        ImVec2(wpos.x, wpos.y + HDR_H - 3),
        ImVec2(wpos.x + W * 0.5f, wpos.y + HDR_H),
        glowT, glowC, glowC, glowT);
    dl->AddRectFilledMultiColor(
        ImVec2(wpos.x + W * 0.5f, wpos.y + HDR_H - 3),
        ImVec2(wpos.x + W, wpos.y + HDR_H),
        glowC, glowT, glowT, glowC);

    // ── Logo / Title ─────────────────────────────────────────────────────────
    // Draw a small coloured square logo
    float lx = wpos.x + 14, ly = wpos.y + 14;
    dl->AddRectFilled(ImVec2(lx, ly), ImVec2(lx + 8, ly + 8),
        IM_COL32(80, 160, 255, 255), 2.0f);
    dl->AddRectFilled(ImVec2(lx + 10, ly), ImVec2(lx + 18, ly + 8),
        IM_COL32(50, 200, 130, 255), 2.0f);
    dl->AddRectFilled(ImVec2(lx, ly + 10), ImVec2(lx + 18, ly + 18),
        IM_COL32(60, 130, 240, 180), 2.0f);

    ImGui::SetCursorPos(ImVec2(38, 16));
    float tp = pulse(1.2f, 0.80f);
    ImVec4 titleC = lerpColor(COL_ACCENT, COL_ACCENT2, tp);
    ImGui::PushStyleColor(ImGuiCol_Text, titleC);
    ImGui::SetWindowFontScale(1.30f);
    ImGui::Text("Expense Tracker");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // ── Budget warning badge ─────────────────────────────────────────────────
    double used = getBudgetUsed(state.allExpenses, state.loggedInUser,
        state.budgetMonth, state.budgetYear);
    double limit = getBudget(state.budgets, state.loggedInUser,
        state.budgetMonth, state.budgetYear);
    if (limit > 0.0 && used >= limit * 0.9) {
        bool over = (used >= limit);
        float wp2 = pulse(3.0f, 0.55f);
        ImU32 badgeC = over
            ? IM_COL32((int)(220), (int)(50 + 60 * wp2), (int)(50 + 60 * wp2), 220)
            : IM_COL32(200, 140, 0, 210);
        // Draw pill badge
        const char* warnTxt = over ? "!! Budget exceeded !!" : "! Near limit";
        ImVec2 ts = ImGui::CalcTextSize(warnTxt);
        float bx = wpos.x + 230, by = wpos.y + 18;
        dl->AddRectFilled(ImVec2(bx - 6, by - 3), ImVec2(bx + ts.x + 6, by + ts.y + 3), badgeC, 6.0f);
        dl->AddText(ImVec2(bx, by), IM_COL32(255, 255, 255, 255), warnTxt);
        ImGui::Dummy(ImVec2(0, 0)); // keep layout flowing
    }

    // ── Right side: total, user, buttons ────────────────────────────────────
    // Total pill
    {
        char totTxt[48];
        snprintf(totTxt, sizeof(totTxt), "$%.2f", totalExpenses(state.expenses));
        float bx = wpos.x + W - 310, by = wpos.y + 14;
        ImVec2 ts = ImGui::CalcTextSize(totTxt);
        dl->AddRectFilled(ImVec2(bx - 28, by - 4), ImVec2(bx + ts.x + 8, by + ts.y + 4),
            IM_COL32(20, 50, 110, 200), 8.0f);
        dl->AddText(ImVec2(bx - 22, by), IM_COL32(130, 170, 255, 255), "Total ");
        dl->AddText(ImVec2(bx - 22 + ImGui::CalcTextSize("Total ").x, by),
            IM_COL32(100, 200, 255, 255), totTxt);
    }
    // User pill
    {
        float bx = wpos.x + W - 200, by = wpos.y + 14;
        ImVec2 ts = ImGui::CalcTextSize(state.loggedInUser.c_str());
        dl->AddRectFilled(ImVec2(bx - 6, by - 4), ImVec2(bx + ts.x + 8, by + ts.y + 4),
            IM_COL32(20, 80, 40, 200), 8.0f);
        dl->AddText(ImVec2(bx, by), IM_COL32(80, 220, 120, 255), state.loggedInUser.c_str());
    }

    // Buttons on the far right
    ImGui::SameLine(W - 168);
    ImGui::SetCursorPosY(15);

    int unread = unreadCount(state);
    char bellLbl[32];
    snprintf(bellLbl, sizeof(bellLbl), unread > 0 ? "Bell(%d)##nb" : "Bell##nb", unread);
    pushBtnStyle(unread > 0 ? COL_BTN_WARN : COL_BTN_NEUTRAL);
    if (ImGui::SmallButton(bellLbl)) state.showNotifCenter = !state.showNotifCenter;
    popBtnStyle();
    ImGui::SameLine(0, 6);

    pushBtnStyle(COL_BTN_NEUTRAL);
    if (ImGui::SmallButton("Settings")) state.showSettings = !state.showSettings;
    popBtnStyle();
    ImGui::SameLine(0, 6);

    pushBtnStyle(COL_BTN_DANGER);
    if (ImGui::SmallButton("Logout")) {
        state.loggedInUser = "";
        state.currentScreen = SCREEN_LOGIN;
        state.expenses.clear();
        state.notifications.clear();
        state.editIdx = -1; state.deleteIdx = -1; state.switchToEdit = false;
        state.notif90Sent = false; state.notif100Sent = false;
        memset(state.statusMsg, 0, sizeof(state.statusMsg));
    }
    popBtnStyle();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ────────────────────────────────────────────────────────────────────────────
//  renderDeleteModal
// ────────────────────────────────────────────────────────────────────────────
void renderDeleteModal(AppState& state)
{
    // The popup ID must match exactly what was passed to OpenPopup
    if (!ImGui::BeginPopupModal("Confirm Delete##modal", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        return;

    ImGui::Spacing();
    ImGui::TextColored(COL_WARNING, "  Delete this expense?");
    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "  This action cannot be undone.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    pushBtnStyle(COL_BTN_DANGER);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    if (ImGui::Button("  Yes, delete  ", ImVec2(130, 32))) {
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
    ImGui::PopStyleVar();
    popBtnStyle();

    ImGui::SameLine(0, 12);

    pushBtnStyle(COL_BTN_NEUTRAL);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    if (ImGui::Button("  Cancel  ", ImVec2(100, 32))) {
        state.deleteIdx = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleVar();
    popBtnStyle();

    ImGui::Spacing();
    ImGui::EndPopup();
}

// ────────────────────────────────────────────────────────────────────────────
//  renderExpenseTable
// ────────────────────────────────────────────────────────────────────────────
void renderExpenseTable(AppState& state)
{
    // ── Filter/sort bar ─────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_BG_WIDGET);

    ImGui::TextColored(COL_MUTED, "Sort:");
    ImGui::SameLine();
    ImGui::RadioButton("Amount", &state.sortMode, 0); ImGui::SameLine();
    ImGui::RadioButton("Date", &state.sortMode, 1); ImGui::SameLine();
    ImGui::RadioButton("Category", &state.sortMode, 2);

    ImGui::SameLine(0, 14); ImGui::TextColored(COL_MUTED, "Month:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("##fm", &state.filterMonth, MONTHS, 13);
    if (state.filterMonth > 0) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(72);
        ImGui::InputInt("##fy", &state.filterYear, 1, 10);
    }

    const char* catCombo[CAT_COUNT + 1];
    catCombo[0] = "All";
    for (int i = 0; i < CAT_COUNT; i++) catCombo[i + 1] = CAT_NAMES[i];
    int catFilter = state.filterCat + 1;
    ImGui::SameLine(0, 14); ImGui::TextColored(COL_MUTED, "Cat:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(110);
    ImGui::Combo("##fc", &catFilter, catCombo, CAT_COUNT + 1);
    state.filterCat = catFilter - 1;

    ImGui::SameLine(0, 14);
    ImGui::Checkbox("Amt range", &state.useAmountFilter);
    if (state.useAmountFilter) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(65);
        ImGui::InputFloat("##fmn", &state.filterMinAmt, 0, 0, "%.0f");
        ImGui::SameLine(); ImGui::TextColored(COL_MUTED, "-"); ImGui::SameLine();
        ImGui::SetNextItemWidth(65);
        ImGui::InputFloat("##fmx", &state.filterMaxAmt, 0, 0, "%.0f");
    }

    ImGui::PopStyleColor();

    // ── Build display list ───────────────────────────────────────────────────
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

    // ── Record count badge ───────────────────────────────────────────────────
    ImGui::Spacing();
    {
        ImVec2 cp = ImGui::GetCursorScreenPos();
        char cnt[32]; snprintf(cnt, sizeof(cnt), " %d record(s)", (int)disp.size());
        ImVec2 ts = ImGui::CalcTextSize(cnt);
        ImGui::GetWindowDrawList()->AddRectFilled(
            cp, ImVec2(cp.x + ts.x + 8, cp.y + ts.y + 4),
            IM_COL32(30, 60, 120, 180), 4.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::TextColored(COL_ACCENT2, "%s", cnt);
    }
    ImGui::Spacing();

    // ── Table ────────────────────────────────────────────────────────────────
    const ImGuiTableFlags tflags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp;

    // Subtle alternating row tint
    ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.10f, 0.11f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.12f, 0.13f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, COL_SEPARATOR);
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0.22f, 0.40f, 0.75f, 0.5f));

    // FIX: declared BEFORE BeginTable so it remains in scope after EndTable
    int pendingDeleteUserIdx = -1;

    float tableH = ImGui::GetContentRegionAvail().y - 28.0f;
    if (ImGui::BeginTable("##tbl", 8, tflags, ImVec2(0, tableH)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 34);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Amount($)", ImGuiTableColumnFlags_WidthFixed, 82);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 112);
        ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 88);
        ImGui::TableSetupColumn("Done", ImGuiTableColumnFlags_WidthFixed, 46);
        ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthFixed, 38);
        ImGui::TableSetupColumn("Del", ImGuiTableColumnFlags_WidthFixed, 34);

        // Styled header row
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.14f, 0.22f, 0.40f, 1.0f));
        ImGui::TableHeadersRow();
        ImGui::PopStyleColor();

        for (int i = 0; i < (int)disp.size(); i++)
        {
            const Expense& e = disp[i];
            int userIdx = -1;
            for (int k = 0; k < (int)state.expenses.size(); k++)
                if (state.expenses[k].id == e.id) { userIdx = k; break; }

            ImGui::TableNextRow(0, 22.0f);

            ImVec4 textCol = e.completed ? COL_DONE : ImVec4(0.92f, 0.93f, 0.95f, 1.0f);
            ImVec4 catC = e.completed ? COL_DONE : CAT_COLS[e.category];

            // Col 0: ID with left coloured category bar drawn via DrawList
            ImGui::TableSetColumnIndex(0);
            {
                ImVec2 rmin = ImGui::GetCursorScreenPos();
                // 3px category colour bar on the left of the row
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImVec2(rmin.x - 6, rmin.y),
                    ImVec2(rmin.x - 3, rmin.y + 22),
                    ImGui::ColorConvertFloat4ToU32(catC), 1.0f);
            }
            ImGui::TextColored(COL_MUTED, "%d", e.id);

            // Col 1: Description (with strikethrough if done)
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(textCol, "%s", e.description.c_str());
            if (e.completed) {
                ImVec2 tmin = ImGui::GetItemRectMin();
                ImVec2 tmax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddLine(
                    ImVec2(tmin.x, (tmin.y + tmax.y) * 0.5f),
                    ImVec2(tmax.x, (tmin.y + tmax.y) * 0.5f),
                    ImGui::ColorConvertFloat4ToU32(COL_DONE), 1.2f);
            }

            // Col 2: Amount — green tint for normal, muted for done
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(e.completed ? COL_DONE : COL_ACCENT2, "%.2f", e.amount);

            // Col 3: Category — dot + name
            ImGui::TableSetColumnIndex(3);
            {
                ImVec2 cp = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddCircleFilled(
                    ImVec2(cp.x + 5, cp.y + 9), 4.5f,
                    ImGui::ColorConvertFloat4ToU32(catC), 12);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 13);
                ImGui::TextColored(catC, "%s", CAT_NAMES[e.category]);
            }

            // Col 4: Date
            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(ImVec4(0.55f, 0.60f, 0.70f, 1.0f),
                "%02d/%02d/%04d", e.month, e.day, e.year);

            // ── Done toggle ────────────────────────────────────────────────
            ImGui::TableSetColumnIndex(5);
            ImGui::PushID(e.id * 10 + 3);
            pushBtnStyle(e.completed
                ? ImVec4(0.18f, 0.55f, 0.20f, 0.9f)
                : ImVec4(0.25f, 0.27f, 0.33f, 0.9f));
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
            popBtnStyle();
            ImGui::PopID();

            // ── Edit ──────────────────────────────────────────────────────
            ImGui::TableSetColumnIndex(6);
            ImGui::PushID(e.id * 10 + 1);
            pushBtnStyle(COL_BTN_EDIT);
            if (ImGui::SmallButton("Edit") && userIdx >= 0) {
                state.editIdx = userIdx;
                state.switchToEdit = true;
                snprintf(state.formDesc, sizeof(state.formDesc),
                    "%s", e.description.c_str());
                state.formAmount = (float)e.amount;
                state.formCategory = (int)e.category;
                state.formDay = e.day;
                state.formMonth = e.month;
                state.formYear = e.year;
            }
            popBtnStyle();
            ImGui::PopID();

            // ── Delete ────────────────────────────────────────────────────
            // FIX: store deleteIdx and open popup AFTER the table loop ends,
            // so the popup is in the same ID scope as where we call
            // BeginPopupModal (inside renderDeleteModal below).
            ImGui::TableSetColumnIndex(7);
            ImGui::PushID(e.id * 10 + 2);
            pushBtnStyle(COL_BTN_DANGER);
            if (ImGui::SmallButton("Del") && userIdx >= 0) {
                pendingDeleteUserIdx = userIdx;   // remember, open popup after table
            }
            popBtnStyle();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleColor(4);

    // FIX: Open the popup and call the modal OUTSIDE the table and all PushID
    // scopes.  ImGui::OpenPopup must be in the same window / ID-stack level as
    // BeginPopupModal, otherwise the popup can never be found.
    if (pendingDeleteUserIdx >= 0) {
        state.deleteIdx = pendingDeleteUserIdx;
        ImGui::OpenPopup("Confirm Delete##modal");
    }
    renderDeleteModal(state);
}

// ────────────────────────────────────────────────────────────────────────────
//  renderFormPanel  (shared by Add and Edit tabs)
// ────────────────────────────────────────────────────────────────────────────
void renderFormPanel(AppState& state)
{
    bool editing = (state.editIdx >= 0);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Section header with icon indicator
    {
        ImVec2 cp = ImGui::GetCursorScreenPos();
        float  W = ImGui::GetContentRegionAvail().x;
        // Header pill background
        ImU32 hbg = editing ? IM_COL32(15, 50, 20, 200) : IM_COL32(12, 30, 70, 200);
        dl->AddRectFilled(cp, ImVec2(cp.x + W, cp.y + 26), hbg, 5.0f);
        ImU32 hln = editing ? IM_COL32(40, 200, 80, 180) : IM_COL32(60, 130, 255, 180);
        dl->AddRectFilled(cp, ImVec2(cp.x + 3, cp.y + 26), hln, 2.0f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
        ImGui::TextColored(editing ? COL_SUCCESS : COL_ACCENT,
            editing ? "Edit Expense" : "Add New Expense");
        ImGui::Dummy(ImVec2(W, 2));
    }
    ImGui::Spacing();

    // Field styling
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.13f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.18f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.22f, 0.34f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));

    ImGui::TextColored(COL_MUTED, "Description");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##desc", state.formDesc, sizeof(state.formDesc));

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Amount ($)");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputFloat("##amt", &state.formAmount, 0.01f, 1.0f, "%.2f");

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Category");
    // Draw category colour swatch before the combo
    {
        ImVec2 cp = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(ImVec2(cp.x, cp.y + 3),
            ImVec2(cp.x + 4, cp.y + 22),
            ImGui::ColorConvertFloat4ToU32(CAT_COLS[state.formCategory]), 2.0f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
    }
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##cat", &state.formCategory, CAT_NAMES, CAT_COUNT);

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Date  (DD / MM / YYYY)");
    ImGui::SetNextItemWidth(72); ImGui::InputInt("##fd", &state.formDay, 1, 0);
    ImGui::SameLine(0, 6);
    ImGui::SetNextItemWidth(72); ImGui::InputInt("##fm", &state.formMonth, 1, 0);
    ImGui::SameLine(0, 6);
    ImGui::SetNextItemWidth(90); ImGui::InputInt("##fy2", &state.formYear, 1, 10);

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    ImGui::Spacing();
    // Divider
    {
        ImVec2 cp = ImGui::GetCursorScreenPos();
        float W = ImGui::GetContentRegionAvail().x;
        ImU32 dL = IM_COL32(50, 100, 220, 120), dR = IM_COL32(0, 0, 0, 0);
        dl->AddRectFilledMultiColor(cp, ImVec2(cp.x + W, cp.y + 1), dL, dR, dR, dL);
        ImGui::Dummy(ImVec2(W, 3));
    }
    ImGui::Spacing();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 10));

    pushBtnStyle(editing
        ? ImVec4(0.10f, 0.52f, 0.20f, 1.0f)
        : ImVec4(0.12f, 0.38f, 0.88f, 1.0f));
    if (ImGui::Button(editing ? "  Save Changes  " : "  Add Expense  ",
        ImVec2(-1, 0)))
    {
        string   desc(state.formDesc);
        double   amt = (double)state.formAmount;
        Category cat = (Category)state.formCategory;
        bool     ok = false;

        if (editing) {
            int targetId = state.expenses[state.editIdx].id;
            int gIdx = -1;
            for (int k = 0; k < (int)state.allExpenses.size(); k++)
                if (state.allExpenses[k].id == targetId) { gIdx = k; break; }
            if (gIdx >= 0) {
                ok = editExpense(state.allExpenses, gIdx, desc, amt, cat,
                    state.formDay, state.formMonth, state.formYear);
                if (ok) refreshExpenses(state);
            }
            setStatus(state, ok ? "Expense updated." : "Error - check input.", !ok);
            if (ok) { state.editIdx = -1; state.switchToEdit = false; }
        }
        else {
            ok = addExpense(state.allExpenses, state.loggedInUser,
                desc, amt, cat, state.formDay, state.formMonth, state.formYear);
            if (ok) { refreshExpenses(state); checkBudgetNotifications(state); }
            setStatus(state, ok ? "Expense added." : "Error - check input.", !ok);
        }
        if (ok) {
            memset(state.formDesc, 0, sizeof(state.formDesc));
            state.formAmount = 0.0f; state.formCategory = 0;
            state.formDay = 1; state.formMonth = 1; state.formYear = 2024;
        }
    }
    popBtnStyle();

    if (editing) {
        ImGui::Spacing();
        pushBtnStyle(COL_BTN_NEUTRAL);
        if (ImGui::Button("  Cancel  ", ImVec2(-1, 0)))
        {
            state.editIdx = -1; state.switchToEdit = false;
        }
        popBtnStyle();
    }
    ImGui::PopStyleVar(2);
}

// ────────────────────────────────────────────────────────────────────────────
//  renderSearchPanel
// ────────────────────────────────────────────────────────────────────────────
void renderSearchPanel(AppState& state)
{
    sectionHeader("Linear Search  —  by Description");

    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_BG_WIDGET);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100);
    ImGui::InputText("##kw", state.searchKeyword, sizeof(state.searchKeyword));
    ImGui::PopStyleColor();
    ImGui::SameLine();
    pushBtnStyle(COL_BTN_PRIMARY);
    if (ImGui::Button("Search##l", ImVec2(-1, 0))) {
        state.linearResults = linearSearch(state.expenses, state.searchKeyword);
        state.linearDone = true;
    }
    popBtnStyle();

    if (state.linearDone) {
        ImGui::Spacing();
        if (state.linearResults.empty()) {
            ImGui::TextColored(COL_ERROR, "  No results found.");
        }
        else {
            ImGui::TextColored(COL_SUCCESS, "  Found: %d record(s)",
                (int)state.linearResults.size());
            ImGui::Spacing();
            for (int idx : state.linearResults) {
                const Expense& e = state.expenses[idx];
                ImVec2 cp = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    cp, ImVec2(cp.x + ImGui::GetContentRegionAvail().x,
                        cp.y + ImGui::GetTextLineHeightWithSpacing() + 2),
                    IM_COL32(25, 55, 110, 80), 3.0f);
                ImGui::TextColored(CAT_COLS[e.category], "  %-24s",
                    e.description.c_str());
                ImGui::SameLine();
                ImGui::TextColored(COL_ACCENT2, "$%-8.2f", e.amount);
                ImGui::SameLine();
                ImGui::TextColored(COL_MUTED, "%02d/%02d/%d",
                    e.month, e.day, e.year);
            }
        }
    }

    ImGui::Spacing();
    sectionHeader("Binary Search  —  by Exact Amount");

    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_BG_WIDGET);
    ImGui::SetNextItemWidth(150);
    ImGui::InputFloat("$##bs", &state.searchAmount, 0.01f, 1.0f, "%.2f");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    pushBtnStyle(COL_BTN_PRIMARY);
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
    popBtnStyle();

    if (state.binaryDone) {
        ImGui::Spacing();
        if (state.binaryResult < 0) {
            ImGui::TextColored(COL_ERROR,
                "  No expense with amount $%.2f.", state.searchAmount);
        }
        else {
            const Expense& e = state.expenses[state.binaryResult];
            ImGui::TextColored(COL_SUCCESS,
                "  Found: %s  $%.2f  (%s)  %02d/%02d/%d",
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
    ImDrawList* dl = ImGui::GetWindowDrawList();

    sectionHeader("Statistics");

    if (state.expenses.empty()) {
        ImGui::TextColored(COL_MUTED, "No expenses to analyse.");
        return;
    }

    double total = totalExpenses(state.expenses);
    double avg = avgExpense(state.expenses);
    double highest = maxExpense(state.expenses);
    double lowest = minExpense(state.expenses);

    // ── 4 summary cards ──────────────────────────────────────────────────────
    struct CardDef { const char* label; const char* icon; double val; ImU32 glowC; ImVec4 textC; };
    CardDef cards[4] = {
        { "Total Spent",  "$", total,   IM_COL32(50,120,255,60),  COL_ACCENT  },
        { "Average",      "~", avg,     IM_COL32(50,200,255,50),  COL_ACCENT2 },
        { "Highest",      "^", highest, IM_COL32(220,60,60,55),   COL_ERROR   },
        { "Lowest",       "v", lowest,  IM_COL32(40,200,100,55),  COL_SUCCESS },
    };

    float availW = ImGui::GetContentRegionAvail().x;
    float cardW = (availW - 12.0f) * 0.5f;
    float cardH = 62.0f;

    for (int ci = 0; ci < 4; ci++) {
        if (ci == 2) { /* second row - no SameLine */ }
        else if (ci == 1 || ci == 3) {} // SameLine called below

        ImVec2 cp = ImGui::GetCursorScreenPos();

        // Card background
        dl->AddRectFilled(cp, ImVec2(cp.x + cardW, cp.y + cardH),
            IM_COL32(16, 18, 28, 255), 8.0f);
        // Glow border
        dl->AddRect(cp, ImVec2(cp.x + cardW, cp.y + cardH),
            cards[ci].glowC, 8.0f, 0, 1.5f);
        // Top accent line
        ImU32 lineC = ImGui::ColorConvertFloat4ToU32(
            ImVec4(cards[ci].textC.x, cards[ci].textC.y, cards[ci].textC.z, 0.7f));
        dl->AddRectFilled(cp, ImVec2(cp.x + cardW, cp.y + 3), lineC, 8.0f);

        // Label
        dl->AddText(ImVec2(cp.x + 12, cp.y + 10),
            IM_COL32(120, 130, 150, 255), cards[ci].label);

        // Value
        char valStr[32]; snprintf(valStr, sizeof(valStr), "$%.2f", cards[ci].val);
        ImGui::SetWindowFontScale(1.18f);
        ImVec2 vs = ImGui::CalcTextSize(valStr);
        dl->AddText(ImVec2(cp.x + cardW - vs.x - 10, cp.y + cardH - vs.y - 10),
            ImGui::ColorConvertFloat4ToU32(cards[ci].textC), valStr);
        ImGui::SetWindowFontScale(1.0f);

        // Advance cursor
        ImGui::Dummy(ImVec2(cardW, cardH));
        if (ci == 0 || ci == 2) { ImGui::SameLine(0, 12); }
    }

    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "  %d records", (int)state.expenses.size());
    ImGui::Spacing();

    // ── Category breakdown with custom bars ──────────────────────────────────
    sectionHeader("By Category");
    double catTot[CAT_COUNT] = {};
    categoryTotals(state.expenses, catTot);
    double grandTotal = total > 0.0 ? total : 1.0;
    float  barW = ImGui::GetContentRegionAvail().x - 130.0f;

    for (int i = 0; i < CAT_COUNT; i++) {
        if (catTot[i] < 0.001) continue;
        float frac = (float)(catTot[i] / grandTotal);
        int   cnt = recursiveCountByCategory(
            state.expenses, (int)state.expenses.size() - 1, (Category)i);

        ImVec2 rowP = ImGui::GetCursorScreenPos();

        // Dot
        dl->AddCircleFilled(ImVec2(rowP.x + 6, rowP.y + 9), 5.0f,
            ImGui::ColorConvertFloat4ToU32(CAT_COLS[i]), 12);

        // Name
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15);
        ImGui::TextColored(CAT_COLS[i], "%-13s", CAT_NAMES[i]);
        ImGui::SameLine(0, 6);

        // Custom bar
        ImVec2 barP = ImGui::GetCursorScreenPos();
        float  bh = 14.0f;
        // Track
        dl->AddRectFilled(barP, ImVec2(barP.x + barW, barP.y + bh),
            IM_COL32(20, 22, 32, 220), 3.0f);
        // Fill with gradient
        ImU32 fillL = ImGui::ColorConvertFloat4ToU32(CAT_COLS[i]);
        ImU32 fillR = IM_COL32(
            (int)(CAT_COLS[i].x * 180), (int)(CAT_COLS[i].y * 180),
            (int)(CAT_COLS[i].z * 180), 200);
        dl->AddRectFilledMultiColor(barP,
            ImVec2(barP.x + barW * frac, barP.y + bh), fillL, fillR, fillR, fillL);

        // Label
        char lbl[32]; snprintf(lbl, sizeof(lbl), "$%.0f (%d)", catTot[i], cnt);
        dl->AddText(ImVec2(barP.x + barW + 6, barP.y),
            IM_COL32(160, 170, 190, 255), lbl);

        ImGui::Dummy(ImVec2(barW + 70, bh + 4));
    }

    // ── Monthly bars ─────────────────────────────────────────────────────────
    ImGui::Spacing();
    sectionHeader("Monthly Breakdown");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_BG_WIDGET);
    ImGui::SetNextItemWidth(90);
    ImGui::InputInt("Year##sy", &state.statsYear, 1, 10);
    ImGui::PopStyleColor();
    ImGui::Spacing();

    double mTot[13] = {};
    for (int m = 1; m <= 12; m++)
        mTot[m] = recursiveMonthlyTotal(
            state.expenses, (int)state.expenses.size() - 1, m, state.statsYear);
    double mMax = 0.0;
    for (int m = 1; m <= 12; m++) if (mTot[m] > mMax) mMax = mTot[m];

    bool any = false;
    float mBarW = ImGui::GetContentRegionAvail().x - 110.0f;
    for (int m = 1; m <= 12; m++) {
        if (mTot[m] < 0.001) continue;
        any = true;
        float frac = (mMax > 0.0) ? (float)(mTot[m] / mMax) : 0.0f;

        ImGui::TextColored(COL_ACCENT, "%-4s", MONTHS[m]);
        ImGui::SameLine(0, 6);

        ImVec2 bp = ImGui::GetCursorScreenPos();
        float  bh = 14.0f;
        dl->AddRectFilled(bp, ImVec2(bp.x + mBarW, bp.y + bh), IM_COL32(20, 22, 32, 220), 3.0f);
        dl->AddRectFilledMultiColor(bp, ImVec2(bp.x + mBarW * frac, bp.y + bh),
            IM_COL32(50, 120, 255, 200), IM_COL32(40, 200, 180, 160),
            IM_COL32(40, 200, 180, 160), IM_COL32(50, 120, 255, 200));

        char lbl[32]; snprintf(lbl, sizeof(lbl), "$%.0f", mTot[m]);
        dl->AddText(ImVec2(bp.x + mBarW + 6, bp.y), IM_COL32(140, 160, 200, 255), lbl);
        ImGui::Dummy(ImVec2(mBarW + 50, bh + 4));
    }
    if (!any) ImGui::TextColored(COL_MUTED, "No expenses in %d.", state.statsYear);
}

// ────────────────────────────────────────────────────────────────────────────
//  renderBudgetPanel
// ────────────────────────────────────────────────────────────────────────────
void renderBudgetPanel(AppState& state)
{
    sectionHeader("Monthly Budget");

    ImGui::TextColored(COL_MUTED,
        "Only expenses marked as Done count toward budget.");
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_BG_WIDGET);
    int bm = state.budgetMonth - 1;
    ImGui::SetNextItemWidth(115);
    ImGui::Combo("##bm", &bm, MONTHS + 1, 12);
    state.budgetMonth = bm + 1;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(88);
    ImGui::InputInt("##by", &state.budgetYear, 1, 10);
    ImGui::PopStyleColor();

    double used = getBudgetUsed(state.allExpenses, state.loggedInUser,
        state.budgetMonth, state.budgetYear);
    double limit = getBudget(state.budgets, state.loggedInUser,
        state.budgetMonth, state.budgetYear);

    ImGui::Spacing();

    if (limit > 0.0) {
        float frac = (float)(used / limit);
        if (frac > 1.0f) frac = 1.0f;

        bool over = (used >= limit);
        bool close = (!over && used >= limit * 0.9);
        ImVec4 barC = over ? COL_ERROR : close ? COL_WARNING : COL_SUCCESS;

        // Animated glow on the bar when over budget
        if (over) {
            float gp = pulse(2.5f, 0.5f);
            barC = lerpColor(COL_ERROR, ImVec4(1, 0.7f, 0.7f, 1), gp);
        }

        ImGui::Text("Spent (Done): $%.2f  /  Limit: $%.2f  (%.1f%%)",
            used, limit, used / limit * 100.0);
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barC);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.15f, 0.20f, 1.0f));
        ImGui::ProgressBar(frac, ImVec2(-1, 24));
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        if (over)
            ImGui::TextColored(COL_ERROR,
                "!! Budget exceeded by $%.2f !!", used - limit);
        else if (close)
            ImGui::TextColored(COL_WARNING,
                "! %.1f%% used - $%.2f remaining",
                used / limit * 100.0, limit - used);
        else
            ImGui::TextColored(COL_SUCCESS,
                "%.1f%% used - $%.2f remaining",
                used / limit * 100.0, limit - used);
    }
    else {
        ImGui::TextColored(COL_MUTED, "No budget set for %s %d.",
            MONTHS[state.budgetMonth], state.budgetYear);
    }

    ImGui::Spacing();
    accentLine();
    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "Set new limit ($):");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, COL_BG_WIDGET);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputFloat("##bl", &state.budgetLimit, 1.0f, 10.0f, "%.2f");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    pushBtnStyle(COL_BTN_PRIMARY);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Button("  Set Budget  ", ImVec2(-1, 34))) {
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
    ImGui::PopStyleVar();
    popBtnStyle();
}

// ────────────────────────────────────────────────────────────────────────────
//  renderNotifCenter
// ────────────────────────────────────────────────────────────────────────────
void renderNotifCenter(AppState& state)
{
    if (!state.showNotifCenter) return;

    ImGui::SetNextWindowSize(ImVec2(400, 320), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_BG_PANEL);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.45f, 0.80f, 0.40f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::Begin("Notification Center", &state.showNotifCenter,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleVar();

    pushBtnStyle(COL_BTN_NEUTRAL);
    if (ImGui::SmallButton(" Mark all read "))
        for (auto& n : state.notifications) n.read = true;
    ImGui::SameLine();
    pushBtnStyle(COL_BTN_DANGER);
    if (ImGui::SmallButton(" Clear all "))
        state.notifications.clear();
    popBtnStyle(); popBtnStyle();  // two popBtnStyle for two pushBtnStyle

    accentLine();

    if (state.notifications.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(COL_MUTED, "  No notifications.");
    }