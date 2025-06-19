// WindowsProject1.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "WindowsProject1.h"
#include <cmath>
#include <functional>
#include "KVector2.h"
#include <MMSystem.h>
#include "KTime.h"
#include "KComplex.h"
#include <gdiplus.h>
#include <vector>
#include <time.h>
#include <windowsx.h>
#include "KMatrix2.h"
#include "KMatrix3.h"
#include "KTileManager.h"
#include <vector>
#include "KSpriteAnimator.h"

using namespace Gdiplus;
#pragma comment(lib,"winmm.lib")
#pragma comment( lib, "gdiplus.lib" )

#define MAX_LOADSTRING 100

// Global Bullet struct and container
struct Bullet {
    KVector2 position;
    KVector2 velocity;
};

struct Player {
    KVector2 position;
    int Health = 5;
    int XP = 0;
    int Damage = 1;
    int Level = 1;
    int XPToNextLevel = 5;
};

struct Enemy {
    KVector2 position;
    double speed;
    int Health = 2;
    int Damage = 1;
};

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

POINT g_center;
int g_pixelPerUnit = 10;
HWND    g_hwnd = NULL;
HDC     g_hdc = 0;
HBITMAP g_hBitmap = 0;
RECT    g_clientRect;
KTime   g_time;
Image* g_image = nullptr;
Image*  g_bulletImage = nullptr;
int		g_mouseLButtonDown = 0;
KVector2 g_worldPoint = KVector2::zero;
double g_angle = 0.0;
double g_pi = 0.0;
KTileManager* g_playerTileManager = nullptr;
KTileManager* g_enemyTileManager = nullptr;
std::vector<Bullet> g_bullets;
KSpriteAnimator g_playerAnimator;
KSpriteAnimator g_enemyAnimator;

/*--- Initializing Player Character ---*/
Player g_player = { KVector2::zero, 5 };

/*--- Initializing the Enemy Character ---*/
std::vector<Enemy> g_enemy;
double g_enemySpawnTimer = 0.0;
double g_enemySpawnInterval = 3.0;

/*--- Difficulty Settings ---*/
double g_difficultyTimer = 0.0;
int g_difficultyLevel = 1;
const double g_difficultyInterval = 10.0;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void OnPaint(HDC hdc);
void DrawFunction(HDC hdc, std::function<double(double)> MyFunction
    , double begin, double end, COLORREF c, double step);
void OnSize(HWND hwnd);
void Transform(double* x, double* y);
void InverseTransform(double* x, double* y);
double GetStdDeviation(double base_, double beginX, double endX, double xstep);
void DrawLine(HDC hdc, double x1, double y1, double x2, double y2, COLORREF c = RGB(0, 0, 0), int penStyle = PS_SOLID, int lineWidth=1);
void DrawVector(HDC hdc, KVector2 p1, KVector2 p2, COLORREF c, int penStyle = PS_SOLID, int lineWidth = 1);
void DrawVector(HDC hdc, KComplex c1, KComplex c2, COLORREF c, int penStyle = PS_SOLID, int lineWidth = 1);
void DrawImage(HDC hdc, Image* image, KVector2 p);
void Initialize();
void Finalize();
void InitializeEnemy();
void MoveEnemyTowardsPlayer();
void OnIdle();
void LButtonDown(int x, int y);
void OnLButtonDown(int x, int y);
void OnLButtonUp();
double CalculatePi();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    Initialize();
    DWORD dwOldTime = ::timeGetTime();
    MSG msg;

    // Main message loop:
    while (true)
    {
        ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        const DWORD dwNewTime = ::timeGetTime();
        const BOOL bIsTranslateMessage = TranslateAccelerator(msg.hwnd, hAccelTable, &msg);
        if (!bIsTranslateMessage)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }//if

        g_time.deltaTime = (dwNewTime - dwOldTime) / 1000.0;
        OnIdle();
        Sleep(10);

        dwOldTime = dwNewTime;

        if (msg.message == WM_QUIT)
        {
            break;
        }//if
    }//while

    Finalize();
    Gdiplus::GdiplusShutdown(gdiplusToken);

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   g_hwnd = hWnd;
   OnSize(hWnd);
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            //OnPaint(hdc);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        OnSize(hWnd);
        break;
    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    case WM_LBUTTONUP:
        OnLButtonUp();
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

