#include <raylib.h>
#include <cmath>
#include <string>
#include <array>
#include <unordered_map>
#include <sstream>
#include <utility>
#include <optional>
#include <cctype>
#include <algorithm>
#include <iostream>

const unsigned int WIDTH = 1024;
const unsigned int HEIGHT = 1024;

const unsigned int WINDOWWIDTH = 1200;

const unsigned int FPS = 30;

const std::string defaultBoard = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

using Board = std::array<std::array<char, 8>, 8>;

struct GameState
{
    Board board;

    char activeColor = 'w';

    bool wCastleKing = false;
    bool wCastleQueen = false;
    bool bCastleKing = false;
    bool bCastleQueen = false;

    std::optional<std::pair<int, int>> enPassant;

    int halfMove = 0;
    int fullMove = 1;
};

struct Move
{
    int fr, fc;
    int tr, tc;

    bool isEP = false;
    bool isCapture = false;
    char castle = 'n';
    bool pDouble = false;

    bool operator==(const Move& other) const
    {
        return (fr == other.fr && fc == other.fc && tr == other.tr && tc == other.tc);
    }
};

typedef struct PieceTextures
{
    std::unordered_map<char, Texture2D> textures;

    PieceTextures()
    {
        textures =
        {
            {'P', LoadTexture("Assets/wp.png")},
            {'R', LoadTexture("Assets/wr.png")},
            {'N', LoadTexture("Assets/wn.png")},
            {'B', LoadTexture("Assets/wb.png")},
            {'Q', LoadTexture("Assets/wq.png")},
            {'K', LoadTexture("Assets/wk.png")},
            {'p', LoadTexture("Assets/bp.png")},
            {'r', LoadTexture("Assets/br.png")},
            {'n', LoadTexture("Assets/bn.png")},
            {'b', LoadTexture("Assets/bb.png")},
            {'q', LoadTexture("Assets/bq.png")},
            {'k', LoadTexture("Assets/bk.png")}
        };
    }

    void drawPiece(char piece, float i, float j)
    {
        DrawTextureEx(textures[piece], Vector2{ i * WIDTH / 8, j * HEIGHT / 8 }, 0, 0.85333333333, WHITE);
    }

    void drawMaterial(char piece, float x, float y)
    {
        DrawTextureEx(textures[piece], Vector2{ x, y }, 0, 0.21333333333, WHITE);
    }
};

static GameState fenToState(std::string fen)
{
    GameState state;

    for (int i = 0;i < 8;++i) state.board.at(i).fill('.'); // fill board with spaces

    std::istringstream data(fen);

    std::string fenBoard;
    char activeColor;
    std::string castle;
    std::string enPassant;
    int half;
    int full;

    data >> fenBoard >> activeColor >> castle >> enPassant >> half >> full;

    int f = 0;
    int t = 0;

    while (t < 64 && f < fenBoard.size())  // fill board
    {
        char c = fenBoard.at(f);
        if (std::isdigit((unsigned char)c))
        {
            t += c - '0';
        }
        else if (c == '/') {}
        else
        {
            state.board.at(t / 8).at(t++ % 8) = c;
        }
        ++f;
    }
    state.activeColor = activeColor; // active color

    if (castle != "-")
    {
        for (char c : castle)
        {
            switch (c)
            {
            case 'K':
                state.wCastleKing = true;
                break;
            case 'Q':
                state.wCastleQueen = true;
                break;
            case 'k':
                state.bCastleKing = true;
                break;
            case 'q':
                state.bCastleQueen = true;
                break;
            }
        }
    }
    else
    {
        state.wCastleKing = false;
        state.wCastleQueen = false;
        state.bCastleKing = false;
        state.bCastleQueen = false;
    }

    state.enPassant = (enPassant == "-") ? std::nullopt : std::optional<std::pair<int, int>>{ {8 - (enPassant[1] - '0'), enPassant[0] - 'a'} }; // enpassant square if any

    state.halfMove = half; // half move counter
    state.fullMove = full; // full move counter

    return state;
}

