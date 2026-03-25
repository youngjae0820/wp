#define UNICODE
#define _UNICODE
#include <windows.h>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <algorithm>

#pragma comment(lib, "Msimg32.lib")

// ============================================================
// Ruina-style Battle Demo (Game-like layout, image-less)
// WinAPI + GDI single file
// ------------------------------------------------------------
// 목표:
// - 이미지 없이도 실제 게임 모드처럼 보이는 전투 화면 배치
// - 중앙 전장 / 좌우 진영 / 상단 상태바 / 우측 계획 패널 / 하단 손패 / 좌하단 로그
// - 카드 선택 -> 대상 지정 -> 행동 예약 -> 턴 실행 -> 합(클래시)
// ============================================================

/*
[슈드코드]
1. 윈도우 생성, 타이머 시작, 전투 초기화.
2. 매 프레임 더블 버퍼링으로 화면 전체 렌더링.
3. 플레이어는 하단 손패에서 카드를 선택.
4. 적 유닛을 클릭하면 현재 선택된 플레이어의 행동 예약.
5. 모든 플레이어가 예약되면 턴 실행.
6. 턴 실행 시 속도값 기준 정렬 후 행동 처리.
7. 서로를 겨냥한 공격 카드끼리는 합 처리.
8. 결과를 로그, 체력, 흐트러짐, 부유 텍스트로 반영.
9. 승패 시 오버레이 표시. R로 재시작.
*/

// ------------------------------
// constants
// ------------------------------
const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;
const UINT_PTR TIMER_ID = 1;
const UINT TIMER_INTERVAL = 16;

const int HAND_CARD_W = 165;
const int HAND_CARD_H = 210;
const int HAND_START_X = 240;
const int HAND_Y = 675;
const int HAND_GAP = 14;

const int PANEL_RIGHT_X = 1240;
const int PANEL_RIGHT_Y = 90;
const int PANEL_RIGHT_W = 320;
const int PANEL_RIGHT_H = 760;

const int LOG_X = 28;
const int LOG_Y = 560;
const int LOG_W = 500;
const int LOG_H = 290;

const int ENDTURN_X = 1325;
const int ENDTURN_Y = 760;
const int ENDTURN_W = 150;
const int ENDTURN_H = 56;

// ------------------------------
// utility
// ------------------------------
int RandInt(int a, int b)
{
    if (a > b)
    {
        int t = a; a = b; b = t;
    }
    return a + rand() % (b - a + 1);
}

std::wstring ToWString(int v)
{
    wchar_t buf[64] = { 0, };
    wsprintf(buf, L"%d", v);
    return buf;
}

bool PtInRectSafe(const RECT& rc, int x, int y)
{
    POINT pt = { x, y };
    return PtInRect(&rc, pt) ? true : false;
}