double Square(double x)
{
    return x * x;
}

double SqureRoot(double x)
{
    //return std::sqrt(x);
    return std::pow(x, 1 / 2.0);
}

double Exp(double x)
{
    return std::exp(x);
}

double ExpFunction(double base_, double x)
{
    return std::pow(base_, x);
}

double Logistic(double x)
{
    // Logistic Function
    const double L = 1.0;
    const double k = 3.0;
    const double x0 = 0.0;
    return L / (1 + std::exp(-k * (x - x0)));
}

double Gaussian(double x)
{
    // Gaussian Function
    const double a = 1.0;
    const double b = 0.0;
    const double c = 1.0;
    return a * std::exp(-((x - b) * (x - b)) / (2 * c * c));
}

//double(*CALLBACK)(double x)

void DrawFunction(HDC hdc,
    std::function<double(double)> MyFunction,
    double begin, double end, COLORREF c, double step = 0.2)
{
    double prevx = begin;
    double prevy = MyFunction(prevx);

    for (double x = begin; x <= end; x += step) {
        double y = MyFunction(x);
        DrawLine(hdc, prevx, prevy, x, y, c, PS_SOLID, 2);
        prevx = x;
        prevy = y;
    }//for
}

void DrawFunction(HDC hdc,
    std::function<double(double,double)> MyFunction,
    double base_,
    double begin, double end, COLORREF c, double step = 0.2)
{
    double prevx = begin;
    double prevy = MyFunction(base_,prevx);

    for (double x = begin; x <= end; x += step) {
        double y = MyFunction(base_,x);
        DrawLine(hdc, prevx, prevy, x, y, c, PS_SOLID, 2);
        prevx = x;
        prevy = y;
    }//for
}

double NewtonsDifference(std::function<double(double)> f, double x, double dx = 0.0001)
{
    const double y0 = f(x);
    const double y1 = f(x + dx);
    return (y1 - y0) / dx;
}

double SymmetricDifference(std::function<double(double)> f, double x, double dx = 0.0001)
{
    const double y0 = f(x - dx);
    const double y1 = f(x + dx);
    return (y1 - y0) / (2.0 * dx);
}

double SymmetricDifference(std::function<double(double,double)> f, double base_, double x, double dx = 0.0001)
{
    const double y0 = f(base_, x - dx);
    const double y1 = f(base_, x + dx);
    return (y1 - y0) / (2.0 * dx);
}

double GetStdDeviation(double base_, double beginX, double endX, double xstep)
{
    std::vector<double> vecDiff;

    double x = beginX;
    double ydiff;
    double N = 0;
    while (x < endX) {
        ydiff = ExpFunction(base_, x) - SymmetricDifference(&ExpFunction, base_, x);
        x += xstep;
        N += 1;
        vecDiff.push_back(ydiff);
    }//while

    double sum = 0;
    for (const double diff : vecDiff) {
        sum += (diff * diff);
    }

    return sqrt(sum / (N - 1));
}

void DrawGrid(HDC hdc, int gridCount)
{
    int width = g_clientRect.right - g_clientRect.left;
    int height = g_clientRect.bottom - g_clientRect.top;

    double halfWidthUnits = (double)width / (2.0 * g_pixelPerUnit);
    double halfHeightUnits = (double)height / (2.0 * g_pixelPerUnit);

    for (int i = 1; i <= gridCount; ++i) {
        double offset = i;

        // Vertical lines (ÁÂ¿ì)
        DrawLine(hdc, -offset, -halfHeightUnits, -offset, +halfHeightUnits, RGB(200, 200, 200), PS_DASH);
        DrawLine(hdc, +offset, -halfHeightUnits, +offset, +halfHeightUnits, RGB(200, 200, 200), PS_DASH);

        // Horizontal lines (»óÇÏ)
        DrawLine(hdc, -halfWidthUnits, -offset, +halfWidthUnits, -offset, RGB(200, 200, 200), PS_DASH);
        DrawLine(hdc, -halfWidthUnits, +offset, +halfWidthUnits, +offset, RGB(200, 200, 200), PS_DASH);
    }
}

