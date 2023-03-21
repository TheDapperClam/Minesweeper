#include <algorithm>
#include <chrono>
#include <conio.h>
#include <future>
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

/// <summary>
/// Information for playing a beep sound.
/// </summary>
struct BSound {
	DWORD Frequency;
	DWORD Duration;
};

/// <summary>
/// Information about our game's difficulty.
/// </summary>
struct Difficulty {
	int Width;
	int Height;
	int MineCount;
};

// Controls
const int MOVE_UP = VK_UP;
const int MOVE_DOWN = VK_DOWN;
const int MOVE_LEFT = VK_LEFT;
const int MOVE_RIGHT = VK_RIGHT;
const int CHECK_MINE = VK_SPACE;
const int PLACE_FLAG = 'F';
const int PLACE_QMK = 'Q';
const int NEW_GAME = 'R';

// Draw chars
const char CHAR_CLEAR = ' ';
const char CHAR_HIDDEN = '-';
const char CHAR_FLAG = '#';
const char CHAR_QMK = '?';
const char CHAR_MINE = 'M';
const char CHAR_CURSOR = 'X';

// Board cells
const int CELL_CLEAR = 0;
const int CELL_HIDDEN = 9;
const int CELL_FLAG = 10;
const int CELL_QMK = 11;

// Game difficulties
const Difficulty DIFFICULTIES[] = {
	{ 9, 9, 10 },
	{ 16, 16, 40 },
	{ 30, 16, 99 }
};

// Draw settings
const int CLEAR_AMOUNT = 100;
const int CELL_SIZE = 2;

// Sound effects
const BSound BSOUND_PLACE = { 100, 100 };
const BSound BSOUND_PICKUP = { 200, 100 };
const BSound BSOUND_MOVE = { 20, 100 };
const BSound BSOUND_CHECKMINE = { 300, 100 };
const BSound BSOUND_WIN = { 200, 1000 };
const BSound BSOUND_LOSE = { 100, 1000 };

// Forward declarations
int RandomRange ( const int &MIN, const int &MAX );
int GetAdjacentMineCount ( const int &X, const int &Y, const std::vector<std::vector<bool>> *MINES );
int GetMinefieldValue ( const int &X, const int &Y, const std::vector<std::vector<int>> *MINEFIELD );
void SetMinefieldValue ( const int &X, const int &Y, const int &VALUE, std::vector<std::vector<int>> *minefield );

/// <summary>
/// Check if there is a mine at the specified position.
/// </summary>
bool CheckMine ( const int &X, const int &Y, const std::vector<std::vector<bool>> *MINES ) {
	const int WIDTH = MINES->front ().size ();
	const int HEIGHT = MINES->size ();
	if ( X < 0 || X >= WIDTH || Y < 0 || Y >= HEIGHT )
		return false;
	return MINES->at ( Y ).at ( X );
}

/// <summary>
/// Write a minefield string to draw to a given output.
/// </summary>
void DrawMinefield ( std::string& output, const std::vector<std::vector<int>> *MINEFIELD, const std::vector<std::vector<bool>> *MINES, const bool &SHOW_MINES ) {
	const int WIDTH = MINEFIELD->front ().size ();
	const int HEIGHT = MINEFIELD->size ();
	const std::string SPACING = std::string ( CELL_SIZE - 1, ' ' );

	for ( int y = 0; y < HEIGHT; y++ ) {
		for ( int x = 0; x < WIDTH; x++ ) {
			if ( SHOW_MINES && CheckMine ( x, y, MINES ) ) {
				output += CHAR_MINE + SPACING;
				continue;
			}

			const int CELL_VALUE = GetMinefieldValue ( x, y, MINEFIELD );
			switch ( CELL_VALUE ) {
				default:
					output += std::to_string ( CELL_VALUE );
					break;
				case CELL_CLEAR:
					output += CHAR_CLEAR;
					break;
				case CELL_FLAG:
					output += CHAR_FLAG;
					break;
				case CELL_HIDDEN:
					output += CHAR_HIDDEN;
					break;
				case CELL_QMK:
					output += CHAR_QMK;
					break;
			}
			output += SPACING;
		}
		// Start a new line until we reach the final line.
		if ( y < HEIGHT - 1 )
			output += '\n';
	}
}