static std::string stateToFen(const GameState& state)
{
    // "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string fen = "";
    for (int i = 0;i < 8;++i)
    {
        int spaces = 0;
        for (int j = 0;j < 8;++j)
        {
            char p = state.board.at(i).at(j);
            if (p == '.') ++spaces;
            else
            {
                if (spaces != 0)
                {
                    fen += (spaces + '0');
                    spaces = 0;
                }
                fen += p;
            }
        }
        if (spaces != 0)
        {
            fen += (spaces + '0');
        }
        if (i != 7) fen += '/';
    }

    fen += ' ';
    fen += state.activeColor;

    std::string castle = " ";

    if (state.wCastleKing) castle += 'K';
    if (state.wCastleQueen) castle += 'Q';
    if (state.bCastleKing) castle += 'k';
    if (state.bCastleQueen) castle += 'q';

    if (castle == " ") fen += " -";
    else fen += castle;

    fen += ' ';

    if (state.enPassant.has_value())
    {
        fen += char(state.enPassant->second + 'a');
        fen += '8' - state.enPassant->first;
    }

    else fen += '-';

    fen += ' ';
    fen += std::to_string(state.halfMove);
    fen += ' ';
    fen += std::to_string(state.fullMove);

    return fen;
}

std::vector<Move> getPawnMoves(int fr, int fc, GameState state)
{
    std::vector<Move> moves;
    char color = state.activeColor;

    if (color == 'w')
    {
        if (fr == 0) return moves;
        if (state.board.at(fr - 1).at(fc) == '.')
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr - 1, .tc = fc }); // forward 1
        }

        if (fr == 6 && state.board.at(fr - 2).at(fc) == '.' && state.board.at(fr - 1).at(fc) == '.') // forward 2
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr - 2, .tc = fc, .pDouble = true });
        }

        if (fc > 0 && state.board.at(fr - 1).at(fc - 1) != '.' && std::islower((unsigned char)state.board.at(fr - 1).at(fc - 1))) // diagonal top left
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr - 1, .tc = fc - 1, .isCapture = true });
        }
        else if (fc > 0 && state.enPassant && state.enPassant->first == fr - 1 && state.enPassant->second == fc - 1) // en passant top left
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr - 1, .tc = fc - 1, .isEP = true, .isCapture = true });
        }

        if (fc < 7 && state.board.at(fr - 1).at(fc + 1) != '.' && std::islower((unsigned char)state.board.at(fr - 1).at(fc + 1))) // diagonal top right
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr - 1, .tc = fc + 1, .isCapture = true });
        }
        else if (fc < 7 && state.enPassant && state.enPassant->first == fr - 1 && state.enPassant->second == fc + 1)// en passant top right
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr - 1, .tc = fc + 1, .isEP = true, .isCapture = true });
        }
    }

    else
    {
        if (fr == 7) return moves;
        if (state.board.at(fr + 1).at(fc) == '.')
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr + 1, .tc = fc });
        }

        if (fr == 1 && state.board.at(fr + 2).at(fc) == '.' && state.board.at(fr + 1).at(fc) == '.')
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr + 2, .tc = fc , .pDouble = true });
        }

        if (fc > 0 && state.board.at(fr + 1).at(fc - 1) != '.' && std::isupper((unsigned char)state.board.at(fr + 1).at(fc - 1)))
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr + 1, .tc = fc - 1, .isCapture = true });
        }
        else if (fc > 0 && state.enPassant && state.enPassant->first == fr + 1 && state.enPassant->second == fc - 1)
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr + 1, .tc = fc - 1 , .isEP = true, .isCapture = true });
        }

        if (fc < 7 && state.board.at(fr + 1).at(fc + 1) != '.' && std::isupper((unsigned char)state.board.at(fr + 1).at(fc + 1)))
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr + 1, .tc = fc + 1, .isCapture = true });
        }
        else if (fc < 7 && state.enPassant && state.enPassant->first == fr + 1 && state.enPassant->second == fc + 1)
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr + 1, .tc = fc + 1 , .isEP = true, .isCapture = true });
        }
    }

    return moves;
}

std::vector<Move> getKingMoves(int fr, int fc, GameState state)
{
    std::vector<Move> moves;

    int ar[] = { -1, -1, -1,  0, 0,  1, 1,  1 };
    int ac[] = { -1,  0,  1, -1, 1, -1, 0,  1 };

    for (int i = 0;i < 8;++i)
    {
        int tr = fr + ar[i];
        int tc = fc + ac[i];

        if (tr < 0 || tr > 7 || tc < 0 || tc > 7) continue;

        char d = state.board.at(tr).at(tc);

        bool isCapture = (d != '.');

        if (!isCapture || (state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, .isCapture = isCapture });
        }
    }

    if (state.activeColor == 'w')
    {
        if (state.wCastleKing)
        {
            if (state.board[fr][fc + 1] == '.' && state.board[fr][fc + 2] == '.')
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr, .tc = fc + 2, .castle = 'K' });
            }
        }

        if (state.wCastleQueen)
        {
            if (state.board[fr][fc - 1] == '.' && state.board[fr][fc - 2] == '.' && state.board[fr][fc - 3] == '.')
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr, .tc = fc - 2, .castle = 'Q' });
            }
        }
    }

    if (state.activeColor == 'b')
    {
        if (state.bCastleKing)
        {
            if (state.board[fr][fc + 1] == '.' && state.board[fr][fc + 2] == '.')
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr, .tc = fc + 2, .castle = 'k' });
            }
        }

        if (state.bCastleQueen)
        {
            if (state.board[fr][fc - 1] == '.' && state.board[fr][fc - 2] == '.' && state.board[fr][fc - 3] == '.')
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr, .tc = fc - 2, .castle = 'q' });
            }
        }
    }

    return moves;
}

