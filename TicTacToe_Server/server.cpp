#include <SFML/Network.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace sf;

// ================= PACKET PROTOCOL =================
// Both Server and Client must agree on these message types
enum PacketType {
  JOIN_LOBBY = 0,   // Client -> Server: "My name is X"
  LOBBY_UPDATE = 1, // Server -> Client: "Waiting for tournament to start... N
                    // players connected"
  MATCH_START =
      2, // Server -> Client: "Match starting! Your opponent is Y. You are X/O."
  MAKE_MOVE = 3,    // Client -> Server: "I clicked row R, col C"
  BOARD_UPDATE = 4, // Server -> Client: "Piece placed at R, C"
  MATCH_OVER = 5,   // Server -> Client: "You Win/Lose/Draw"
  TOURNEY_OVER =
      6 // Server -> Client: "Tournament finished! Here are the standings."
};

// ================= HEADLESS BOARD =================
// The server must keep track of boards to prevent cheating and check wins
class HeadlessBoard {
public:
  char grid[3][3];
  int moves = 0;

  HeadlessBoard() { reset(); }

  void reset() {
    moves = 0;
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        grid[i][j] = ' ';
  }

  bool move(int r, int c, char p) {
    if (grid[r][c] == ' ') {
      grid[r][c] = p;
      moves++;
      return true;
    }
    return false;
  }

  char checkWin() {
    for (int i = 0; i < 3; i++) {
      if (grid[i][0] != ' ' && grid[i][0] == grid[i][1] &&
          grid[i][1] == grid[i][2])
        return grid[i][0];
      if (grid[0][i] != ' ' && grid[0][i] == grid[1][i] &&
          grid[1][i] == grid[2][i])
        return grid[0][i];
    }
    if (grid[0][0] != ' ' && grid[0][0] == grid[1][1] &&
        grid[1][1] == grid[2][2])
      return grid[0][0];
    if (grid[0][2] != ' ' && grid[0][2] == grid[1][1] &&
        grid[1][1] == grid[2][0])
      return grid[0][2];
    return ' ';
  }
};

// ================= TOURNAMENT STRUCTURES =================
struct Player {
  TcpSocket *socket;
  string name;
  int score = 0; // Win = 3, Draw = 1, Loss = 0
  bool isPlaying = false;
};

struct Match {
  Player *p1; // Plays 'X'
  Player *p2; // Plays 'O'
  HeadlessBoard board;
  bool p1Turn = true;
  bool isFinished = false;
};