void DrawImage(HDC hdc, Image* image, KVector2 p, double degree)
{
    if (image != nullptr) {
        int imageWidth = image->GetWidth();
        int imageHeight = image->GetHeight();

        Transform(&p.x, &p.y); // transform to client coordinate

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeHighQuality);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

        // push transform to the stack
        graphics.TranslateTransform((REAL)p.x, (REAL)p.y);
        graphics.RotateTransform((REAL)degree);
        graphics.TranslateTransform((REAL)-imageWidth / 2, (REAL)-imageHeight / 2);

        graphics.DrawImage(image, 0, 0, imageWidth, imageHeight);

        // reset transform
        graphics.ResetTransform();
    }
}

void OnPaint(HDC hdc)
{
    char buffer[100];
    int len = 0;

    for (const Bullet& bullet : g_bullets) {
        DrawImage(hdc, g_bulletImage, bullet.position);
    }

    {
        const float moveSpeed = 5.0f;
        float dt = (float)g_time.deltaTime;

        if (GetAsyncKeyState(VK_LEFT) & 0x8000)
            g_player.position.x -= moveSpeed * dt;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
            g_player.position.x += moveSpeed * dt;
        if (GetAsyncKeyState(VK_UP) & 0x8000)
            g_player.position.y += moveSpeed * dt;
        if (GetAsyncKeyState(VK_DOWN) & 0x8000)
            g_player.position.y -= moveSpeed * dt;
    }

    KVector2 playerPos = g_player.position;
    Transform(&playerPos.x, &playerPos.y);

    g_playerAnimator.Draw(hdc, 0, playerPos.x - 15, playerPos.y - 40, false, 2.0);

    for (const Enemy& enemy : g_enemy) {
        KVector2 enemyPos = enemy.position;
        Transform(&enemyPos.x, &enemyPos.y);

        g_enemyAnimator.Draw(hdc, 0, enemyPos.x - 15, enemyPos.y - 40, false, 2.0);
    }

    sprintf_s(buffer, "HP: %d   DMG: %d   XP: %d/%d   LVL: %d   DANGER: %d", g_player.Health, g_player.Damage, g_player.XP, g_player.XPToNextLevel, g_player.Level, g_difficultyLevel);
    TextOutA(hdc, 10, 10, buffer, (int)strlen(buffer));
}

void DrawLine(HDC hdc, double x1, double y1, double x2, double y2, COLORREF c, int penStyle, int lineWidth)
{
    HPEN hPen;
    HPEN hPrevPen;
    hPen = CreatePen(penStyle, lineWidth, c);
    hPrevPen = (HPEN)SelectObject(hdc, hPen);

    Transform(&x1, &y1);
    MoveToEx(hdc, (int)x1, (int)y1, NULL);
    Transform(&x2, &y2);
    LineTo(hdc, (int)x2, (int)y2);

    SelectObject(hdc, hPrevPen);
    DeleteObject(hPen);
}

void DrawVector(HDC hdc, KVector2 p1, KVector2 p2, COLORREF c, int penStyle, int lineWidth)
{
    DrawLine(hdc, p1.x, p1.y, p2.x, p2.y, c, penStyle, lineWidth);
}

void DrawVector(HDC hdc, KComplex c1, KComplex c2, COLORREF c, int penStyle, int lineWidth)
{
    DrawLine(hdc, c1.r, c1.i, c2.r, c2.i, c, penStyle, lineWidth);
}

void DrawImage(HDC hdc, Image* image, KVector2 p)
{
    if (image != nullptr) {
        int imageWidth = image->GetWidth();
        int imageHeight = image->GetHeight();
        Graphics graphics(hdc);
        Transform(&p.x, &p.y);
        graphics.DrawImage(image, (int)p.x - imageWidth / 2, (int)p.y - imageHeight / 2);
    }
}

void OnSize(HWND hwnd)
{
    Finalize();

    ::GetClientRect(g_hwnd, &g_clientRect);
    const int width = g_clientRect.right - g_clientRect.left + 1;
    const int height = g_clientRect.bottom - g_clientRect.top + 1;
    g_center.x = width / 2;
    g_center.y = height / 2;
    g_pixelPerUnit = 50;

    HDC hdc = ::GetDC(g_hwnd);
    g_hdc = CreateCompatibleDC(hdc);
    g_hBitmap = CreateCompatibleBitmap(hdc, width, height);
    SelectObject(g_hdc, g_hBitmap);

    Initialize();
}

