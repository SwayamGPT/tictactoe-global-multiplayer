#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <cmath>
#include <iostream>
#include <vector>

using namespace sf;
using namespace std;

// ================= CONSTANTS & SETTINGS =================
const int WINDOW_W = 400;
const int WINDOW_H = 580;
const int BOARD_OFFSET_X = 50;
const int BOARD_OFFSET_Y = 210;
const int CELL_SIZE = 100;
const unsigned short PORT = 53000;

// ================= PACKET PROTOCOL =================
// Must perfectly match the server's protocol!
enum PacketType {
  JOIN_LOBBY = 0,
  LOBBY_UPDATE = 1,
  MATCH_START = 2,
  MAKE_MOVE = 3,
  BOARD_UPDATE = 4,
  MATCH_OVER = 5,
  TOURNEY_OVER = 6
};

enum Difficulty { EASY = 30, MEDIUM = 70, HARD = 100 };

// ================= UI & LOGIC CLASSES =================
class Button {
public:
  RectangleShape shape;
  Text text;

  Button() {}
  Button(Vector2f size, Vector2f pos, string str, Font &font) {
    shape.setSize(size);
    shape.setPosition(pos);
    shape.setFillColor(Color(45, 45, 65));
    shape.setOutlineThickness(2);
    shape.setOutlineColor(Color(80, 80, 120));

    text.setFont(font);
    text.setString(str);
    text.setCharacterSize(20);

    FloatRect textRect = text.getLocalBounds();
    text.setOrigin(textRect.left + textRect.width / 2.0f,
                   textRect.top + textRect.height / 2.0f);
    text.setPosition(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
  }

  void update(Vector2i mouse) {
    if (shape.getGlobalBounds().contains((Vector2f)mouse)) {
      shape.setFillColor(Color(70, 70, 100));
      shape.setOutlineColor(Color(0, 200, 255));
      shape.setScale(1.02f, 1.02f);
    } else {
      shape.setFillColor(Color(45, 45, 65));
      shape.setOutlineColor(Color(80, 80, 120));
      shape.setScale(1.f, 1.f);
    }
  }

  void setText(string str) {
    text.setString(str);
    FloatRect textRect = text.getLocalBounds();
    text.setOrigin(textRect.left + textRect.width / 2.0f,
                   textRect.top + textRect.height / 2.0f);
  }

  bool isClicked(Vector2i mouse) {
    return shape.getGlobalBounds().contains((Vector2f)mouse);
  }
  void draw(RenderWindow &w) {
    w.draw(shape);
    w.draw(text);
  }
};

class Board {
public:
  char grid[3][3];
  vector<Vector2i> winCells;

  Board() { reset(); }

  void reset() {
    winCells.clear();
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        grid[i][j] = ' ';
  }

  bool move(int r, int c, char p) {
    if (grid[r][c] == ' ') {
      grid[r][c] = p;
      return true;
    }
    return false;
  }

  bool movesLeft() {
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        if (grid[i][j] == ' ')
          return true;
    return false;
  }

  char checkWin() {
    winCells.clear();
    for (int i = 0; i < 3; i++) {
      if (grid[i][0] != ' ' && grid[i][0] == grid[i][1] &&
          grid[i][1] == grid[i][2]) {
        winCells = {{i, 0}, {i, 1}, {i, 2}};
        return grid[i][0];
      }
      if (grid[0][i] != ' ' && grid[0][i] == grid[1][i] &&
          grid[1][i] == grid[2][i]) {
        winCells = {{0, i}, {1, i}, {2, i}};
        return grid[0][i];
      }
    }
    if (grid[0][0] != ' ' && grid[0][0] == grid[1][1] &&
        grid[1][1] == grid[2][2]) {
      winCells = {{0, 0}, {1, 1}, {2, 2}};
      return grid[0][0];
    }
    if (grid[0][2] != ' ' && grid[0][2] == grid[1][1] &&
        grid[1][1] == grid[2][0]) {
      winCells = {{0, 2}, {1, 1}, {2, 0}};
      return grid[0][2];
    }
    return ' ';
  }
};

class AI {
private:
  int minimax(Board &b, bool isMax) {
    char w = b.checkWin();
    if (w == 'O')
      return 10;
    if (w == 'X')
      return -10;
    if (!b.movesLeft())
      return 0;

    int best = isMax ? -1000 : 1000;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        if (b.grid[i][j] == ' ') {
          b.grid[i][j] = isMax ? 'O' : 'X';
          if (isMax)
            best = max(best, minimax(b, false));
          else
            best = min(best, minimax(b, true));
          b.grid[i][j] = ' ';
        }
      }
    }
    return best;
  }
  void randomMove(Board &b) {
    vector<Vector2i> free;
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        if (b.grid[i][j] == ' ')
          free.push_back({i, j});

    if (!free.empty()) {
      auto m = free[rand() % free.size()];
      b.grid[m.x][m.y] = 'O';
    }
  }
  void bestMove(Board &b) {
    int bestVal = -1000, r = -1, c = -1;
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        if (b.grid[i][j] == ' ') {
          b.grid[i][j] = 'O';
          int val = minimax(b, false);
          b.grid[i][j] = ' ';
          if (val > bestVal) {
            bestVal = val;
            r = i;
            c = j;
          }
        }
    if (r != -1 && c != -1)
      b.grid[r][c] = 'O';
  }

