#define _CRT_SECURE_NO_WARNINGS // for STB

#include "../../thirteen.h"

#include <stdio.h>
#include <random>
#include <vector>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static const unsigned int c_width = 768;
static const unsigned int c_height = 768;
static const bool c_fullscreen = false;

static constexpr unsigned int c_boardSize[2] = { 16, 16 };
static const unsigned int c_numMines = 40;

class Board
{
public:
    static void Initialize()
    {
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<int> distX(0, c_boardSize[0] - 1);
        std::uniform_int_distribution<int> distY(0, c_boardSize[1] - 1);

        memset(s_mines, 0, sizeof(s_mines));
        memset(s_revealed, 0, sizeof(s_revealed));
        memset(s_flagged, 0, sizeof(s_flagged));

        int minesPlaced = 0;
        while (minesPlaced < c_numMines)
        {
            int x = distX(rng);
            int y = distY(rng);
            if (s_mines[y * c_boardSize[0] + x])
                continue;
            s_mines[y * c_boardSize[0] + x] = true;
            minesPlaced++;
        }

        s_initialized = true;
    }

    static bool Revealed(int x, int y)
    {
        return s_revealed[y * c_boardSize[0] + x];
    }

    static bool Flagged(int x, int y)
    {
        return s_flagged[y * c_boardSize[0] + x];
    }

    static bool Mine(int x, int y)
    {
        return s_mines[y * c_boardSize[0] + x];
    }

    enum class GameResult
    {
        Undecided,
        Win,
        Lose
    };

    static GameResult GetGameResult()
    {
        // If any mines are revealed, that's a loss
        for (size_t i = 0; i < c_boardSize[0] * c_boardSize[1]; ++i)
        {
            if (s_revealed[i] && s_mines[i])
                return GameResult::Lose;
        }

        // If there is a square that isn't revealed, and it isn't a mine, the game is still going.
        for (size_t i = 0; i < c_boardSize[0] * c_boardSize[1]; ++i)
        {
            if (!s_revealed[i] && !s_mines[i])
                return GameResult::Undecided;
        }

        // Otherwise the game was won
        return GameResult::Win;
    }

    static void OnLeftClick(int x, int y)
    {
        s_revealed[y * c_boardSize[0] + x] = true;

        if (Mine(x, y))
            return;

        std::vector<int> checkForReveal;
        checkForReveal.push_back(y * c_boardSize[0] + x);

        while (!checkForReveal.empty())
        {
            int checkIndex = checkForReveal.back();
            checkForReveal.pop_back();

            int checkX = checkIndex % c_boardSize[0];
            int checkY = checkIndex / c_boardSize[0];
            if (NumNeighbors(checkX, checkY) == 0)
            {
                for (int iy = -1; iy <= 1; ++iy)
                {
                    for (int ix = -1; ix <= 1; ++ix)
                    {
                        if (ix == 0 && iy == 0)
                            continue;

                        int newX = checkX + ix;
                        int newY = checkY + iy;
                        if (newX < 0 || newX >= (int)c_boardSize[0] || newY < 0 || newY >= (int)c_boardSize[1])
                            continue;

                        if (!Revealed(newX, newY))
                        {
                            s_revealed[newY * c_boardSize[0] + newX] = true;
                            checkForReveal.push_back(newY * c_boardSize[0] + newX);
                        }
                    }
                }
            }
        }
    }

    static void OnRightClick(int x, int y)
    {
        s_flagged[y * c_boardSize[0] + x] = !s_flagged[y * c_boardSize[0] + x];
    }

    static void ScreenPosToBoardPos(int screenX, int screenY, int& boardX, int& boardY)
    {
        float percentX = (float(screenX) + 0.5f) / float(c_width);
        float percentY = (float(screenY) + 0.5f) / float(c_height);
        boardX = int(percentX * c_boardSize[0]);
        boardY = int(percentY * c_boardSize[1]);
    }

    static int NumNeighbors(int cellX, int cellY)
    {
        int ret = 0;
        for (int iy = -1; iy <= 1; ++iy)
        {
            for (int ix = -1; ix <= 1; ++ix)
            {
                if (ix == 0 && iy == 0)
                    continue;

                int x = cellX + ix;
                int y = cellY + iy;

                if (x < 0 || x >= (int)c_boardSize[0] || y < 0 || y >= (int)c_boardSize[1])
                    continue;

                if (Mine(x, y))
                    ret++;
            }
        }
        return ret;
    }

    static bool DrawCircle(size_t x, size_t y, size_t centerX, size_t centerY, size_t radius)
    {
        float dx = float(x) - float(centerX);
        float dy = float(y) - float(centerY);
        float dist = std::sqrt(dx * dx + dy * dy);
        return dist < float(radius);
    }