std::vector<Move> getKnightMoves(int fr, int fc, GameState state)
{
    std::vector<Move> moves;

    int ar[] = { -2, -2, -1,  -1,  1,  1,  2,  2 };
    int ac[] = { -1,  1,  -2,  2, -2,  2, -1,  1 };

    for (int i = 0;i < 8;++i)
    {
        int tr = fr + ar[i];
        int tc = fc + ac[i];

        if (tr < 0 || tr > 7 || tc < 0 || tc > 7) continue;

        char d = state.board.at(tr).at(tc);

        bool isCapture = (d != '.');

        if (!isCapture || (state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
        {
            moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, .isCapture = isCapture });
        }
    }

    return moves;
}

std::vector<Move> getRookMoves(int fr, int fc, GameState state)
{
    std::vector<Move> moves;

    for (int i = 1;fr + i < 8;++i)
    {
        int tr = fr + i;
        char d = state.board[tr][fc];

        if (d == '.') moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = fc, });
        else
        {
            if ((state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = fc, .isCapture = true });
            }
            break;
        }
    }

    for (int i = 1;fr - i >= 0;++i)
    {
        int tr = fr - i;
        char d = state.board[tr][fc];

        if (d == '.') moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = fc, });
        else
        {
            if ((state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = fc, .isCapture = true });
            }
            break;
        }
    }

    for (int i = 1;fc + i < 8;++i)
    {
        int tc = fc + i;
        char d = state.board[fr][tc];

        if (d == '.') moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr, .tc = tc, });
        else
        {
            if ((state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr, .tc = tc, .isCapture = true });
            }
            break;
        }
    }

    for (int i = 1;fc - i >= 0;++i)
    {
        int tc = fc - i;
        char d = state.board[fr][tc];

        if (d == '.') moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr, .tc = tc, });
        else
        {
            if ((state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = fr, .tc = tc, .isCapture = true });
            }
            break;
        }
    }

    return moves;
}

std::vector<Move> getBishopMoves(int fr, int fc, GameState state)
{
    std::vector<Move> moves;
    int i = 1;
    while (fr + i <= 7 && fc + i <= 7)
    {
        int tr = fr + i;
        int tc = fc + i;
        char d = state.board[tr][tc];
        if (d == '.') moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, });
        else
        {
            if ((state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, .isCapture = true });
            }
            break;
        }
        ++i;
    }

    i = 1;
    while (fr + i <= 7 && fc - i >= 0)
    {
        int tr = fr + i;
        int tc = fc - i;
        char d = state.board[tr][tc];
        if (d == '.') moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, });
        else
        {
            if ((state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, .isCapture = true });
            }
            break;
        }
        ++i;
    }

    i = 1;
    while (fr - i >= 0 && fc - i >= 0)
    {
        int tr = fr - i;
        int tc = fc - i;
        char d = state.board[tr][tc];
        if (d == '.') moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, });
        else
        {
            if ((state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, .isCapture = true });
            }
            break;
        }
        ++i;
    }

    i = 1;
    while (fr - i >= 0 && fc + i <= 7)
    {
        int tr = fr - i;
        int tc = fc + i;
        char d = state.board[tr][tc];
        if (d == '.') moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, });
        else
        {
            if ((state.activeColor == 'w' && std::islower(d)) || (state.activeColor == 'b' && std::isupper(d)))
            {
                moves.push_back(Move{ .fr = fr, .fc = fc, .tr = tr, .tc = tc, .isCapture = true });
            }
            break;
        }
        ++i;
    }

    return moves;
}