int ClampInt(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ------------------------------
// data
// ------------------------------
enum class DiceType
{
    ATTACK,
    DEFEND,
    EVADE
};

enum class Team
{
    PLAYER,
    ENEMY
};

struct Dice
{
    DiceType type;
    int minValue;
    int maxValue;
};

struct Card
{
    std::wstring name;
    int cost;
    std::wstring desc;
    std::vector<Dice> dices;
};

struct Action
{
    int actorIndex;
    int targetIndex;
    int handCardIndex;
    int speed;
    bool valid;
    Action() : actorIndex(-1), targetIndex(-1), handCardIndex(-1), speed(0), valid(false) {}
};

struct Unit
{
    std::wstring name;
    Team team;
    int hp, maxHp;
    int stagger, maxStagger;
    int light, maxLight;
    int speed;
    bool alive;
    bool staggered;
    RECT rect;
    std::vector<Card> deck;
    std::vector<Card> hand;
    Action action;

    Unit()
    {
        team = Team::PLAYER;
        hp = maxHp = 30;
        stagger = maxStagger = 20;
        light = maxLight = 3;
        speed = 1;
        alive = true;
        staggered = false;
        rect = { 0,0,0,0 };
    }
};

struct FloatingText
{
    int x;
    int y;
    int value;
    int lifetime;
    bool isDamage;
};

// ------------------------------
// globals
// ------------------------------
HWND g_hWnd = NULL;
std::vector<Unit> g_units;
std::vector<std::wstring> g_logs;
std::vector<FloatingText> g_floatingTexts;

int g_selectedPlayer = 0;
int g_selectedHandIndex = -1;
bool g_battleEnded = false;
std::wstring g_endMessage;
RECT g_handRects[8] = {};
RECT g_endTurnButton = { ENDTURN_X, ENDTURN_Y, ENDTURN_X + ENDTURN_W, ENDTURN_Y + ENDTURN_H };

// ------------------------------
// helpers
// ------------------------------
void PushLog(const std::wstring& s)
{
    g_logs.push_back(s);
    if (g_logs.size() > 11)
        g_logs.erase(g_logs.begin());
}

void AddFloatingText(int x, int y, int value, bool isDamage)
{
    FloatingText ft;
    ft.x = x;
    ft.y = y;
    ft.value = value;
    ft.lifetime = 42;
    ft.isDamage = isDamage;
    g_floatingTexts.push_back(ft);
}

std::wstring DiceTypeText(DiceType t)
{
    if (t == DiceType::ATTACK) return L"ATK";
    if (t == DiceType::DEFEND) return L"DEF";
    return L"EVA";
}

int RollDice(const Dice& d)
{
    return RandInt(d.minValue, d.maxValue);
}

int UnitCenterX(const Unit& u)
{
    return (u.rect.left + u.rect.right) / 2;
}

int UnitCenterY(const Unit& u)
{
    return (u.rect.top + u.rect.bottom) / 2;
}

bool IsPlayerIndex(int idx)
{
    return idx >= 0 && idx < (int)g_units.size() && g_units[idx].team == Team::PLAYER;
}

bool IsEnemyIndex(int idx)
{
    return idx >= 0 && idx < (int)g_units.size() && g_units[idx].team == Team::ENEMY;
}

std::vector<int> GetAliveTeam(Team t)
{
    std::vector<int> result;
    for (int i = 0; i < (int)g_units.size(); ++i)
        if (g_units[i].team == t && g_units[i].alive)
            result.push_back(i);
    return result;
}

int FindUnitAt(int x, int y)
{
    for (int i = 0; i < (int)g_units.size(); ++i)
        if (PtInRectSafe(g_units[i].rect, x, y))
            return i;
    return -1;
}

void UpdateAlive(Unit& u)
{
    if (u.hp <= 0)
    {
        u.hp = 0;
        u.alive = false;
        u.action = Action();
    }
    else
    {
        u.alive = true;
    }

    if (u.stagger <= 0)
    {
        u.stagger = 0;
        u.staggered = true;
    }
    else
    {
        u.staggered = false;
    }
}

void RefillLight(Unit& u)
{
    u.light = min(u.maxLight, u.light + 1);
}

void DrawUntil(Unit& u, int n)
{
    while ((int)u.hand.size() < n && !u.deck.empty())
    {
        u.hand.push_back(u.deck.back());
        u.deck.pop_back();
    }
}

void ShuffleDeck(Unit& u)
{
    std::random_shuffle(u.deck.begin(), u.deck.end());
}

// ------------------------------
// cards
// ------------------------------
Card MakeStrike()
{
    Card c;
    c.name = L"Strike";
    c.cost = 1;
    c.desc = L"단일 공격 2~6";
    c.dices.push_back({ DiceType::ATTACK, 2, 6 });
    return c;
}

Card MakeHeavySlash()
{
    Card c;
    c.name = L"Heavy Slash";
    c.cost = 2;
    c.desc = L"강한 공격 4~9";
    c.dices.push_back({ DiceType::ATTACK, 4, 9 });
    return c;
}

Card MakeGuard()
{
    Card c;
    c.name = L"Guard";
    c.cost = 1;
    c.desc = L"방어 2~7";
    c.dices.push_back({ DiceType::DEFEND, 2, 7 });
    return c;
}

Card MakeEvade()
{
    Card c;
    c.name = L"Sidestep";
    c.cost = 1;
    c.desc = L"회피 3~8";
    c.dices.push_back({ DiceType::EVADE, 3, 8 });
    return c;
}

Card MakeTwinCut()
{
    Card c;
    c.name = L"Twin Cut";
    c.cost = 2;
    c.desc = L"2연격 2~5 / 2~5";
    c.dices.push_back({ DiceType::ATTACK, 2, 5 });
    c.dices.push_back({ DiceType::ATTACK, 2, 5 });
    return c;
}

Card MakeEnemyBite()
{
    Card c;
    c.name = L"Bite";
    c.cost = 1;
    c.desc = L"공격 2~5";
    c.dices.push_back({ DiceType::ATTACK, 2, 5 });
    return c;
}

Card MakeEnemyShell()
{
    Card c;
    c.name = L"Shell";
    c.cost = 1;
    c.desc = L"방어 1~6";
    c.dices.push_back({ DiceType::DEFEND, 1, 6 });
    return c;
}

Card MakeEnemyRush()
{
    Card c;
    c.name = L"Rush";
    c.cost = 2;
    c.desc = L"공격 3~8";
    c.dices.push_back({ DiceType::ATTACK, 3, 8 });
    return c;
}

void SetupDeck(Unit& u)
{
    u.deck.clear();
    u.hand.clear();

    if (u.team == Team::PLAYER)
    {
        u.deck.push_back(MakeStrike());
        u.deck.push_back(MakeStrike());
        u.deck.push_back(MakeHeavySlash());
        u.deck.push_back(MakeGuard());
        u.deck.push_back(MakeEvade());
        u.deck.push_back(MakeTwinCut());
        u.deck.push_back(MakeStrike());
        u.deck.push_back(MakeGuard());
    }
    else
    {
        u.deck.push_back(MakeEnemyBite());
        u.deck.push_back(MakeEnemyShell());
        u.deck.push_back(MakeEnemyRush());
        u.deck.push_back(MakeEnemyBite());
        u.deck.push_back(MakeEnemyShell());
        u.deck.push_back(MakeEnemyBite());
    }

    ShuffleDeck(u);
    DrawUntil(u, 4);
}

// ------------------------------
// battle init / turn
// ------------------------------
void PrepareNewTurn()
{
    for (auto& u : g_units)
    {
        if (!u.alive) continue;
        RefillLight(u);
        u.speed = RandInt(1, 6);
        u.action = Action();
        u.stagger = min(u.maxStagger, u.stagger + 1);
        if (u.deck.empty()) SetupDeck(u);
        DrawUntil(u, 4);
    }

    g_selectedHandIndex = -1;
    for (int i = 0; i < (int)g_units.size(); ++i)
    {
        if (g_units[i].team == Team::PLAYER && g_units[i].alive)
        {
            g_selectedPlayer = i;
            break;
        }
    }
    PushLog(L"새 턴이 시작되었습니다.");
}

void ResetBattle()
{
    g_units.clear();
    g_logs.clear();
    g_floatingTexts.clear();
    g_selectedPlayer = 0;
    g_selectedHandIndex = -1;
    g_battleEnded = false;
    g_endMessage.clear();

    Unit p1;
    p1.name = L"Roland";
    p1.team = Team::PLAYER;
    p1.maxHp = p1.hp = 38;
    p1.maxStagger = p1.stagger = 24;
    p1.maxLight = p1.light = 3;
    p1.rect = { 250, 205, 420, 470 };
    SetupDeck(p1);

    Unit p2;
    p2.name = L"Angela";
    p2.team = Team::PLAYER;
    p2.maxHp = p2.hp = 34;
    p2.maxStagger = p2.stagger = 22;
    p2.maxLight = p2.light = 3;
    p2.rect = { 470, 155, 640, 420 };
    SetupDeck(p2);

    Unit e1;
    e1.name = L"Fixer A";
    e1.team = Team::ENEMY;
    e1.maxHp = e1.hp = 32;
    e1.maxStagger = e1.stagger = 18;
    e1.maxLight = e1.light = 3;
    e1.rect = { 915, 155, 1085, 420 };
    SetupDeck(e1);

    Unit e2;
    e2.name = L"Fixer B";
    e2.team = Team::ENEMY;
    e2.maxHp = e2.hp = 28;
    e2.maxStagger = e2.stagger = 20;
    e2.maxLight = e2.light = 3;
    e2.rect = { 1135, 205, 1305, 470 };
    SetupDeck(e2);

    g_units.push_back(p1);
    g_units.push_back(p2);
    g_units.push_back(e1);
    g_units.push_back(e2);

    PushLog(L"접대 전투 시작.");
    PushLog(L"카드를 선택한 뒤 적을 클릭하면 행동이 예약됩니다.");
    PushLog(L"모든 아군이 행동을 예약하면 자동으로 턴이 진행됩니다.");
    PrepareNewTurn();
}

bool IsBattleFinished()
{
    bool playerAlive = false;
    bool enemyAlive = false;
    for (const auto& u : g_units)
    {
        if (!u.alive) continue;
        if (u.team == Team::PLAYER) playerAlive = true;
        if (u.team == Team::ENEMY) enemyAlive = true;
    }

    if (!playerAlive)
    {
        g_battleEnded = true;
        g_endMessage = L"패배";
        return true;
    }
    if (!enemyAlive)
    {
        g_battleEnded = true;
        g_endMessage = L"승리";
        return true;
    }
    return false;
}

bool AllPlayersAssigned()
{
    for (const auto& u : g_units)
        if (u.team == Team::PLAYER && u.alive && !u.action.valid)
            return false;
    return true;
}

void AssignEnemyActions()
{
    std::vector<int> alivePlayers = GetAliveTeam(Team::PLAYER);
    if (alivePlayers.empty()) return;

    for (int i = 0; i < (int)g_units.size(); ++i)
    {
        Unit& e = g_units[i];
        if (!e.alive || e.team != Team::ENEMY) continue;

        int selected = -1;
        for (int h = 0; h < (int)e.hand.size(); ++h)
        {
            if (e.hand[h].cost <= e.light)
            {
                selected = h;
                break;
            }
        }
        if (selected == -1) continue;

        int target = alivePlayers[RandInt(0, (int)alivePlayers.size() - 1)];
        e.action.actorIndex = i;
        e.action.targetIndex = target;
        e.action.handCardIndex = selected;
        e.action.speed = e.speed;
        e.action.valid = true;
    }
}

void ApplyDamage(Unit& target, int dmg)
{
    if (!target.alive) return;
    dmg = max(0, dmg);
    target.hp -= dmg;
    target.stagger -= dmg / 2 + 1;
    UpdateAlive(target);
}

void ResolveSingleDie(Unit& actor, Unit& target, const Dice& d)
{
    if (!actor.alive || !target.alive) return;

    int roll = RollDice(d);
    PushLog(actor.name + L" " + DiceTypeText(d.type) + L" [" + ToWString(roll) + L"]");

    if (d.type == DiceType::ATTACK)
    {
        ApplyDamage(target, roll);
        AddFloatingText(UnitCenterX(target), UnitCenterY(target), roll, true);
        PushLog(target.name + L" 피해 " + ToWString(roll));
    }
    else if (d.type == DiceType::DEFEND)
    {
        actor.stagger = min(actor.maxStagger, actor.stagger + roll / 2 + 1);
        AddFloatingText(UnitCenterX(actor), UnitCenterY(actor), roll, false);
    }
    else
    {
        actor.stagger = min(actor.maxStagger, actor.stagger + roll / 3 + 1);
        AddFloatingText(UnitCenterX(actor), UnitCenterY(actor), roll, false);
    }

    UpdateAlive(actor);
    UpdateAlive(target);
}

void ResolveClash(Unit& actor, Unit& target, const Card& actorCard, const Card& targetCard)
{
    PushLog(L"=== 합 시작 ===");
    size_t count = min(actorCard.dices.size(), targetCard.dices.size());

    for (size_t i = 0; i < count; ++i)
    {
        if (!actor.alive || !target.alive) break;

        int a = RollDice(actorCard.dices[i]);
        int b = RollDice(targetCard.dices[i]);
        PushLog(actor.name + L"(" + DiceTypeText(actorCard.dices[i].type) + L":" + ToWString(a) + L") vs " +
            target.name + L"(" + DiceTypeText(targetCard.dices[i].type) + L":" + ToWString(b) + L")");

        if (a > b)
        {
            int dmg = (actorCard.dices[i].type == DiceType::ATTACK) ? (a - b + 1) : 0;
            if (dmg > 0)
            {
                ApplyDamage(target, dmg);
                AddFloatingText(UnitCenterX(target), UnitCenterY(target), dmg, true);
                PushLog(actor.name + L" 합 승리, 피해 " + ToWString(dmg));
            }
            else
            {
                PushLog(actor.name + L" 합 승리");
            }
        }
        else if (b > a)
        {
            int dmg = (targetCard.dices[i].type == DiceType::ATTACK) ? (b - a + 1) : 0;
            if (dmg > 0)
            {
                ApplyDamage(actor, dmg);
                AddFloatingText(UnitCenterX(actor), UnitCenterY(actor), dmg, true);
                PushLog(target.name + L" 합 승리, 피해 " + ToWString(dmg));
            }
            else
            {
                PushLog(target.name + L" 합 승리");
            }
        }
        else
        {
            PushLog(L"합 무승부");
        }

        UpdateAlive(actor);
        UpdateAlive(target);
    }

    if (actor.alive && target.alive)
    {
        for (size_t i = count; i < actorCard.dices.size(); ++i)
            ResolveSingleDie(actor, target, actorCard.dices[i]);
        for (size_t i = count; i < targetCard.dices.size(); ++i)
            ResolveSingleDie(target, actor, targetCard.dices[i]);
    }

    PushLog(L"=== 합 종료 ===");
}

void ExecuteAction(Action act)
{
    if (!act.valid) return;
    if (act.actorIndex < 0 || act.actorIndex >= (int)g_units.size()) return;
    if (act.targetIndex < 0 || act.targetIndex >= (int)g_units.size()) return;

    Unit& actor = g_units[act.actorIndex];
    Unit& target = g_units[act.targetIndex];
    if (!actor.alive || !target.alive) return;
    if (act.handCardIndex < 0 || act.handCardIndex >= (int)actor.hand.size()) return;

    Card actorCard = actor.hand[act.handCardIndex];
    if (actorCard.cost > actor.light)
    {
        PushLog(actor.name + L" 빛 부족");
        return;
    }

    int opposingIndex = -1;
    for (int i = 0; i < (int)g_units.size(); ++i)
    {
        if (i == act.actorIndex) continue;
        const Unit& other = g_units[i];
        if (!other.alive || !other.action.valid) continue;
        if (other.action.actorIndex == act.targetIndex && other.action.targetIndex == act.actorIndex)
        {
            opposingIndex = i;
            break;
        }
    }

    actor.light -= actorCard.cost;
    actor.hand.erase(actor.hand.begin() + act.handCardIndex);

    if (opposingIndex != -1)
    {
        Unit& other = g_units[opposingIndex];
        if (other.action.valid && other.action.handCardIndex >= 0 && other.action.handCardIndex < (int)other.hand.size())
        {
            Card otherCard = other.hand[other.action.handCardIndex];
            if (otherCard.cost <= other.light)
            {
                other.light -= otherCard.cost;
                other.hand.erase(other.hand.begin() + other.action.handCardIndex);
                other.action.valid = false;
                ResolveClash(actor, target, actorCard, otherCard);
                return;
            }
        }
    }

    PushLog(actor.name + L" -> " + target.name + L" : " + actorCard.name);
    for (const auto& d : actorCard.dices)
    {
        if (!target.alive) break;
        ResolveSingleDie(actor, target, d);
    }
}

void RunTurn()
{
    if (g_battleEnded) return;

    AssignEnemyActions();

    std::vector<Action> order;
    for (const auto& u : g_units)
        if (u.alive && u.action.valid)
            order.push_back(u.action);

    std::sort(order.begin(), order.end(), [](const Action& a, const Action& b)
        {
            if (a.speed != b.speed) return a.speed > b.speed;
            return a.actorIndex < b.actorIndex;
        });

    PushLog(L"------ 턴 진행 ------");
    for (auto act : order)
    {
        if (act.actorIndex < 0 || act.actorIndex >= (int)g_units.size()) continue;
        if (!g_units[act.actorIndex].action.valid) continue;
        g_units[act.actorIndex].action.valid = false;
        ExecuteAction(act);
        if (IsBattleFinished()) return;
    }

    if (!IsBattleFinished())
        PrepareNewTurn();
}

void AssignPlayerAction(int targetIndex)
{
    if (g_battleEnded) return;
    if (!IsPlayerIndex(g_selectedPlayer)) return;
    if (!IsEnemyIndex(targetIndex)) return;
    if (g_selectedHandIndex < 0) return;

    Unit& p = g_units[g_selectedPlayer];
    if (!p.alive) return;
    if (g_selectedHandIndex >= (int)p.hand.size()) return;
    if (p.hand[g_selectedHandIndex].cost > p.light)
    {
        PushLog(L"빛이 부족합니다.");
        return;
    }

    p.action.actorIndex = g_selectedPlayer;
    p.action.targetIndex = targetIndex;
    p.action.handCardIndex = g_selectedHandIndex;
    p.action.speed = p.speed;
    p.action.valid = true;

    PushLog(p.name + L" 예약: " + p.hand[g_selectedHandIndex].name + L" -> " + g_units[targetIndex].name);
    g_selectedHandIndex = -1;

    for (int i = 0; i < (int)g_units.size(); ++i)
    {
        if (g_units[i].team == Team::PLAYER && g_units[i].alive && !g_units[i].action.valid)
        {
            g_selectedPlayer = i;
            return;
        }
    }

    if (AllPlayersAssigned())
        RunTurn();
}

// ------------------------------
// drawing helpers
// ------------------------------
void FillRoundRect(HDC hdc, const RECT& rc, COLORREF fill, COLORREF border, int penWidth, int radius)
{
    HBRUSH b = CreateSolidBrush(fill);
    HPEN p = CreatePen(PS_SOLID, penWidth, border);
    HGDIOBJ oldB = SelectObject(hdc, b);
    HGDIOBJ oldP = SelectObject(hdc, p);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
    SelectObject(hdc, oldB);
    SelectObject(hdc, oldP);
    DeleteObject(b);
    DeleteObject(p);
}

void DrawTextCenter(HDC hdc, const RECT& rc, const std::wstring& s)
{
    RECT t = rc;
    DrawText(hdc, s.c_str(), -1, &t, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawShadowRect(HDC hdc, const RECT& rc, COLORREF shadow)
{
    RECT s = { rc.left + 6, rc.top + 6, rc.right + 6, rc.bottom + 6 };
    HBRUSH b = CreateSolidBrush(shadow);
    FillRect(hdc, &s, b);
    DeleteObject(b);
}

void DrawBar(HDC hdc, int x, int y, int w, int h, int value, int maxValue, COLORREF fillColor, COLORREF backColor)
{
    RECT back = { x, y, x + w, y + h };
    HBRUSH bb = CreateSolidBrush(backColor);
    FillRect(hdc, &back, bb);
    DeleteObject(bb);

    FrameRect(hdc, &back, (HBRUSH)GetStockObject(BLACK_BRUSH));
    if (maxValue <= 0) return;

    int fw = (w - 2) * ClampInt(value, 0, maxValue) / maxValue;
    RECT fr = { x + 1, y + 1, x + 1 + fw, y + h - 1 };
    HBRUSH fb = CreateSolidBrush(fillColor);
    FillRect(hdc, &fr, fb);
    DeleteObject(fb);
}

COLORREF UnitFillColor(const Unit& u)
{
    if (!u.alive) return RGB(55, 55, 60);
    return (u.team == Team::PLAYER) ? RGB(66, 89, 148) : RGB(140, 68, 72);
}

void DrawBattleBackground(HDC hdc)
{
    RECT full = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };
    HBRUSH bg = CreateSolidBrush(RGB(23, 25, 31));
    FillRect(hdc, &full, bg);
    DeleteObject(bg);

    RECT stage = { 120, 90, 1210, 530 };
    DrawShadowRect(hdc, stage, RGB(15, 15, 18));
    FillRoundRect(hdc, stage, RGB(44, 40, 44), RGB(90, 84, 90), 1, 24);

    RECT centerLine = { 655, 105, 675, 515 };
    HBRUSH lineBrush = CreateSolidBrush(RGB(78, 72, 78));
    FillRect(hdc, &centerLine, lineBrush);
    DeleteObject(lineBrush);

    RECT handPanel = { 180, 630, 1200, 875 };
    DrawShadowRect(hdc, handPanel, RGB(12, 12, 16));
    FillRoundRect(hdc, handPanel, RGB(28, 30, 38), RGB(76, 82, 96), 1, 18);

    RECT topBar = { 20, 18, 1580, 72 };
    FillRoundRect(hdc, topBar, RGB(31, 34, 42), RGB(90, 94, 110), 1, 14);

    RECT rightPanel = { PANEL_RIGHT_X, PANEL_RIGHT_Y, PANEL_RIGHT_X + PANEL_RIGHT_W, PANEL_RIGHT_Y + PANEL_RIGHT_H };
    DrawShadowRect(hdc, rightPanel, RGB(12, 12, 16));
    FillRoundRect(hdc, rightPanel, RGB(28, 30, 38), RGB(76, 82, 96), 1, 18);

    RECT logPanel = { LOG_X, LOG_Y, LOG_X + LOG_W, LOG_Y + LOG_H };
    DrawShadowRect(hdc, logPanel, RGB(12, 12, 16));
    FillRoundRect(hdc, logPanel, RGB(17, 18, 24), RGB(80, 80, 92), 1, 14);
}

void DrawUnit(HDC hdc, const Unit& u, bool selected)
{
    RECT shadow = { u.rect.left + 8, u.rect.top + 8, u.rect.right + 8, u.rect.bottom + 8 };
    HBRUSH sb = CreateSolidBrush(RGB(20, 20, 24));
    FillRect(hdc, &shadow, sb);
    DeleteObject(sb);

    FillRoundRect(hdc, u.rect, UnitFillColor(u), selected ? RGB(255, 220, 90) : RGB(235, 235, 240), selected ? 4 : 1, 22);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));

    RECT nameRc = { u.rect.left + 8, u.rect.top + 10, u.rect.right - 8, u.rect.top + 42 };
    DrawTextCenter(hdc, nameRc, u.name);

    RECT roleRc = { u.rect.left + 8, u.rect.top + 42, u.rect.right - 8, u.rect.top + 68 };
    DrawTextCenter(hdc, roleRc, u.team == Team::PLAYER ? L"Librarian" : L"Guest");

    DrawBar(hdc, u.rect.left + 14, u.rect.bottom - 86, (u.rect.right - u.rect.left) - 28, 16, u.hp, u.maxHp, RGB(196, 64, 72), RGB(66, 34, 38));
    DrawBar(hdc, u.rect.left + 14, u.rect.bottom - 58, (u.rect.right - u.rect.left) - 28, 16, u.stagger, u.maxStagger, RGB(224, 192, 82), RGB(76, 64, 30));

    RECT stat1 = { u.rect.left + 16, u.rect.bottom - 110, u.rect.right - 16, u.rect.bottom - 90 };
    RECT stat2 = { u.rect.left + 16, u.rect.bottom - 30, u.rect.right - 16, u.rect.bottom - 10 };
    std::wstring hpText = L"HP " + ToWString(u.hp) + L" / " + ToWString(u.maxHp);
    std::wstring spText = L"SPD " + ToWString(u.speed) + L"   LIGHT " + ToWString(u.light);
    DrawText(hdc, hpText.c_str(), -1, &stat1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawText(hdc, spText.c_str(), -1, &stat2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    if (u.action.valid)
    {
        RECT actRc = { u.rect.left + 20, u.rect.top + 82, u.rect.right - 20, u.rect.top + 112 };
        FillRoundRect(hdc, actRc, RGB(48, 92, 60), RGB(180, 230, 180), 1, 14);
        SetTextColor(hdc, RGB(235, 255, 235));
        DrawTextCenter(hdc, actRc, L"ACTION READY");
    }

    if (u.staggered)
    {
        RECT stRc = { u.rect.left + 20, u.rect.top + 120, u.rect.right - 20, u.rect.top + 150 };
        FillRoundRect(hdc, stRc, RGB(110, 80, 20), RGB(240, 210, 110), 1, 14);
        SetTextColor(hdc, RGB(255, 248, 220));
        DrawTextCenter(hdc, stRc, L"STAGGERED");
    }
}

void DrawCard(HDC hdc, const RECT& rc, const Card& c, bool selected, bool enabled)
{
    COLORREF fill = enabled ? RGB(230, 232, 238) : RGB(132, 136, 145);
    COLORREF border = selected ? RGB(255, 214, 85) : RGB(40, 42, 48);

    DrawShadowRect(hdc, rc, RGB(12, 12, 16));
    FillRoundRect(hdc, rc, fill, border, selected ? 4 : 1, 18);

    RECT band = { rc.left, rc.top, rc.right, rc.top + 36 };
    FillRoundRect(hdc, band, RGB(58, 62, 74), border, 1, 18);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    RECT nameRc = { rc.left + 12, rc.top + 6, rc.right - 12, rc.top + 34 };
    DrawText(hdc, c.name.c_str(), -1, &nameRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    RECT costCircle = { rc.right - 46, rc.top + 44, rc.right - 14, rc.top + 76 };
    HBRUSH cb = CreateSolidBrush(RGB(72, 105, 188));
    HPEN cp = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HGDIOBJ oldB = SelectObject(hdc, cb);
    HGDIOBJ oldP = SelectObject(hdc, cp);
    Ellipse(hdc, costCircle.left, costCircle.top, costCircle.right, costCircle.bottom);
    SelectObject(hdc, oldB);
    SelectObject(hdc, oldP);
    DeleteObject(cb);
    DeleteObject(cp);

    SetTextColor(hdc, RGB(255, 255, 255));
    DrawTextCenter(hdc, costCircle, ToWString(c.cost));

    SetTextColor(hdc, RGB(28, 30, 34));
    RECT typeRc = { rc.left + 12, rc.top + 50, rc.right - 60, rc.top + 76 };
    if (!c.dices.empty())
    {
        std::wstring typeText = DiceTypeText(c.dices[0].type);
        DrawText(hdc, typeText.c_str(), -1, &typeRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    RECT descRc = { rc.left + 12, rc.top + 88, rc.right - 12, rc.bottom - 12 };
    DrawText(hdc, c.desc.c_str(), -1, &descRc, DT_WORDBREAK | DT_TOP);
}

void DrawConnections(HDC hdc)
{
    HPEN pPlayer = CreatePen(PS_SOLID, 3, RGB(100, 170, 255));
    HPEN pEnemy = CreatePen(PS_SOLID, 3, RGB(255, 130, 130));

    for (const auto& u : g_units)
    {
        if (!u.alive || !u.action.valid) continue;
        if (u.action.targetIndex < 0 || u.action.targetIndex >= (int)g_units.size()) continue;
        const Unit& t = g_units[u.action.targetIndex];
        HPEN old = (HPEN)SelectObject(hdc, u.team == Team::PLAYER ? pPlayer : pEnemy);
        MoveToEx(hdc, UnitCenterX(u), UnitCenterY(u), NULL);
        LineTo(hdc, UnitCenterX(t), UnitCenterY(t));
        SelectObject(hdc, old);
    }

    DeleteObject(pPlayer);
    DeleteObject(pEnemy);
}

void DrawTopBar(HDC hdc)
{
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(245, 245, 248));

    RECT left = { 40, 26, 780, 62 };
    DrawText(hdc, L"Reception Battle Demo  |  카드 선택 -> 적 클릭  |  R : Restart", -1, &left,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT center = { 650, 26, 1040, 62 };
    std::wstring selected = L"현재 지정: ";
    if (IsPlayerIndex(g_selectedPlayer))
        selected += g_units[g_selectedPlayer].name;
    else
        selected += L"-";
    DrawText(hdc, selected.c_str(), -1, &center, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT right = { 1120, 26, 1550, 62 };
    DrawText(hdc, g_battleEnded ? L"전투 종료" : L"행동 선택 중", -1, &right, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
}

void DrawLogs(HDC hdc)
{
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(235, 235, 240));

    RECT title = { LOG_X + 16, LOG_Y + 12, LOG_X + LOG_W - 16, LOG_Y + 38 };
    DrawText(hdc, L"Combat Log", -1, &title, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    int y = LOG_Y + 46;
    for (size_t i = 0; i < g_logs.size(); ++i)
    {
        RECT line = { LOG_X + 16, y, LOG_X + LOG_W - 16, y + 22 };
        DrawText(hdc, g_logs[i].c_str(), -1, &line, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        y += 22;
    }
}

void DrawHand(HDC hdc)
{
    if (!IsPlayerIndex(g_selectedPlayer)) return;

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(245, 245, 248));

    RECT title = { 220, 642, 600, 668 };
    std::wstring t = g_units[g_selectedPlayer].name + L" Hand";
    DrawText(hdc, t.c_str(), -1, &title, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    Unit& p = g_units[g_selectedPlayer];
    for (int i = 0; i < (int)p.hand.size() && i < 8; ++i)
    {
        int x = HAND_START_X + i * (HAND_CARD_W + HAND_GAP);
        g_handRects[i] = { x, HAND_Y, x + HAND_CARD_W, HAND_Y + HAND_CARD_H };
        bool enabled = p.hand[i].cost <= p.light;
        DrawCard(hdc, g_handRects[i], p.hand[i], i == g_selectedHandIndex, enabled);
    }
}

void DrawRightPanel(HDC hdc)
{
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(240, 242, 245));

    RECT title = { PANEL_RIGHT_X + 18, PANEL_RIGHT_Y + 16, PANEL_RIGHT_X + PANEL_RIGHT_W - 18, PANEL_RIGHT_Y + 44 };
    DrawText(hdc, L"Action Queue", -1, &title, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    int y = PANEL_RIGHT_Y + 60;
    for (int i = 0; i < (int)g_units.size(); ++i)
    {
        const Unit& u = g_units[i];
        RECT row = { PANEL_RIGHT_X + 18, y, PANEL_RIGHT_X + PANEL_RIGHT_W - 18, y + 84 };
        FillRoundRect(hdc, row, RGB(38, 42, 50), RGB(90, 94, 106), 1, 14);

        RECT n = { row.left + 12, row.top + 10, row.right - 12, row.top + 32 };
        RECT s = { row.left + 12, row.top + 34, row.right - 12, row.top + 56 };
        RECT a = { row.left + 12, row.top + 56, row.right - 12, row.bottom - 8 };

        std::wstring speed = L"Speed: " + ToWString(u.speed);
        std::wstring actionText = L"No action";
        if (u.action.valid)
        {
            std::wstring targetName = (u.action.targetIndex >= 0 && u.action.targetIndex < (int)g_units.size()) ? g_units[u.action.targetIndex].name : L"?";
            std::wstring cardName = (u.action.handCardIndex >= 0 && u.action.handCardIndex < (int)u.hand.size()) ? u.hand[u.action.handCardIndex].name : L"Reserved";
            actionText = cardName + L" -> " + targetName;
        }

        SetTextColor(hdc, RGB(245, 245, 248));
        DrawText(hdc, u.name.c_str(), -1, &n, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        SetTextColor(hdc, RGB(190, 200, 216));
        DrawText(hdc, speed.c_str(), -1, &s, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        SetTextColor(hdc, u.action.valid ? RGB(160, 235, 170) : RGB(170, 170, 180));
        DrawText(hdc, actionText.c_str(), -1, &a, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        y += 96;
    }

    FillRoundRect(hdc, g_endTurnButton, RGB(72, 88, 122), RGB(235, 240, 250), 1, 16);
    SetTextColor(hdc, RGB(255, 255, 255));
    DrawTextCenter(hdc, g_endTurnButton, L"End Turn");

    RECT tip = { PANEL_RIGHT_X + 18, 690, PANEL_RIGHT_X + PANEL_RIGHT_W - 18, 740 };
    SetTextColor(hdc, RGB(210, 214, 224));
    DrawText(hdc, L"카드를 선택한 뒤 적을 클릭하면 행동을 예약합니다.", -1, &tip, DT_WORDBREAK | DT_TOP);
}

void DrawFloatingTexts(HDC hdc)
{
    SetBkMode(hdc, TRANSPARENT);
    for (const auto& ft : g_floatingTexts)
    {
        SetTextColor(hdc, ft.isDamage ? RGB(255, 96, 96) : RGB(146, 230, 156));
        RECT rc = { ft.x - 30, ft.y - ft.lifetime, ft.x + 30, ft.y - ft.lifetime + 28 };
        std::wstring txt = ft.isDamage ? (L"-" + ToWString(ft.value)) : (L"+" + ToWString(ft.value));
        DrawText(hdc, txt.c_str(), -1, &rc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }
}

void DrawTargetHint(HDC hdc)
{
    if (!IsPlayerIndex(g_selectedPlayer) || g_selectedHandIndex < 0 || g_battleEnded) return;

    RECT rc = { 760, 95, 1080, 130 };
    FillRoundRect(hdc, rc, RGB(66, 70, 86), RGB(240, 212, 110), 1, 14);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 245, 200));
    DrawTextCenter(hdc, rc, L"대상으로 적 유닛을 클릭하세요");
}

void DrawEndOverlay(HDC hdc)
{
    if (!g_battleEnded) return;

    RECT full = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
    HBITMAP old = (HBITMAP)SelectObject(mem, bmp);
    HBRUSH b = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(mem, &full, b);
    DeleteObject(b);

    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 135, 0 };
    AlphaBlend(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, mem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, bf);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);

    RECT box = { 540, 280, 1060, 500 };
    DrawShadowRect(hdc, box, RGB(8, 8, 8));
    FillRoundRect(hdc, box, RGB(26, 28, 36), RGB(230, 230, 235), 1, 24);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    RECT title = { 560, 330, 1040, 390 };
    DrawText(hdc, g_endMessage.c_str(), -1, &title, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT sub = { 560, 390, 1040, 430 };
    DrawText(hdc, L"R 키를 눌러 다시 시작", -1, &sub, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void RenderScene(HDC hdc)
{
    DrawBattleBackground(hdc);
    DrawTopBar(hdc);

    for (int i = 0; i < (int)g_units.size(); ++i)
        DrawUnit(hdc, g_units[i], i == g_selectedPlayer);

    DrawConnections(hdc);
    DrawTargetHint(hdc);
    DrawLogs(hdc);
    DrawHand(hdc);
    DrawRightPanel(hdc);
    DrawFloatingTexts(hdc);
    DrawEndOverlay(hdc);
}

// ------------------------------
// update
// ------------------------------
void UpdateFrame()
{
    for (size_t i = 0; i < g_floatingTexts.size();)
    {
        g_floatingTexts[i].lifetime--;
        if (g_floatingTexts[i].lifetime <= 0)
            g_floatingTexts.erase(g_floatingTexts.begin() + i);
        else
            ++i;
    }
}

// ------------------------------
// winproc
// ------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        srand((unsigned int)time(NULL));
        ResetBattle();
        SetTimer(hWnd, TIMER_ID, TIMER_INTERVAL, NULL);
        break;

    case WM_TIMER:
        if (wParam == TIMER_ID)
        {
            UpdateFrame();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;

    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (g_battleEnded)
            break;

        if (PtInRectSafe(g_endTurnButton, x, y))
        {
            RunTurn();
            break;
        }

        if (IsPlayerIndex(g_selectedPlayer))
        {
            Unit& p = g_units[g_selectedPlayer];
            for (int i = 0; i < (int)p.hand.size() && i < 8; ++i)
            {
                if (PtInRectSafe(g_handRects[i], x, y))
                {
                    g_selectedHandIndex = i;
                    InvalidateRect(hWnd, NULL, FALSE);
                    return 0;
                }
            }
        }

        int clicked = FindUnitAt(x, y);
        if (clicked != -1)
        {
            if (IsEnemyIndex(clicked) && g_selectedHandIndex >= 0)
            {
                AssignPlayerAction(clicked);
            }
            else if (IsPlayerIndex(clicked) && g_units[clicked].alive)
            {
                g_selectedPlayer = clicked;
                g_selectedHandIndex = -1;
            }
        }
    }
    break;

    case WM_KEYDOWN:
        if (wParam == 'R')
        {
            ResetBattle();
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        HFONT font = CreateFont(
            22, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Malgun Gothic");
        HFONT oldFont = (HFONT)SelectObject(memDC, font);

        RenderScene(memDC);
        BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldFont);
        DeleteObject(font);
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_ID);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"RuinaInspiredBattleWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, L"윈도우 클래스 등록 실패", L"오류", MB_ICONERROR);
        return 0;
    }

    HWND hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Ruina-style Battle Demo - Game Layout",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hWnd == NULL)
    {
        MessageBox(NULL, L"윈도우 생성 실패", L"오류", MB_ICONERROR);
        return 0;
    }

    g_hWnd = hWnd;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