void Transform(double* x, double* y)
{
    *x = g_center.x + (*x) * g_pixelPerUnit;
    *y = g_center.y + -(*y) * g_pixelPerUnit;
}

void InverseTransform(double* x, double* y)
{
    *x = ((*x) - g_center.x) / g_pixelPerUnit;
    *y = -((*y) - g_center.y) / g_pixelPerUnit;
}

void Initialize()
{
    srand(time(NULL));
    if (g_image == nullptr)
        g_image = new Image(L"Baren.png");

    if (g_bulletImage == nullptr)
        g_bulletImage = new Image(L"bullet.png");

    if (g_playerTileManager == nullptr)
        g_playerTileManager = new KTileManager();
    g_playerTileManager->LoadTileSheet(L"Atlas-working.png", 16, 32);

    if (g_enemyTileManager == nullptr) {
        g_enemyTileManager = new KTileManager();
    }
    g_enemyTileManager->LoadTileSheet(L"Atlas-working.png", 16, 32);

    g_playerAnimator.ClearAll();
    g_playerAnimator.SetTilemap(g_playerTileManager);

    std::vector<KVector2> playerWalkRightFrames = {
        KVector2(0, 0),
        KVector2(1, 0),
        KVector2(2, 0),
        KVector2(3, 0)
    };

    g_playerAnimator.SetAnimation(0, playerWalkRightFrames, 0.15);

    g_enemyAnimator.ClearAll();
    g_enemyAnimator.SetTilemap(g_enemyTileManager);

    std::vector<KVector2> enemyWalkRightFrames = {
        KVector2(0, 1),
        KVector2(1, 1),
        KVector2(2, 1),
        KVector2(3, 1)
    };

    g_enemyAnimator.SetAnimation(0, enemyWalkRightFrames, 0.15);

}

void Finalize()
{
    if (g_playerTileManager != nullptr) {
        delete g_playerTileManager;
        g_playerTileManager = nullptr;
    }
    if (g_enemyTileManager != nullptr) {
        delete g_enemyTileManager;
        g_enemyTileManager = nullptr;
    }
    if (g_hdc != 0) {
        DeleteDC(g_hdc);
        g_hdc = 0;
    }//if
    if (g_hBitmap != 0) {
        DeleteObject(g_hBitmap);
        g_hBitmap = 0;
    }//if
    if (g_image != nullptr) {
        delete g_image;
        g_image = nullptr;
    }//if
    if (g_bulletImage != nullptr) {
        delete g_bulletImage;
        g_bulletImage = nullptr;
    }//if
}//Finalize()

void ShowUpgradeMenu()
{
    int result = MessageBox(
        g_hwnd,
        L"Choose your upgrade:\nYes = +1 Max HP\nNo = +1 Damage",
        L"Upgrade Choice",
        MB_YESNO | MB_ICONQUESTION
    );

    if (result == IDYES) {
        g_player.Health += 1;
    }
    else if (result == IDNO) {
        g_player.Damage += 1;
    }
}

void EnemyHitDetection() {
    for (int i = (int)g_bullets.size() - 1; i >= 0; --i) {
        Bullet& bullet = g_bullets[i];
        for (int j = (int)g_enemy.size() - 1; j >= 0; --j) {
            Enemy& enemy = g_enemy[j];
            if ((bullet.position - enemy.position).Length() < 0.5) {
                enemy.Health -= g_player.Damage;
                g_bullets.erase(g_bullets.begin() + i);

                if (enemy.Health <= 0) {
                    g_enemy.erase(g_enemy.begin() + j);

                    g_player.XP += 1;
                    if (g_player.XP >= g_player.XPToNextLevel) {
                        g_player.XP = 0;
                        g_player.Level += 1;
                        g_player.XPToNextLevel += 2;

                        MessageBox(g_hwnd, L"Level up! You earned an upgrade!", L"LEVEL UP", MB_OK);
                        ShowUpgradeMenu();
                    }
                }

                break;
            }
        }
    }
}