public:
  void playMove(Board &b, Difficulty diff) {
    int chance = rand() % 100;
    if (chance < diff)
      bestMove(b);
    else
      randomMove(b);
  }
};

// ================= MAIN GAME LOOP =================
class Game {
public:
  enum State {
    MENU,
    ENTER_IP,
    ENTER_NAME,
    LOBBY,
    PLAYING,
    GAME_OVER,
    WAITING_CONNECTION
  };
  enum Mode { LOCAL, AI_MODE, NET_CLIENT }; // Host mode removed!

  State state = MENU;
  Mode mode = AI_MODE;
  Difficulty aiDiff = HARD;

  Board board;
  AI ai;

  // Networking Variables
  bool playerTurn = true;
  char myPiece = 'X';
  string result = "";
  string ipInput = "127.0.0.1"; // Default to local for easy testing
  string nameInput = "";
  string opponentName = "";
  int lobbyCount = 0;
  int lobbyRequired = 0;

  TcpSocket socket;

  Font font;
  Button btnAI, btnDiff, btnLocal, btnJoinTourney, btnConnect, btnPlayAgain,
      btnMainMenu;

  float animScale[3][3] = {0};
  float animAlpha[3][3] = {0};
  Clock clock;

  Game() {
    if (!font.loadFromFile("arial.ttf"))
      if (!font.loadFromFile("../Resources/arial.ttf"))
        font.loadFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");

    float startY = 150;
    btnAI = Button({250, 45}, {75, startY}, "Play vs AI", font);
    btnDiff = Button({250, 45}, {75, startY + 60}, "AI: Hard", font);
    btnLocal = Button({250, 45}, {75, startY + 120}, "2 Player Local", font);
    btnJoinTourney =
        Button({250, 45}, {75, startY + 180}, "Join Tournament", font);

    btnConnect = Button({200, 45}, {100, 300}, "Next", font);
    btnPlayAgain = Button({200, 45}, {100, 320}, "Play Again", font);
    btnMainMenu = Button({200, 45}, {100, 380}, "Main Menu", font);

    srand(time(0));
  }