    static void DrawPixel(int x, int y, unsigned char* outColor, Board::GameResult gameResult)
    {
        outColor[3] = 255;

        int cellX, cellY;
        ScreenPosToBoardPos(x, y, cellX, cellY);

        size_t cellWidth = c_width / c_boardSize[0];
        size_t cellHeight = c_height / c_boardSize[1];

        int relX = x % cellWidth;
        int relY = y % cellHeight;

        // Draw unrevealed cell
        if (!Board::Revealed(cellX, cellY))
        {
            if (relX < 2 || relY < 2)
            {
                outColor[0] = 255;
                outColor[1] = 255;
                outColor[2] = 255;
            }
            else if (relX >= cellWidth - 2 || relY >= cellHeight - 2)
            {
                outColor[0] = 128;
                outColor[1] = 128;
                outColor[2] = 128;
            }
            else
            {
                outColor[0] = 192;
                outColor[1] = 192;
                outColor[2] = 192;
            }

            if (Board::Flagged(cellX, cellY))
            {
                if (DrawCircle(relX, relY, cellWidth / 2, cellHeight / 2, cellWidth / 4))
                {
                    outColor[0] = 0;
                    outColor[1] = 255;
                    outColor[2] = 0;
                }
            }
        }
        // Draw exposed mine
        else if (Board::Mine(cellX, cellY))
        {
            if (DrawCircle(relX, relY, cellWidth / 2, cellHeight / 2, cellWidth / 3))
            {
                outColor[0] = 255;
                outColor[1] = 0;
                outColor[2] = 0;
            }
            else
            {
                outColor[0] = 255;
                outColor[1] = 128;
                outColor[2] = 128;
            }
        }
        // Draw exposed empty cell, with the number of adjacent mines
        else
        {
            if (relX < 1 || relY < 1 || relX >= cellWidth - 1 || relY >= cellHeight - 1)
            {
                outColor[0] = 100;
                outColor[1] = 100;
                outColor[2] = 100;
            }
            else
            {
                outColor[0] = 164;
                outColor[1] = 164;
                outColor[2] = 164;
            }

            int numNeighbors = NumNeighbors(cellX, cellY);

            // center dot
            if (numNeighbors == 1 || numNeighbors == 3 || numNeighbors == 5 || numNeighbors == 7)
            {
                if (DrawCircle(relX, relY, cellWidth / 2, cellHeight / 2, cellWidth / 8))
                {
                    outColor[0] = 64;
                    outColor[1] = 64;
                    outColor[2] = 64;
                }
            }

            // upper left dot
            if (numNeighbors == 2 || numNeighbors == 3 || numNeighbors == 4 || numNeighbors == 5 || numNeighbors == 6 || numNeighbors == 7 || numNeighbors == 8)
            {
                if (DrawCircle(relX, relY, cellWidth * 1 / 4, cellHeight * 1 / 4, cellWidth / 8))
                {
                    outColor[0] = 64;
                    outColor[1] = 64;
                    outColor[2] = 64;
                }
            }

            // lower right dot
            if (numNeighbors == 2 || numNeighbors == 3 || numNeighbors == 4 || numNeighbors == 5 || numNeighbors == 6 || numNeighbors == 7 || numNeighbors == 8)
            {
                if (DrawCircle(relX, relY, cellWidth * 3 / 4, cellHeight * 3 / 4, cellWidth / 8))
                {
                    outColor[0] = 64;
                    outColor[1] = 64;
                    outColor[2] = 64;
                }
            }

            // lower left dot
            if (numNeighbors == 4 || numNeighbors == 5 || numNeighbors == 6 || numNeighbors == 7 || numNeighbors == 8)
            {
                if (DrawCircle(relX, relY, cellWidth * 1 / 4, cellHeight * 3 / 4, cellWidth / 8))
                {
                    outColor[0] = 64;
                    outColor[1] = 64;
                    outColor[2] = 64;
                }
            }

            // upper right dot
            if (numNeighbors == 4 || numNeighbors == 5 || numNeighbors == 6 || numNeighbors == 7 || numNeighbors == 8)
            {
                if (DrawCircle(relX, relY, cellWidth * 3 / 4, cellHeight * 1 / 4, cellWidth / 8))
                {
                    outColor[0] = 64;
                    outColor[1] = 64;
                    outColor[2] = 64;
                }
            }

            // left center dot
            if (numNeighbors == 6 || numNeighbors == 7 || numNeighbors == 8)
            {
                if (DrawCircle(relX, relY, cellWidth * 1 / 4, cellHeight * 1 / 2, cellWidth / 8))
                {
                    outColor[0] = 64;
                    outColor[1] = 64;
                    outColor[2] = 64;
                }
            }

            // right center dot
            if (numNeighbors == 6 || numNeighbors == 7 || numNeighbors == 8)
            {
                if (DrawCircle(relX, relY, cellWidth * 3 / 4, cellHeight * 1 / 2, cellWidth / 8))
                {
                    outColor[0] = 64;
                    outColor[1] = 64;
                    outColor[2] = 64;
                }
            }

            // center top dot
            if (numNeighbors == 8)
            {
                if (DrawCircle(relX, relY, cellWidth * 1 / 2, cellHeight * 1 / 4, cellWidth / 8))
                {
                    outColor[0] = 64;
                    outColor[1] = 64;
                    outColor[2] = 64;
                }
            }

            // center bottom dot
            if (numNeighbors == 8)
            {
                if (DrawCircle(relX, relY, cellWidth * 1 / 2, cellHeight * 3 / 4, cellWidth / 8))
                {
                    outColor[0] = 64;
                    outColor[1] = 64;
                    outColor[2] = 64;
                }
            }
        }

        if (gameResult == GameResult::Win)
        {
            outColor[1] = 255;
        }
        else if (gameResult == GameResult::Lose)
        {
            outColor[1] /= 2;
            outColor[2] /= 2;
        }
    }