void UpdateBullets()
{
    const double speed = 2.0;
    double dt = g_time.deltaTime;

    for (auto& bullet : g_bullets) {
        bullet.position = bullet.position + bullet.velocity * (float)(speed * dt);
    }

    EnemyHitDetection();
}

double Random()
{
    return (double)rand() / RAND_MAX;
}

void InitializeEnemy() {
    g_enemySpawnTimer += g_time.deltaTime;
    if (g_enemySpawnTimer >= g_enemySpawnInterval) {
        g_enemySpawnTimer = 0.0;

        Enemy enemy;
        enemy.position = KVector2(Random() * 20.0 - 10.0, Random() * 20.0 - 10.0);
        enemy.speed = 1.5 + Random() * 1.0 + g_difficultyLevel * 0.1;
        enemy.Health = 2 + g_difficultyLevel / 2;
        enemy.Damage = enemy.Damage = 1 + g_difficultyLevel / 3;
        
        g_enemy.push_back(enemy);
    }
}

void MoveEnemyTowardsPlayer() {
    for (auto& enemy : g_enemy) {
        KVector2 dir = g_player.position - enemy.position;
        if (dir.Length() > 0.0001) {
            dir = dir.Normalize();
            enemy.position += dir * enemy.speed * g_time.deltaTime;
        }
        for (int i = (int)g_enemy.size() - 1; i >= 0; --i) {
            Enemy& enemy = g_enemy[i];
            if ((enemy.position - g_player.position).Length() < 0.5) {
                g_player.Health -= enemy.Damage;
                g_enemy.erase(g_enemy.begin() + i);
                if (g_player.Health <= 0) {
                    MessageBox(g_hwnd, L"You were defeated by an Enemy! Game Over!", L"Game Over", MB_OK);
                    PostQuitMessage(0);
                    return;
                }
            }
        }
    }
}

void OnIdle()
{
    const int iWidth = g_clientRect.right - g_clientRect.left + 1;
    const int iHeight = g_clientRect.bottom - g_clientRect.top + 1;

    HDC hdc = ::GetDC(g_hwnd);

    HBRUSH brush;
    brush = CreateSolidBrush(RGB(255, 255, 255));
    SelectObject(g_hdc, brush);
    Rectangle(g_hdc, 0, 0, iWidth, iHeight);

    g_difficultyTimer += g_time.deltaTime;
    if (g_difficultyTimer >= g_difficultyInterval) {
        g_difficultyTimer = 0.0;
        g_difficultyLevel += 1;
    }

    {
        UpdateBullets();

        InitializeEnemy();
        MoveEnemyTowardsPlayer();

        g_playerAnimator.Update(g_time.deltaTime);
        g_enemyAnimator.Update(g_time.deltaTime);

        OnPaint(g_hdc);
    }

    BitBlt(hdc, 0, 0, iWidth, iHeight, g_hdc, 0, 0, SRCCOPY);
    DeleteObject(brush);

    ::ReleaseDC(g_hwnd, hdc);
}

void LButtonDown(int x, int y)
{
    KVector2 mousePoint;
    mousePoint.x = x;
    mousePoint.y = y;
    InverseTransform(&mousePoint.x, &mousePoint.y);
    g_worldPoint = mousePoint;
    g_angle = 0.0;

    KVector2 dir = mousePoint - g_player.position;
    if (dir.Length() > 0.0001) {
        dir = dir.Normalize();
        Bullet bullet;
        bullet.position = g_player.position;
        bullet.velocity = dir*2;
        g_bullets.push_back(bullet);
    }
}

void OnLButtonDown(int x, int y)
{
    if (g_mouseLButtonDown != 1) {
        LButtonDown(x, y);
    }
    g_mouseLButtonDown = 1;
}

void OnLButtonUp()
{
    g_mouseLButtonDown = 0;
}

double CalculatePi()
{
    const long numPoints = 1000000;
    long numPointsInCircle = 0;
    double x, y;

    for (long i = 0; i < numPoints; i++) {
        x = Random() * 2 - 1;
        y = Random() * 2 - 1;

        // count number of points in circle
        if (x * x + y * y <= 1) {
            numPointsInCircle += 1;
        }
    }

    return 4.0 * numPointsInCircle / numPoints;
}