  void reset() {
    board.reset();
    playerTurn = true;
    result = "";
    state = PLAYING;
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) {
        animScale[i][j] = 0;
        animAlpha[i][j] = 0;
      }
  }

  void handleText(Uint32 unicode) {
    if (state == ENTER_IP) {
      if (unicode == '\b' && !ipInput.empty())
        ipInput.pop_back();
      else if (unicode == '\r' || unicode == '\n')
        state = ENTER_NAME;
      else if (unicode >= 32 && unicode < 128 && ipInput.size() < 15)
        ipInput += static_cast<char>(unicode);
    } else if (state == ENTER_NAME) {
      if (unicode == '\b' && !nameInput.empty())
        nameInput.pop_back();
      else if (unicode == '\r' || unicode == '\n')
        joinServer();
      else if (unicode >= 32 && unicode < 128 && nameInput.size() < 10)
        nameInput += static_cast<char>(unicode);
    }
  }

  void joinServer() {
    if (nameInput.empty())
      nameInput = "Player";
    mode = NET_CLIENT;
    state = WAITING_CONNECTION;
    socket.setBlocking(true);

    if (socket.connect(ipInput, PORT, seconds(3)) == sf::Socket::Done) {
      socket.setBlocking(false);

      // Send Name to Server
      sf::Packet packet;
      packet << JOIN_LOBBY << nameInput;
      socket.send(packet);

      state = LOBBY;
    } else {
      result = "Connection Failed!";
      state = GAME_OVER;
    }
  }

  void handleClick(Vector2i m) {
    if (state == MENU) {
      if (btnAI.isClicked(m)) {
        mode = AI_MODE;
        reset();
      } else if (btnDiff.isClicked(m)) {
        if (aiDiff == HARD) {
          aiDiff = EASY;
          btnDiff.setText("AI: Easy");
        } else if (aiDiff == EASY) {
          aiDiff = MEDIUM;
          btnDiff.setText("AI: Medium");
        } else {
          aiDiff = HARD;
          btnDiff.setText("AI: Hard");
        }
      } else if (btnLocal.isClicked(m)) {
        mode = LOCAL;
        reset();
      } else if (btnJoinTourney.isClicked(m)) {
        state = ENTER_IP;
      }
    } else if (state == ENTER_IP) {
      if (btnConnect.isClicked(m))
        state = ENTER_NAME;
    } else if (state == ENTER_NAME) {
      if (btnConnect.isClicked(m))
        joinServer();
    } else if (state == PLAYING) {
      if (m.x >= BOARD_OFFSET_X && m.x <= BOARD_OFFSET_X + 300 &&
          m.y >= BOARD_OFFSET_Y && m.y <= BOARD_OFFSET_Y + 300) {

        int x = (m.x - BOARD_OFFSET_X) / CELL_SIZE;
        int y = (m.y - BOARD_OFFSET_Y) / CELL_SIZE;

        if (board.grid[y][x] == ' ') {
          if (mode == NET_CLIENT) {
            // In tournament, only send the move to the server. Do not draw it
            // yet! The server will validate it and broadcast the BOARD_UPDATE
            // back.
            if (playerTurn) {
              sf::Packet packet;
              packet << MAKE_MOVE << y << x;
              socket.send(packet);
            }
          } else {
            // Local Modes
            board.move(y, x, playerTurn ? 'X' : 'O');
            playerTurn = !playerTurn;
          }
        }
      }
    } else if (state == GAME_OVER) {
      if (btnMainMenu.isClicked(m)) {
        socket.disconnect();
        state = MENU;
      } else if (mode != NET_CLIENT && btnPlayAgain.isClicked(m)) {
        reset(); // Local play again
      }
    }
  }

  void update() {
    float dt = clock.restart().asSeconds();

    // Animations
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        if (board.grid[i][j] != ' ') {
          animScale[i][j] += dt * 5;
          animAlpha[i][j] += dt * 400;
          if (animScale[i][j] > 1.0f)
            animScale[i][j] = 1.0f;
          if (animAlpha[i][j] > 255.0f)
            animAlpha[i][j] = 255.0f;
        }

    // ================= TOURNAMENT NETWORK LOGIC =================
    if (mode == NET_CLIENT && state != MENU && state != ENTER_IP &&
        state != ENTER_NAME) {
      sf::Packet packet;
      if (socket.receive(packet) == sf::Socket::Done) {
        int typeInt;
        if (packet >> typeInt) {
          PacketType type = static_cast<PacketType>(typeInt);

          if (type == LOBBY_UPDATE) {
            packet >> lobbyCount >> lobbyRequired;
          } else if (type == MATCH_START) {
            bool goFirst;
            packet >> opponentName >> goFirst;
            myPiece = goFirst ? 'X' : 'O';
            playerTurn = goFirst;
            reset(); // Clear board for new match
          } else if (type == BOARD_UPDATE) {
                        int r, c; 
                        sf::Int8 pInt; // Extract safely as a strict 8-bit integer
                        packet >> r >> c >> pInt;
                        
                        char p = static_cast<char>(pInt); // Convert back to character
                        board.move(r, c, p);
                        playerTurn = (p != myPiece); // Switch turn
                    } else if (type == MATCH_OVER) {
            packet >> result;
            state = GAME_OVER;
          } else if (type == TOURNEY_OVER) {
            result = "Tournament Complete!";
            state = GAME_OVER;
          }
        }
      } else if (socket.receive(packet) == sf::Socket::Disconnected) {
        result = "Lost Connection to Server.";
        state = GAME_OVER;
      }
    }

    // ================= LOCAL LOGIC ONLY =================
    if (mode != NET_CLIENT && state == PLAYING) {
      char w = board.checkWin();
      if (w != ' ') {
        result = (w == 'X' ? "Player 1 Wins!" : "Player 2 Wins!");
        state = GAME_OVER;
      } else if (!board.movesLeft()) {
        result = "It's a Draw!";
        state = GAME_OVER;
      } else if (mode == AI_MODE && !playerTurn) {
        ai.playMove(board, aiDiff);
        playerTurn = true;
      }
    }
  }

  void draw(RenderWindow &w) {
    VertexArray bg(Quads, 4);
    bg[0].position = Vector2f(0, 0);
    bg[0].color = Color(20, 20, 35);
    bg[1].position = Vector2f(WINDOW_W, 0);
    bg[1].color = Color(35, 25, 45);
    bg[2].position = Vector2f(WINDOW_W, WINDOW_H);
    bg[2].color = Color(10, 10, 20);
    bg[3].position = Vector2f(0, WINDOW_H);
    bg[3].color = Color(15, 10, 25);
    w.draw(bg);

    Text title("TIC TAC TOE", font, 36);
    title.setStyle(Text::Bold);
    title.setFillColor(Color::White);
    FloatRect tb = title.getLocalBounds();
    title.setOrigin(tb.left + tb.width / 2.0f, tb.top + tb.height / 2.0f);
    title.setPosition(WINDOW_W / 2.0f, 60);
    w.draw(title);

    if (state == MENU) {
      Vector2i mouse = Mouse::getPosition(w);
      btnAI.update(mouse);
      btnDiff.update(mouse);
      btnLocal.update(mouse);
      btnJoinTourney.update(mouse);

      btnAI.draw(w);
      btnDiff.draw(w);
      btnLocal.draw(w);
      btnJoinTourney.draw(w);
      return;
    }

    if (state == ENTER_IP || state == ENTER_NAME) {
      string prStr =
          (state == ENTER_IP) ? "Enter Server IP Address:" : "Enter Your Name:";
      Text prompt(prStr, font, 20);
      prompt.setPosition(50, 180);
      w.draw(prompt);

      RectangleShape inputBox({300, 40});
      inputBox.setPosition(50, 220);
      inputBox.setFillColor(Color(30, 30, 45));
      inputBox.setOutlineThickness(2);
      inputBox.setOutlineColor(Color(100, 100, 150));
      w.draw(inputBox);

      string typedStr = (state == ENTER_IP) ? ipInput : nameInput;
      Text txt(typedStr + "_", font, 22);
      txt.setPosition(60, 225);
      w.draw(txt);

      btnConnect.setText(state == ENTER_IP ? "Next" : "Join Lobby");
      btnConnect.update(Mouse::getPosition(w));
      btnConnect.draw(w);
      return;
    }

    if (state == WAITING_CONNECTION || state == LOBBY) {
      string msg = (state == WAITING_CONNECTION)
                       ? "Connecting..."
                       : "LOBBY\n\nWaiting for players...\n" +
                             to_string(lobbyCount) + " / " +
                             to_string(lobbyRequired);

      Text wait(msg, font, 22);
      FloatRect wb = wait.getLocalBounds();
      wait.setOrigin(wb.left + wb.width / 2.0f, wb.top + wb.height / 2.0f);
      wait.setPosition(WINDOW_W / 2.0f, 250);
      wait.setFillColor(Color(0, 200, 255));
      w.draw(wait);
      return;
    }

    // Draw Player Headers during Match
    if (mode == NET_CLIENT) {
      string header = "You (" + string(1, myPiece) + ")  vs  " + opponentName;
      Text vsText(header, font, 18);
      vsText.setFillColor(Color(150, 150, 200));
      vsText.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y - 40);
      w.draw(vsText);

      string turnStr = playerTurn ? "Your Turn!" : "Waiting for opponent...";
      Text turnText(turnStr, font, 18);
      turnText.setFillColor(playerTurn ? Color(0, 255, 150)
                                       : Color(200, 100, 100));
      turnText.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y + 310);
      w.draw(turnText);
    }

    // Draw Grid
    RectangleShape line({300, 4});
    line.setFillColor(Color(100, 100, 140, 150));
    for (int i = 1; i < 3; i++) {
      line.setPosition(BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_SIZE - 2);
      w.draw(line);
      line.setSize({4, 300});
      line.setPosition(BOARD_OFFSET_X + i * CELL_SIZE - 2, BOARD_OFFSET_Y);
      w.draw(line);
      line.setSize({300, 4});
    }

    // Draw Pieces
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        if (board.grid[i][j] != ' ') {
          float cx = BOARD_OFFSET_X + j * CELL_SIZE + CELL_SIZE / 2.0f;
          float cy = BOARD_OFFSET_Y + i * CELL_SIZE + CELL_SIZE / 2.0f;

          if (board.grid[i][j] == 'X') {
            RectangleShape xLine({70, 8});
            xLine.setOrigin(35, 4);
            xLine.setPosition(cx, cy);
            xLine.setScale(animScale[i][j], animScale[i][j]);
            xLine.setFillColor(Color(0, 200, 255, (int)animAlpha[i][j]));
            xLine.setRotation(45);
            w.draw(xLine);
            xLine.setRotation(-45);
            w.draw(xLine);
          } else {
            CircleShape oShape(25);
            oShape.setOrigin(25, 25);
            oShape.setPosition(cx, cy);
            oShape.setScale(animScale[i][j], animScale[i][j]);
            oShape.setFillColor(Color::Transparent);
            oShape.setOutlineThickness(8);
            oShape.setOutlineColor(Color(255, 80, 80, (int)animAlpha[i][j]));
            w.draw(oShape);
          }
        }
      }
    }

    if (board.winCells.size() == 3) {
      Vector2f s(BOARD_OFFSET_X + board.winCells[0].y * CELL_SIZE + 50,
                 BOARD_OFFSET_Y + board.winCells[0].x * CELL_SIZE + 50);
      Vector2f e(BOARD_OFFSET_X + board.winCells[2].y * CELL_SIZE + 50,
                 BOARD_OFFSET_Y + board.winCells[2].x * CELL_SIZE + 50);
      RectangleShape winLine({hypot(e.x - s.x, e.y - s.y) + 40, 8});
      winLine.setOrigin(20, 4);
      winLine.setPosition(s);
      winLine.setRotation(atan2(e.y - s.y, e.x - s.x) * 180 / 3.14159f);
      winLine.setFillColor(Color(0, 255, 150, 220));
      w.draw(winLine);
    }

    if (state == GAME_OVER) {
      RectangleShape overlay({WINDOW_W, WINDOW_H});
      overlay.setFillColor(Color(0, 0, 0, 180));
      w.draw(overlay);

      Text res(result, font, 28);
      res.setStyle(Text::Bold);
      res.setFillColor(Color::White);
      FloatRect rb = res.getLocalBounds();
      res.setOrigin(rb.left + rb.width / 2.0f, rb.top + rb.height / 2.0f);
      res.setPosition(WINDOW_W / 2.0f, WINDOW_H / 2.0f - 80);
      w.draw(res);

      if (mode == NET_CLIENT && result != "Lost Connection to Server.") {
        Text waitTxt("Waiting for next round...", font, 18);
        waitTxt.setFillColor(Color(0, 200, 255));
        FloatRect wb = waitTxt.getLocalBounds();
        waitTxt.setOrigin(wb.left + wb.width / 2.0f, wb.top + wb.height / 2.0f);
        waitTxt.setPosition(WINDOW_W / 2.0f, WINDOW_H / 2.0f - 30);
        w.draw(waitTxt);
      }

      Vector2i mouse = Mouse::getPosition(w);
      btnMainMenu.update(mouse);
      btnMainMenu.draw(w);

      // In Tournaments, the server decides when you play again, so we hide the
      // local Play Again button.
      if (mode != NET_CLIENT) {
        btnPlayAgain.update(mouse);
        btnPlayAgain.draw(w);
      }
    }
  }
};

int main() {
  RenderWindow window(VideoMode(WINDOW_W, WINDOW_H),
                      "Tic Tac Toe Tournament Edition");
  window.setFramerateLimit(60);
  Game game;

  while (window.isOpen()) {
    Event e;
    while (window.pollEvent(e)) {
      if (e.type == Event::Closed)
        window.close();
      if (e.type == Event::TextEntered)
        game.handleText(e.text.unicode);
      if (e.type == Event::MouseButtonPressed &&
          e.mouseButton.button == Mouse::Left)
        game.handleClick(Mouse::getPosition(window));
    }
    game.update();
    window.clear();
    game.draw(window);
    window.display();
  }
  return 0;
}