// ================= SERVER MAIN =================
int main() {
  const unsigned short PORT = 53000;
  const int REQUIRED_PLAYERS = 4; // Auto-start tournament when 4 players join

  TcpListener listener;
  SocketSelector selector;
  vector<Player *> players;
  vector<Match *> activeMatches;

  enum ServerState { LOBBY, PLAYING_ROUND, TOURNAMENT_OVER };
  ServerState state = LOBBY;

  if (listener.listen(PORT) != Socket::Done) {
    cout << "Error: Cannot bind to port " << PORT << endl;
    return -1;
  }
  selector.add(listener);
  cout << "Tournament Server running on port " << PORT << "..." << endl;
  cout << "Waiting for " << REQUIRED_PLAYERS << " players to join." << endl;

  while (true) {
    // Wait for incoming data on any socket (blocks until data arrives)
    if (selector.wait()) {

      // 1. Check for New Connections
      if (selector.isReady(listener)) {
        TcpSocket *client = new TcpSocket;
        if (listener.accept(*client) == Socket::Done) {
          if (state != LOBBY) {
            cout << "Rejected a connection: Tournament already in progress.\n";
            client->disconnect();
            delete client;
          } else {
            selector.add(*client);
            Player *newPlayer = new Player;
            newPlayer->socket = client;
            players.push_back(newPlayer);
            cout << "New client connected! IP: " << client->getRemoteAddress()
                 << endl;
          }
        } else {
          delete client;
        }
      }

      // 2. Check for Data from Existing Players
      else {
        for (auto it = players.begin(); it != players.end();) {
          Player *p = *it;
          if (selector.isReady(*(p->socket))) {
            Packet packet;
            Socket::Status status = p->socket->receive(packet);

            if (status == Socket::Disconnected) {
              cout << "Player disconnected: "
                   << (p->name.empty() ? "Unknown" : p->name) << endl;
              selector.remove(*(p->socket));
              p->socket->disconnect();
              delete p->socket;
              delete p;
              it = players.erase(it);
              continue;
            } else if (status == Socket::Done) {
              int type;
              packet >> type;

              // Handle: Player set their name
              if (type == JOIN_LOBBY && state == LOBBY) {
                packet >> p->name;
                cout << p->name << " joined the lobby. (" << players.size()
                     << "/" << REQUIRED_PLAYERS << ")" << endl;

                // Broadcast lobby update
                Packet update;
                update << LOBBY_UPDATE << (int)players.size()
                       << REQUIRED_PLAYERS;
                for (Player *broadcastPlayer : players) {
                  broadcastPlayer->socket->send(update);
                }

                // Start Tournament if lobby is full!
                if (players.size() == REQUIRED_PLAYERS) {
                  cout << "\n=== STARTING TOURNAMENT ===\n";
                  state = PLAYING_ROUND;

                  // VERY BASIC MATCHMAKING (Round 1 only for this example)
                  // Player 0 vs 1, Player 2 vs 3
                  for (size_t i = 0; i < players.size(); i += 2) {
                    Match *m = new Match;
                    m->p1 = players[i];
                    m->p2 = players[i + 1];
                    m->p1->isPlaying = true;
                    m->p2->isPlaying = true;
                    activeMatches.push_back(m);

                    Packet start1, start2;
                    start1 << MATCH_START << m->p2->name
                           << true; // P1 goes first
                    start2 << MATCH_START << m->p1->name
                           << false; // P2 goes second
                    m->p1->socket->send(start1);
                    m->p2->socket->send(start2);

                    cout << "Match started: " << m->p1->name << " vs "
                         << m->p2->name << endl;
                  }
                }
              }
              // Handle: Player made a move
              else if (type == MAKE_MOVE && state == PLAYING_ROUND) {
                int r, c;
                packet >> r >> c;

                // Find which match this player belongs to
                for (Match *m : activeMatches) {
                  if (m->isFinished)
                    continue;

                  if ((p == m->p1 && m->p1Turn) || (p == m->p2 && !m->p1Turn)) {
                    char piece = (p == m->p1) ? 'X' : 'O';

                    if (m->board.move(r, c, piece)) {
                      // Broadcast move to both players
                      Packet moveAck;
                      moveAck << BOARD_UPDATE << r << c << piece;
                      m->p1->socket->send(moveAck);
                      m->p2->socket->send(moveAck);

                      m->p1Turn = !m->p1Turn; // Switch turn

                      // Check win condition
                      char winner = m->board.checkWin();
                      if (winner != ' ') {
                        m->isFinished = true;
                        Packet winPkt, losePkt;
                        winPkt << MATCH_OVER << string("You Win!");
                        losePkt << MATCH_OVER << string("You Lose!");

                        if (winner == 'X') {
                          m->p1->score += 3;
                          m->p1->socket->send(winPkt);
                          m->p2->socket->send(losePkt);
                        } else {
                          m->p2->score += 3;
                          m->p2->socket->send(winPkt);
                          m->p1->socket->send(losePkt);
                        }
                        cout << "Match ended. Winner: " << winner << endl;
                      } else if (m->board.moves == 9) {
                        m->isFinished = true;
                        m->p1->score += 1;
                        m->p2->score += 1;
                        Packet drawPkt;
                        drawPkt << MATCH_OVER << string("Draw!");
                        m->p1->socket->send(drawPkt);
                        m->p2->socket->send(drawPkt);
                        cout << "Match ended in a draw." << endl;
                      }
                    }
                  }
                }
              }
            }
          }
          ++it;
        }
      }
    }
  }
  return 0;
}