std::vector<Move> getQueenMoves(int fr, int fc, GameState state)
{
    std::vector<Move> moves = getRookMoves(fr, fc, state);
    std::vector<Move> bishopMoves = getBishopMoves(fr, fc, state);
    moves.insert(moves.end(), bishopMoves.begin(), bishopMoves.end());

    return moves;
}

std::vector<Move> getPseudoLegalMoves(int fr, int fc, GameState state)
{
    switch (std::tolower(state.board[fr][fc]))
    {
    case 'p':
        return getPawnMoves(fr, fc, state);
    case 'k':
        return getKingMoves(fr, fc, state);
    case 'n':
        return getKnightMoves(fr, fc, state);
    case 'r':
        return getRookMoves(fr, fc, state);
    case 'b':
        return getBishopMoves(fr, fc, state);
    case 'q':
        return getQueenMoves(fr, fc, state);
    }
    return {};
}

bool scanCheck(GameState state)
{
    char color = state.activeColor;
    int fr = 20;
    int fc = 20;
    for (int i = 0;i <= 7; ++i)
    {
        for (int j = 0;j <= 7; ++j)
        {
            if ((state.board[i][j] == 'k' && color == 'b') || (state.board[i][j] == 'K' && color == 'w'))
            {
                fr = i;
                fc = j;
            }
        }
    }

    if (fr == 20 || fc == 20) return true;

    if (color == 'w')
    {
        if (fr != 0) // check for pawn checks
        {
            if (fc != 0) // top left pawn
            {
                if (state.board[fr - 1][fc - 1] == 'p') return true;
            }
            if (fc != 7) // top right pawn
            {
                if (state.board[fr - 1][fc + 1] == 'p') return true;
            }
        }

        std::vector<Move> knightMoves = getKnightMoves(fr, fc, state); // check for knight checks
        for (Move& m : knightMoves)
        {
            if (state.board[m.tr][m.tc] == 'n') return true;
        }

        std::vector<Move> rookMoves = getRookMoves(fr, fc, state); // check for rook checks
        for (Move& m : rookMoves)
        {
            if (state.board[m.tr][m.tc] == 'r') return true;
        }

        std::vector<Move> bishopMoves = getBishopMoves(fr, fc, state); // check for bishop checks
        for (Move& m : bishopMoves)
        {
            if (state.board[m.tr][m.tc] == 'b') return true;
        }

        std::vector<Move> queenMoves = getQueenMoves(fr, fc, state); // check for queen checks
        for (Move& m : queenMoves)
        {
            if (state.board[m.tr][m.tc] == 'q') return true;
        }
    }

    if (color == 'b')
    {
        if (fr != 7) // check for pawn checks
        {
            if (fc != 0) // top left pawn
            {
                if (state.board[fr + 1][fc - 1] == 'P') return true;
            }
            if (fc != 7) // top right pawn
            {
                if (state.board[fr + 1][fc + 1] == 'P') return true;
            }
        }

        std::vector<Move> knightMoves = getKnightMoves(fr, fc, state); // check for knight checks
        for (Move& m : knightMoves)
        {
            if (state.board[m.tr][m.tc] == 'N') return true;
        }

        std::vector<Move> rookMoves = getRookMoves(fr, fc, state); // check for rook checks
        for (Move& m : rookMoves)
        {
            if (state.board[m.tr][m.tc] == 'R') return true;
        }

        std::vector<Move> bishopMoves = getBishopMoves(fr, fc, state); // check for bishop checks
        for (Move& m : bishopMoves)
        {
            if (state.board[m.tr][m.tc] == 'B') return true;
        }

        std::vector<Move> queenMoves = getQueenMoves(fr, fc, state); // check for queen checks
        for (Move& m : queenMoves)
        {
            if (state.board[m.tr][m.tc] == 'Q') return true;
        }
    }



    return false;
}

std::vector<Move> filterLegalMoves(std::vector<Move> moves, GameState& state)
{
    if (!moves.empty())
    {
        for (auto it = moves.begin(); it != moves.end();)
        {
            Board cpy = state.board;
            char p = state.board.at(it->fr).at(it->fc);
            state.board[it->tr][it->tc] = p;

            if (state.board[it->tr][it->tc] == 'p' && it->tr == 7)
            {
                state.board[it->tr][it->tc] = 'q';
            }
            else if (state.board[it->tr][it->tc] == 'P' && it->tr == 0)
            {
                state.board[it->tr][it->tc] = 'Q';
            }
            else if (it->isEP)
            {
                state.board[it->fr][it->tc] = '.';
            }

            state.board[it->fr][it->fc] = '.';
            if (scanCheck(state))
            {
                it = moves.erase(it);
            }
            else
            {
                ++it;
            }
            state.board = cpy;
        }
    }

    return moves;
}