    static bool s_mines[c_boardSize[0] * c_boardSize[1]];
    static bool s_revealed[c_boardSize[0] * c_boardSize[1]];
    static bool s_flagged[c_boardSize[0] * c_boardSize[1]];
    static bool s_initialized;
};

bool Board::s_mines[c_boardSize[0] * c_boardSize[1]] = {};
bool Board::s_revealed[c_boardSize[0] * c_boardSize[1]] = {};
bool Board::s_flagged[c_boardSize[0] * c_boardSize[1]] = {};
bool Board::s_initialized = false;

int main(int argc, char** argv)
{
    Thirteen::SetApplicationName("Thirteen Demo - Minesweeper");

    unsigned char* pixels = Thirteen::Init(c_width, c_height, c_fullscreen);
    if (!pixels)
    {
        printf("Could not initialize Thirteen\n");
        return 1;
    }

    static const float c_aspectRatio = float(c_width) / float(c_height);

    bool dirty = true;
    Board::GameResult lastGameResult = Board::GameResult::Undecided;
    do
    {
        // V to toggle vsync
        if (Thirteen::GetKey('V') && !Thirteen::GetKeyLastFrame('V'))
            Thirteen::SetVSync(!Thirteen::GetVSync());

        // F to toggle full screen
        if (Thirteen::GetKey('F') && !Thirteen::GetKeyLastFrame('F'))
            Thirteen::SetFullscreen(!Thirteen::GetFullscreen());

        // S to save a screenshot
        if (Thirteen::GetKey('S') && !Thirteen::GetKeyLastFrame('S'))
            stbi_write_png("screenshot.png", c_width, c_height, 4, pixels, 0);

        // Initialize the board if we need to, or if space key was pressed to reset it
        if (!Board::s_initialized || (Thirteen::GetKey(VK_SPACE) && !Thirteen::GetKeyLastFrame(VK_SPACE)))
        {
            Board::Initialize();
            dirty = true;
        }

        // If the game is not yet over:
        // Left click to reveal. Right click to toggle flag.
        Board::GameResult gameResult = Board::GetGameResult();
        if (gameResult == Board::GameResult::Undecided)
        {
            if (Thirteen::GetMouseButton(0) && !Thirteen::GetMouseButtonLastFrame(0))
            {
                int clickPosX, clickPosY;
                Thirteen::GetMousePosition(clickPosX, clickPosY);
                int cellX, cellY;
                Board::ScreenPosToBoardPos(clickPosX, clickPosY, cellX, cellY);
                Board::OnLeftClick(cellX, cellY);
                dirty = true;
            }
            if (Thirteen::GetMouseButton(1) && !Thirteen::GetMouseButtonLastFrame(1))
            {
                int clickPosX, clickPosY;
                Thirteen::GetMousePosition(clickPosX, clickPosY);
                int cellX, cellY;
                Board::ScreenPosToBoardPos(clickPosX, clickPosY, cellX, cellY);
                Board::OnRightClick(cellX, cellY);
                dirty = true;
            }
        }

        // redraw the board if the game result changed
        if (lastGameResult != gameResult)
        {
            lastGameResult = gameResult;
            dirty = true;
        }

        // Draw the board, only when dirty
        if (dirty)
        {
            for (int iy = 0; iy < c_height; ++iy)
            {
                for (int ix = 0; ix < c_width; ++ix)
                {
                    size_t i = (iy * c_width + ix) * 4;
                    Board::DrawPixel(ix, iy, &pixels[i], gameResult);
                }
            }
            dirty = false;
        }
    }
    while (Thirteen::Render() && !Thirteen::GetKey(VK_ESCAPE));

    Thirteen::Shutdown();

    return 0;
}