/// <summary>
/// Clear all cells in an area of a minefield that contain no mines, or have no adjacent mines.
/// </summary>
void FloodMinefield ( const int &X, const int &Y, std::vector<std::vector<int>> *minefield, const std::vector<std::vector<bool>> *MINES ) {
	const int WIDTH = minefield->front ().size ();
	const int HEIGHT = minefield->size ();

	if ( X < 0 || X >= WIDTH || Y < 0 || Y >= HEIGHT )
		return;
	if ( GetMinefieldValue ( X, Y, minefield ) == CELL_CLEAR )
		return;
	if ( CheckMine ( X, Y, MINES ) )
		return;

	int adjacentMines = GetAdjacentMineCount ( X, Y, MINES );
	SetMinefieldValue ( X, Y, adjacentMines, minefield );
	if ( adjacentMines > 0 )
		return;

	// Top left
	FloodMinefield ( X - 1, Y - 1, minefield, MINES );
	// Top
	FloodMinefield ( X, Y - 1, minefield, MINES );
	// Top right
	FloodMinefield ( X + 1, Y - 1, minefield, MINES );
	// Left
	FloodMinefield ( X - 1, Y, minefield, MINES );
	// Right
	FloodMinefield ( X + 1, Y, minefield, MINES );
	// Bottom left
	FloodMinefield ( X - 1, Y + 1, minefield, MINES );
	// Bottom
	FloodMinefield ( X, Y + 1, minefield, MINES );
	// Bottom right
	FloodMinefield ( X + 1, Y + 1, minefield, MINES );
}

/// <summary>
/// Get how many mines are around a given position.
/// </summary>
int GetAdjacentMineCount ( const int &X, const int &Y, const std::vector<std::vector<bool>> *MINES ) {
	int count = 0;
	// Top left
	if ( CheckMine ( X - 1, Y - 1, MINES ) )
		count++;
	// Top
	if ( CheckMine ( X, Y - 1, MINES ) )
		count++;
	// Top right
	if ( CheckMine ( X + 1, Y - 1, MINES ) )
		count++;
	// Left
	if ( CheckMine ( X - 1, Y, MINES ) )
		count++;
	// Right
	if ( CheckMine ( X + 1, Y, MINES ) )
		count++;
	// Bottom left
	if ( CheckMine ( X - 1, Y + 1, MINES ) )
		count++;
	// Bottom
	if ( CheckMine ( X, Y + 1, MINES ) )
		count++;
	// Bottom right
	if ( CheckMine ( X + 1, Y + 1, MINES ) )
		count++;
	return count;
}

/// <summary>
/// Get the value of a cell in a minefield.
/// </summary>
int GetMinefieldValue ( const int &X, const int &Y, const std::vector<std::vector<int>> *MINEFIELD ) {
	return MINEFIELD->at ( Y ).at ( X );
}

/// <summary>
/// Has a given key been pressed.
/// </summary>
bool IsKeyPressed ( const SHORT &KEY, std::vector<SHORT> *buffer ) {
	const bool PRESSED = std::count ( buffer->begin (), buffer->end (), KEY );
	if ( GetKeyState ( KEY ) & 0x8000 ) {
		if ( !PRESSED ) {
			buffer->push_back ( KEY );
			return true;
		}
	} else if ( PRESSED )
		buffer->erase ( std::remove ( buffer->begin (), buffer->end (), KEY ), buffer->end () );
	return false;
}

/// <summary>
/// Randomly populate a minefield with mines.
/// </summary>
void PlaceMines ( const int &AMOUNT, std::vector<std::vector<bool>> *mines, const int &AVOID_X, const int &AVOID_Y ) {
	const int WIDTH = mines->front ().size ();
	const int HEIGHT = mines->size ();
	for ( int i = 0; i < AMOUNT; i++ ) {
		int x = 0;
		int y = 0;

		do {
			x = RandomRange ( 0, WIDTH );
			y = RandomRange ( 0, HEIGHT );
		} while ( x == AVOID_X && y == AVOID_Y || CheckMine ( x, y, mines ) );

		mines->at ( y ).at ( x ) = true;
	}
}