std::vector<Move> getAllLegalMoves(GameState& state)
{
    std::vector<Move> allLegalMoves;
    for (int i = 0;i < 8;++i)
    {
        for (int j = 0;j < 8;++j)
        {
            char curPiece = state.board[i][j];
            if ((state.activeColor == 'w' && std::isupper(curPiece)) || (state.activeColor == 'b' && std::islower(curPiece)))
            {
                std::vector<Move> newMoves;
                switch (std::tolower(curPiece))
                {
                case 'p':
                    newMoves = getPawnMoves(i, j, state);
                    break;
                case 'k':
                    newMoves = getKingMoves(i, j, state);
                    break;
                case 'n':
                    newMoves = getKnightMoves(i, j, state);
                    break;
                case 'r':
                    newMoves = getRookMoves(i, j, state);
                    break;
                case 'b':
                    newMoves = getBishopMoves(i, j, state);
                    break;
                case 'q':
                    newMoves = getQueenMoves(i, j, state);
                    break;
                }
                if (!newMoves.empty())
                {
                    allLegalMoves.insert(allLegalMoves.begin(), newMoves.begin(), newMoves.end());
                }
            }
        }
    }


    return filterLegalMoves(allLegalMoves, state);
}



class Game
{
private:
    int selX = 100;
    int selY = 100;

    std::vector<Move> moves;
    char playing = 'm'; // m == menu, p == pass and play, b == bot
    char winner = 'n'; // n == no winner, w == white wins, b == black wins, s == stalemate

    std::vector<std::string> previousMoves;
    int currentMove = 0;

    unsigned int whiteTime = 600; // ten minutes
    unsigned int blackTime = 600; // ten minutes

    float timeAccumulator = 0;

    std::vector<char> whiteCaptures;
    std::vector<char> blackCaptures;

    int material = 0;

    std::unordered_map<char, int> materialPoints =
    {
        {'p', 1},
        {'b', 3},
        {'n', 3},
        {'r', 5},
        {'q', 9},
        {'P', 1},
        {'B', 3},
        {'N', 3},
        {'R', 5},
        {'Q', 9}
    };

    char gameSelection = 'p';


public:
    GameState state;
    std::string fen;
    PieceTextures textures;
    bool inCheck = false;


    Game()
    {
        fen = defaultBoard;
        state = fenToState(defaultBoard);
        previousMoves = { fen };
        int currentMove = 0;
    }

    Game(std::string fen)
    {
        this->fen = fen;
        state = fenToState(fen);
        previousMoves = { fen };
        int currentMove = 0;
    }

