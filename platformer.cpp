#include <Windows.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <cmath>

int nScreenWidth = 60;
int nScreenHeight = 30;
int nPixels = nScreenWidth * nScreenHeight;
DWORD dwBytesWritten;
auto clock1 = std::chrono::high_resolution_clock::now();
auto clock2 = std::chrono::high_resolution_clock::now();

const std::unordered_map<wchar_t, wchar_t> tileMap {
    {L'#', 0x2588},
    {L'.', L' '}
};

struct Map {
    std::vector<std::vector<char>> gameMap;
    int width;
    int height;

    void loadMap(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            MessageBox(NULL, "Unable to open file", "Error", MB_OK | MB_ICONERROR);
            return;
        }
        
        //Read height and width
        std::string line;

        if (std::getline(file, line)) {
            height = std::stoi(line);
        } else {
            MessageBox(NULL, "Cannot read file", "Error", MB_OK | MB_ICONERROR);
            return;
        }

        std::getline(file, line);
        width = std::stoi(line);

        // Read the map data
        while (std::getline(file, line)) {
            if ((line.size()) != width) {
                MessageBox(NULL, "Incorrect number of characters in row", "Error", MB_OK | MB_ICONERROR);
                return;
            }
            std::vector<char> row(line.begin(), line.end());
            gameMap.push_back(row);
        }

        if (gameMap.size() != height) {
            MessageBox(NULL, "Incorrect number of rows", "Error", MB_OK | MB_ICONERROR);
            return;
        }

    }
};

class Player {
public:
    //Variables
    float fPlayerX;
    float fPlayerY;
    float fPlayerXVelo = 0.0f;
    float fPlayerYVelo = 0.0f;
    bool bOnGround;

    //Constants (adjust these)
    float fPlayerJumpVelo = 6.0f;
    float fPlayerFallAccel = 6.0f;
    float fPlayerXAccel = 16.0f;
    float fFrictionCoefficient = 0.2f;

    Player(int x, int y) : fPlayerX{static_cast<float>(x)}, fPlayerY{static_cast<float>(y)} {}
    
    void move(Map map) {
        //Calculate Elapsed Time per frame
        clock2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsedTime = clock2 - clock1;
        float dt = elapsedTime.count();
        clock1 = clock2;

        if (GetAsyncKeyState((unsigned short)'D') & 0x8000) {
            //Change Velocity
            fPlayerXVelo += fPlayerXAccel * dt;
        }

        if (GetAsyncKeyState((unsigned short)'A') & 0x8000) {
            //Change Velocity
            fPlayerXVelo -= fPlayerXAccel * dt;
        }

        if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
            if (bOnGround == true) {
                fPlayerYVelo = -fPlayerJumpVelo;
            }
        }

        //Resistance
        fPlayerXVelo *= std::pow(fFrictionCoefficient, dt);
        fPlayerX += fPlayerXVelo * dt;

        //Collision Detection X
        if (fPlayerXVelo > 0) {
            if (round(fPlayerX) + 1 < map.width && map.gameMap[round(fPlayerY)][round(fPlayerX) + 1] == L'#' 
            || round(fPlayerX) + 1 == map.width) {
                fPlayerXVelo = 0;
                fPlayerX = round(fPlayerX);
            }
        }
        
        if (fPlayerXVelo < 0) {
            if (round(fPlayerX) - 1 >= 0 && map.gameMap[round(fPlayerY)][round(fPlayerX) - 1] == L'#'
            || round(fPlayerX) == 0) {
                fPlayerXVelo = 0;
                fPlayerX = round(fPlayerX);
            }
        }

        //Collision Detection Y
        if (round(fPlayerY) + 1 < map.height && map.gameMap[round(fPlayerY) + 1][round(fPlayerX)] == L'#') {
            if (fPlayerYVelo > 0) {
                fPlayerYVelo = 0;
                fPlayerY = round(fPlayerY);
            }
            bOnGround = true;
        } else {
            bOnGround = false;
            fPlayerYVelo += fPlayerFallAccel * dt;
        }

        if (round(fPlayerY) + 1 < map.height && map.gameMap[round(fPlayerY)][round(fPlayerX)] == L'#') {
            while (round(fPlayerY) + 1 < map.height && map.gameMap[round(fPlayerY)][round(fPlayerX)] == L'#') {
                fPlayerY -= 1;
            }
            bOnGround = true;
        }

        if (fPlayerYVelo < 0) {
            if (round(fPlayerY) - 1 >= 0 && map.gameMap[round(fPlayerY) - 1][round(fPlayerX)] == L'#') {
                fPlayerYVelo = 0;
                fPlayerY = round(fPlayerY);
            }
        }

        //Gravity
        fPlayerY += fPlayerYVelo * dt;
    }
};

void render(const Player& player, const Map& map, HANDLE& console, wchar_t* screen) {
    //Render Map
    for (int i = 0; i < map.height; i++) {
        for (int j = 0; j < map.width; j++) {
            wchar_t tile = map.gameMap[i][j];
            auto it = tileMap.find(tile);
            if (it != tileMap.end()) {
                screen[i * map.width + j] = it->second;
            }
        }
    }

    //Render Player
    int playerX = round(player.fPlayerX);
    int playerY = round(player.fPlayerY);
    if ((playerY * map.width + playerX) < (nScreenWidth * nScreenHeight) && 
    (playerY * map.width + playerX) >= 0) {
        screen[playerY * map.width + playerX] = L'O';
    }

    //Render debugger
    if (player.bOnGround) {
        screen[0] = L'T';
    } else screen[0] = L'F';

    //Render FPS

    // Display Frame
    WriteConsoleOutputCharacterW(console, screen, nScreenWidth * nScreenHeight, {0,0}, &dwBytesWritten);
}

int main(void) {
    //Create Screen Buffer
    wchar_t* screen = new wchar_t[nPixels];

    //Console Output Buffer
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SMALL_RECT rectWindow = {0, 0, static_cast<SHORT>(nScreenWidth - 1), static_cast<SHORT>(nScreenHeight - 1)};
    SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);
    COORD coord = {static_cast<SHORT>(nScreenWidth), static_cast<SHORT>(nScreenHeight)};
    SetConsoleScreenBufferSize(hConsole, coord);
    SetConsoleActiveScreenBuffer(hConsole);

    //Initialize
    Player player(20, 24);
    Map map;
    map.loadMap("map.txt");
    
    //Game Loop
    while(true) {
        render(player, map, hConsole, screen);
        player.move(map);
        Sleep(10);
    }
}