/// <summary>
/// Play a beep sound.
/// </summary>
void PlayBSound ( const BSound &SOUND ) {
	std::thread t ( [] ( const BSound &S ) {
		Beep ( S.Frequency, S.Duration );
	}, SOUND );
	t.detach ();
}

/// <summary>
/// Get a random number within a specified range.
/// </summary>
int RandomRange ( const int &MIN, const int &MAX ) {
	return rand () % MAX + MIN;
}

/// <summary>
/// Set the value of a cell in a minefield.
/// </summary>
void SetMinefieldValue ( const int &X, const int &Y, const int &VALUE, std::vector<std::vector<int>> *minefield ) {
	minefield->at ( Y ).at ( X ) = VALUE;
}

/// <summary>
/// Check if the game's win conditions have been met.
/// </summary>
bool VerifyVictory ( const std::vector<std::vector<int>> *minefield, const std::vector<std::vector<bool>> *mines ) {
	int WIDTH = minefield->front ().size ();
	int HEIGHT = minefield->size ();
	for ( int y = 0; y < HEIGHT; y++ )
		for ( int x = 0; x < WIDTH; x++ )
			if ( GetMinefieldValue ( x, y, minefield ) == CELL_HIDDEN && !CheckMine ( x, y, mines ) )
				return false;
	return true;
}