    GameState tryMove(Move m, const std::vector<Move>& movesList, GameState& state)
    {
        auto it = std::find(movesList.begin(), movesList.end(), m);
        if (it != movesList.end())
        {
            // Apply move
            char moveCopy = state.board[it->tr][it->tc];

            // Update material. While it is possible to also handle en passant
            // captures here, I decided to seperate it
            char cap = state.board[it->tr][it->tc];
            if (it->isCapture)
            {
                if (state.activeColor == 'w')
                {
                    if (it->isEP)
                    {
                        whiteCaptures.push_back('p');
                        material += 1;
                        state.board[it->fr][it->tc] = '.';
                    }
                    else
                    {
                        whiteCaptures.push_back(cap);
                        material += materialPoints.at(cap);
                    }
                }
                else
                {
                    if (it->isEP)
                    {
                        blackCaptures.push_back('P');
                        material -= 1;
                        state.board[it->fr][it->tc] = '.';
                    }
                    else
                    {
                        blackCaptures.push_back(cap);
                        material -= materialPoints.at(cap);
                    }
                }
            }



            state.board[it->tr][it->tc] = state.board[it->fr][it->fc];

            char moving = state.board[it->fr][it->fc];

            if (it->pDouble)
            {
                if (state.activeColor == 'w') state.enPassant = std::make_pair(it->tr + 1, it->fc);
                else state.enPassant = std::make_pair(it->tr - 1, it->fc);
            }
            else
            {
                state.enPassant = std::nullopt;
            }

            // update castling rights
            if (moving == 'K')
            {
                state.wCastleKing = false;
                state.wCastleQueen = false;
            }

            else if (moving == 'k')
            {
                state.bCastleKing = false;
                state.bCastleQueen = false;
            }

            else if (moving == 'R')
            {
                if (it->fr == 7 && it->fc == 7) state.wCastleKing = false;
                else if (it->fr == 7 && it->fc == 0) state.wCastleQueen = false;
            }

            else if (moving == 'r')
            {
                if (it->fr == 0 && it->fc == 7) state.bCastleKing = false;
                else if (it->fr == 0 && it->fc == 0) state.bCastleQueen = false;
            }

            if (moveCopy == 'R')
            {
                if (it->tr == 7 && it->tc == 7) state.wCastleKing = false;
                if (it->tr == 7 && it->tc == 0) state.wCastleQueen = false;
            }

            else if (moveCopy == 'r')
            {
                if (it->tr == 0 && it->tc == 7) state.bCastleKing = false;
                if (it->tr == 0 && it->tc == 0) state.bCastleQueen = false;
            }

            // castle + update castling rights
            if (it->castle != 'n')
            {
                switch (it->castle)
                {
                case 'K':
                    state.board[it->tr][it->fc + 1] = 'R';
                    state.board[7][7] = '.';
                    state.wCastleKing = false;
                    break;
                case 'Q':
                    state.board[it->tr][it->fc - 1] = 'R';
                    state.board[7][0] = '.';
                    state.wCastleQueen = false;
                    break;
                case 'k':
                    state.board[it->tr][it->fc + 1] = 'r';
                    state.board[0][7] = '.';
                    state.bCastleKing = false;
                    break;
                case 'q':
                    state.board[it->tr][it->fc - 1] = 'r';
                    state.board[0][0] = '.';
                    state.bCastleQueen = false;
                    break;
                }
            }

            state.board[it->fr][it->fc] = '.';

            // En passant captures
            if (it->isEP)
            {
                state.board[it->fr][it->tc] = '.';
            }

            // Auto promote -> Queen
            if (moving == 'p' && it->tr == 7)
            {
                state.board[it->tr][it->tc] = 'q';
                material -= 9;
            }
            else if (moving == 'P' && it->tr == 0)
            {
                state.board[it->tr][it->tc] = 'Q';
                material += 9;
            }

            if (state.activeColor == 'b') ++state.fullMove;
            if (it->isCapture || (moving == 'p' || moving == 'P')) state.halfMove = 0;
            else ++state.halfMove;

            state.activeColor = (state.activeColor == 'w') ? 'b' : 'w';

            inCheck = scanCheck(state);
            // std::cout << sc << '\n';

            // check for winner or stalemate
            std::vector<Move> allLegalMoves = getAllLegalMoves(state);

            if (allLegalMoves.empty())
            {
                playing = 'n';
                if (inCheck) winner = (state.activeColor == 'w') ? 'b' : 'w';
                else winner = 's';
            }

            return state;
        }
    }

