# Internet Project 2 - OX Chess Network Program

This is a **Client-Server** based OX chess game application that supports multiplayer connections and allows players to compete against each other in real-time.

## 📌 Features

1. **Multiplayer Connection**:
   - Supports at least 2 Clients connected to the Server simultaneously.
   - Clients can list all online users.

2. **Challenge Mechanism**:
   - Players can select an opponent and send an invitation for a match.
   - The game starts once the opponent accepts the invitation.

3. **Game Rules**:
   - Players take turns to make moves until a winner is decided or the game ends in a draw.
   - If a player quits, the game ends automatically.

4. **Logout Functionality**:
   - Players can choose to log out from the Server.

## 🌟 Additional Features

1. **Invitation Check**:
   - If the invited player is already engaged in a match with another player, the inviter will be notified that the player is currently in a game and will be prompted to select another opponent.

2. **Enhanced Online User List**:
   - The list of online users includes an indicator for players who are currently engaged in a game (e.g., "in-game"), allowing others to avoid inviting them.

3. **Game Exit Handling**:
   - Players engaged in a match can quit the game. When a player leaves, the opponent will receive a notification that their opponent has left, and the match will end.

4. **Command Menu**:
   - A `menu` command is available, providing a list of all available commands and their descriptions.

## 🚀 How to Run

### Server
1. Compile the server program: `gcc server.c -lpthread -o server`
2. Run the server: `./server`

### Client
1. Compile the client program: `gcc client.c -lpthread -o client`
2. Run the client: `./client`

## 📋 Commands Available in the Game

- **`menu`**: Displays the list of available commands.
- **`list`**: Shows a list of all online players. Players who are currently in a game are marked as "in-game".
- **`match [username]`**: Sends an invitation to the specified player for a match.
- **`Y` or `y`**: Accepts a match invitation.
- **`N` or `n`**: Rejects a match invitation.
- **`quit`**: Exits the game and disconnects from the server.

## 🎮 Gameplay Instructions

1. **Start the Game**:
   - Players can view the list of online users using the `list` command.
   - Send an invitation to a specific player using `match [username]`.

2. **Match Invitation**:
   - The invited player can accept (`Y`) or reject (`N`) the invitation.

3. **Game Rules**:
   - Once the game starts, players take turns placing their marker (O or X) on the board by entering a number between 1 and 9.
   - The game ends when a player wins, the board is full (draw), or a player quits.

4. **Exit the Game**:
   - Players can leave the match by entering `-1` during their turn, which will notify the opponent and end the game.

## 🛠️ Makefile

To simplify compilation, a Makefile is included:

- **`make all`**: Compiles both the server and client programs.
- **`make clean`**: Removes compiled binaries.

## ⚠️ Notes

- The program is designed for educational purposes and demonstrates the basics of socket programming, multithreading, and real-time communication in a networked environment.
- Ensure that the server is running before starting any client connections.

