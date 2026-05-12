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
    const char* title = "Spendora";
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