/// <summary>
/// Entry point of our game.
/// </summary>
int main () {
	auto inputBuffer = std::vector<SHORT> ();
	int difficultyInput = 0;
	bool endGame = false;
	bool newGame = false;
	srand ( std::chrono::duration_cast<std::chrono::milliseconds> ( std::chrono::system_clock::now ().time_since_epoch () ).count () );

	do {
		difficultyInput = -1;
		std::cout << "Select a difficulty: 1 - Beginner, 2 - Intermediate, 3 - Expert" << std::endl;
		while ( difficultyInput < 0 ) {
			if ( IsKeyPressed ( '1', &inputBuffer ) )
				difficultyInput = 0;
			else if ( IsKeyPressed ( '2', &inputBuffer ) )
				difficultyInput = 1;
			else if ( IsKeyPressed ( '3', &inputBuffer ) )
				difficultyInput = 2;
		}
			
		const Difficulty GAME_DIFFICULTY = DIFFICULTIES[ difficultyInput ];
		const int WIDTH = GAME_DIFFICULTY.Width;
		const int HEIGHT = GAME_DIFFICULTY.Height;
		const int BASE_MINE_COUNT = GAME_DIFFICULTY.MineCount;
		int cursorX = 0;
		int cursorY = 0;
		auto minefield = std::vector<std::vector<int>> ( HEIGHT, std::vector<int> ( WIDTH, CELL_HIDDEN ) );
		auto mines = std::vector<std::vector<bool>> ( HEIGHT, std::vector<bool> ( WIDTH, false ) );
		bool shouldPlaceMines = true;
		bool run = true; 
		bool victory = false;
		bool shouldDrawMinefield = true;
		endGame = false;
		newGame = false;

		while ( run && !victory && !newGame ) {
			// Pre-fill our out string with new lines for "clearing" the screen.
			std::string out = std::string ( CLEAR_AMOUNT, '\n' );

			bool wantsToMove = false;
			// Move left
			if ( IsKeyPressed ( MOVE_UP, &inputBuffer ) ) {
				cursorY--;
				wantsToMove = true;
			}
			// Move down
			if ( IsKeyPressed ( MOVE_DOWN, &inputBuffer ) ) {
				cursorY++;
				wantsToMove = true;
			}
			// Move left
			if ( IsKeyPressed ( MOVE_LEFT, &inputBuffer ) ) {
				cursorX--;
				wantsToMove = true;
			}
			// Move right
			if ( IsKeyPressed ( MOVE_RIGHT, &inputBuffer ) ) {
				cursorX++;
				wantsToMove = true;
			}
			if ( wantsToMove ) {
				PlayBSound ( BSOUND_MOVE );
				shouldDrawMinefield = true;
			}

			if ( cursorX < 0 )
				cursorX = WIDTH - 1;
			else if ( cursorX >= WIDTH )
				cursorX = 0;
			if ( cursorY < 0 )
				cursorY = HEIGHT - 1;
			else if ( cursorY >= HEIGHT )
				cursorY = 0;

			const int CURRENT_CELL_VALUE = GetMinefieldValue ( cursorX, cursorY, &minefield );
			if ( IsKeyPressed ( PLACE_FLAG, &inputBuffer ) ) {
				shouldDrawMinefield = true;
				switch ( CURRENT_CELL_VALUE ) {
					case CELL_HIDDEN:
						SetMinefieldValue ( cursorX, cursorY, CELL_FLAG, &minefield );
						PlayBSound ( BSOUND_PLACE );
						break;
					case CELL_QMK:
						SetMinefieldValue ( cursorX, cursorY, CELL_FLAG, &minefield );
						PlayBSound ( BSOUND_PLACE );
						break;
					case CELL_FLAG:
						SetMinefieldValue ( cursorX, cursorY, CELL_HIDDEN, &minefield );
						PlayBSound ( BSOUND_PICKUP );
						break;
				}
			} else if ( IsKeyPressed ( PLACE_QMK, &inputBuffer ) ) {
				shouldDrawMinefield = true;
				switch ( CURRENT_CELL_VALUE ) {
					case CELL_HIDDEN:
						SetMinefieldValue ( cursorX, cursorY, CELL_QMK, &minefield );
						PlayBSound ( BSOUND_PLACE );
						break;
					case CELL_FLAG:
						SetMinefieldValue ( cursorX, cursorY, CELL_QMK, &minefield );
						PlayBSound ( BSOUND_PLACE );
						break;
					case CELL_QMK:
						SetMinefieldValue ( cursorX, cursorY, CELL_HIDDEN, &minefield );
						PlayBSound ( BSOUND_PICKUP );
						break;
				}
			} else if ( IsKeyPressed ( CHECK_MINE, &inputBuffer ) && CURRENT_CELL_VALUE == CELL_HIDDEN ) {
				shouldDrawMinefield = true;
				// Populate our field with mines after the first check, this way the player will never lose on their first check.
				if ( shouldPlaceMines ) {
					PlaceMines ( BASE_MINE_COUNT, &mines, cursorX, cursorY );
					shouldPlaceMines = false;
				}

				if ( !CheckMine ( cursorX, cursorY, &mines ) ) {
					FloodMinefield ( cursorX, cursorY, &minefield, &mines );
					victory = VerifyVictory ( &minefield, &mines );
				} else
					run = false;

				if ( victory )
					PlayBSound ( BSOUND_WIN );
				else if ( !run )
					PlayBSound ( BSOUND_LOSE );
				else
					PlayBSound ( BSOUND_CHECKMINE );
			} else if ( IsKeyPressed ( NEW_GAME, &inputBuffer ) )
				newGame = true;

			if ( shouldDrawMinefield ) {
				out += "UP, DOWN, LEFT, RIGHT - Move | SPACE - Check | F - Flag | Q - Question mark | R - Restart \n";
				out += "Position X: " + std::to_string ( cursorX ) + ", Y: " + std::to_string ( cursorY ) + '\n';
				// Anything that gets draw above the gameboard should go above this point so we don't mess up the cursor.
				const int CURSOR_OFFSET = out.size ();
				DrawMinefield ( out, &minefield, &mines, !run );
				// Here we convert our 2D cursor position to 1D.
				out[cursorY * ( WIDTH * CELL_SIZE + 1 ) + cursorX * CELL_SIZE + CURSOR_OFFSET] = CHAR_CURSOR;
				std::cout << out << std::endl;
				shouldDrawMinefield = false;
			}
		}

		if ( !newGame ) {
			if ( victory )
				std::cout << "YOU WIN!" << std::endl;
			else
				std::cout << "YOU LOSE!" << std::endl;
			std::cout << "New Game: Y/N" << std::endl;
			while ( !newGame && !endGame ) {
				if ( IsKeyPressed ( 'Y', &inputBuffer ) )
					newGame = true;
				else if ( IsKeyPressed ( 'N', &inputBuffer ) )
					endGame = true;
			}
		}
	} while ( !endGame || newGame );

	return 0;
}