    void handleMenuKeys()
    {
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_UP))
        {
            if (gameSelection == 'p') gameSelection = 'b';
            else gameSelection = 'p';
        }

        else if (IsKeyPressed(KEY_ENTER))
        {
            playing = gameSelection;
        }
    }

    void handleTime()
    {
        timeAccumulator += GetFrameTime();
        if (timeAccumulator >= 1.0f) // Calculate time usage
        {
            if (state.activeColor == 'w')
            {
                whiteTime -= 1;
                if (whiteTime <= 0)
                {
                    winner = 'b';
                    playing = 'n';
                }
            }
            else
            {
                blackTime -= 1;
                if (blackTime <= 0)
                {
                    winner = 'w';
                    playing = 'n';
                }
            }

            timeAccumulator -= 1.0f;
        }
    }

    void restart()
    {
        fen = defaultBoard;
        state = fenToState(defaultBoard);
        winner = 'n';
        playing = gameSelection;
        selX = 100;
        selY = 100;
        previousMoves = { fen };
        currentMove = 0;
        whiteTime = 600;
        blackTime = 600;
        material = 0;
        whiteCaptures = {};
        blackCaptures = {};
        inCheck = false;
    }

    void rewindMove()
    {
        state = fenToState(previousMoves[--currentMove]);
        playing = 'n';
        winner = 'n';
        inCheck = scanCheck(state);
    }

    void forwardMove()
    {
        state = fenToState(previousMoves[++currentMove]);
        playing = (currentMove == previousMoves.size() - 1) ? 'p' : 'n';
        inCheck = scanCheck(state);
        std::vector<Move> allLegalMoves = getAllLegalMoves(state);
        if (allLegalMoves.empty())
        {
            playing = 'n';
            if (inCheck) winner = (state.activeColor == 'w') ? 'b' : 'w';
            else winner = 's';
        }
    }

    void appendMove()
    {
        std::string newFen = stateToFen(state);
        previousMoves.push_back(newFen);
        ++currentMove;
    }

    void update()
    {
        if (playing == 'm') // Menu -> Choose game
        {
            handleMenuKeys();
        }

        else
        {
            if (playing == 'p') // Update time
            {
                handleTime();
            }

            if (IsKeyPressed(KEY_R)) // Restart Game
            {
                restart();
            }

            if (IsKeyPressed(KEY_LEFT) && currentMove > 0) // Go backwards one move
            {
                rewindMove();
                return;
            }

            if (IsKeyPressed(KEY_RIGHT) && currentMove < (previousMoves.size() - 1)) // Go forwards one move
            {
                forwardMove();
                return;
            }
            // Piece selection
            int x = GetMouseX() / (WIDTH / 8);
            int y = GetMouseY() / (HEIGHT / 8);

            if (IsMouseButtonPressed(0) && playing != 'm' && x <= 7 && y <= 7)
            {
                if (state.activeColor == 'b')
                {
                    x = 7 - x;
                    y = 7 - y;
                }

                if (x >= 8 || y >= 8 || x < 0 || y < 0) return;

                char p = state.board.at(y).at(x);
                // std::cout << y << ", " << x << p << '\n';

                if (((p >= 'A' && p <= 'Z') && (state.activeColor == 'w')) || ((p >= 'a' && p <= 'z') && (state.activeColor == 'b'))) // Check if piece is same as thy color
                {
                    selX = x;
                    selY = y;

                    moves = getPseudoLegalMoves(selY, selX, state);

                    // filter legal moves
                    moves = filterLegalMoves(moves, state);
                }

                else
                {
                    Move m = { .fr = selY, .fc = selX, .tr = y, .tc = x };
                    if (selX + selY != 200 && std::find(moves.begin(), moves.end(), m) != moves.end())
                    {
                        state = tryMove(m, moves, state);
                        selX = 100;
                        selY = 100;
                        moves = {};

                        appendMove();

                        if (playing == 'b')
                        {
                            std::vector<Move> botMoves = getAllLegalMoves(state);
                            if (botMoves.empty())
                            {
                                playing = 'n';
                                if (inCheck) winner = (state.activeColor == 'w') ? 'b' : 'w';
                                else winner = 's';
                            }

                            else if (playing != 'n') state = tryMove(botMoves[0], botMoves, state);



                            appendMove();
                        }
                    }
                }
            }
        }
    }

    void drawPromotion()
    {
        if (state.activeColor == 'w')
        {
            textures.drawPiece('Q', 3, 8);
            textures.drawPiece('R', 4, 8);
            textures.drawPiece('B', 5, 8);
            textures.drawPiece('N', 6, 8);
        }
        else
        {
            textures.drawPiece('q', 3, 8);
            textures.drawPiece('r', 4, 8);
            textures.drawPiece('b', 5, 8);
            textures.drawPiece('n', 6, 8);
        }
    }

    void drawTime()
    {
        // Draw background
        DrawRectangle(WIDTH, 0, WINDOWWIDTH - WIDTH, HEIGHT, BLACK);

        // Make time strings
        std::string whiteTimeString;
        std::string blackTimeString;

        std::string whiteSeconds = std::to_string(whiteTime % 60);
        std::string blackSeconds = std::to_string(blackTime % 60);

        if (whiteTime % 60 < 10) whiteSeconds = "0" + std::to_string(whiteTime % 60);
        if (blackTime % 60 < 10) blackSeconds = "0" + std::to_string(blackTime % 60);

        whiteTimeString = std::to_string(whiteTime / 60) + ":" + whiteSeconds;
        blackTimeString = std::to_string(blackTime / 60) + ":" + blackSeconds;


        Color whiteTimeColor = (winner == 'b') ? RED : RAYWHITE;
        Color blackTimeColor = (winner == 'w') ? RED : RAYWHITE;

        // Display on correct side
        if (state.activeColor == 'w')
        {
            DrawText(blackTimeString.c_str(), WIDTH + 50, 200, 40, blackTimeColor);
            DrawText(whiteTimeString.c_str(), WIDTH + 50, HEIGHT - 200, 40, whiteTimeColor);
        }
        else
        {
            DrawText(whiteTimeString.c_str(), WIDTH + 50, 200, 40, whiteTimeColor);
            DrawText(blackTimeString.c_str(), WIDTH + 50, HEIGHT - 200, 40, blackTimeColor);
        }
    }

    void drawCaptures()
    {
        float x = WIDTH;
        float y = (state.activeColor == 'w') ? 875 : 68;

        float bx = WIDTH;
        float by = (y == 875) ? 68 : 875;

        int offset = 32;

        for (char c : whiteCaptures)
        {
            textures.drawMaterial(c, x, y);
            x += offset;
            if (x + offset > WINDOWWIDTH)
            {
                y += offset;
                x = WIDTH;
            }
        }
        if (material > 0) DrawText(('+' + std::to_string(material)).c_str(), WIDTH, y + offset, 30, WHITE);

        for (char b : blackCaptures)
        {
            textures.drawMaterial(b, bx, by);
            bx += offset;
            if (bx + offset > WINDOWWIDTH)
            {
                by += offset;
                bx = WIDTH;
            }
        }
        if (material < 0) DrawText(('+' + std::to_string(-material)).c_str(), WIDTH, by + offset, 30, WHITE);
    }

    void drawBoard()
    {
        for (int i = 0;i < 8;++i)
        {
            for (int j = 0;j < 8;++j)
            {
                int index = i;
                int jndex = j;

                // flip board
                if (state.activeColor == 'b' && playing != 'b')
                {
                    index = 7 - i;
                    jndex = 7 - j;
                }

                int coordX = jndex * WIDTH / 8;
                int coordY = index * HEIGHT / 8;

                char p = state.board.at(i).at(j);
                if ((i + j) % 2) DrawRectangle(coordX, coordY, WIDTH / 8, HEIGHT / 8, BROWN);
                if (i == selY && j == selX) DrawRectangle(coordX, coordY, WIDTH / 8, HEIGHT / 8, GREEN);
                if (inCheck && ((state.board[i][j] == 'k' && state.activeColor == 'b') || (state.board[i][j] == 'K' && state.activeColor == 'w')))
                {
                    DrawRectangle(coordX, coordY, WIDTH / 8, HEIGHT / 8, RED);
                }

                if ((winner == 'w' && p == 'K') || (winner == 'b' && p == 'k') || (winner == 's' && std::tolower(p) == 'k'))
                {
                    DrawRectangle(coordX, coordY, WIDTH / 8, HEIGHT / 8, GOLD);
                }

                if (p != '.') textures.drawPiece(p, jndex, index);


                // std::cout << p << ", ";
            }
            // std::cout << '\n';
        }
        if (!moves.empty())
        {
            for (const Move& m : moves)
            {
                // flip board
                int tc = m.tc;
                int tr = m.tr;
                if (state.activeColor == 'b')
                {
                    tc = 7 - m.tc;
                    tr = 7 - m.tr;
                }
                DrawCircle((tc * WIDTH / 8) + WIDTH / 16, (tr * HEIGHT / 8) + +HEIGHT / 16, 32, Color{ 128, 128, 128, 128 });
            }
        }
    }

    void drawMenu()
    {
        int titleFontSize = 150;
        int optionFontSize = titleFontSize / 2.5;

        Color passColor = (gameSelection == 'p') ? GREEN : LIME;
        Color botColor = (gameSelection == 'b') ? GREEN : LIME;

        int titleSize = MeasureTextEx(GetFontDefault(), "CHESS", titleFontSize, titleFontSize * .1f).x;
        DrawText("CHESS", WINDOWWIDTH / 2 - (titleSize / 2), 100, titleFontSize, LIME);

        int passSize = MeasureTextEx(GetFontDefault(), "PASS AND PLAY", optionFontSize, optionFontSize * .1f).x;
        DrawText("PASS AND PLAY", WINDOWWIDTH / 2 - (passSize / 2), 500, optionFontSize, passColor);

        int botSize = MeasureTextEx(GetFontDefault(), "BOT", optionFontSize, optionFontSize * .1f).x;
        DrawText("BOT", WINDOWWIDTH / 2 - (botSize / 2), 700, optionFontSize, botColor);
    }

    void draw()
    {
        if (playing == 'm')
        {
            DrawRectangle(0, 0, WINDOWWIDTH, HEIGHT, DARKGREEN);

            drawMenu();
        }

        else
        {
            drawBoard();
            if(playing == 'p') drawTime(); // No time for bot game
            drawCaptures();
        }
    }
};

int main()
{
    InitWindow(WINDOWWIDTH, HEIGHT, "Chess");
    SetTargetFPS(FPS);

    Game game(defaultBoard);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        game.update();
        game.draw();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
