#include <pico/stdlib.h>
#include "DisplayRender.hpp"

class Tetris {
public:
    void init() {
        for (int row = 0; row < LCD_ROWS; row++) {
            for (int col = 0; col < LCD_COLUMNS; col++) {
                grid[row][col] = ' ';
            }
        }
    }

    void draw_grid() {
        for (int row = 0; row < LCD_ROWS; row++) {
            std::string line;
            for (int col = 0; col < LCD_COLUMNS; col++) {
                line += grid[row][col];
            }
            render_set_row(row, line);
        }
    }

    void spawn_piece() {
        piece_x = LCD_COLUMNS / 2;
        piece_y = 0;
        grid[piece_y][piece_x] = 'O';
    }

    void move_piece(int dx, int dy) {
        if (piece_x + dx >= 0 && piece_x + dx < LCD_COLUMNS 
            && piece_y + dy >= 0 && piece_y + dy < LCD_ROWS) {
            grid[piece_y][piece_x] = ' ';
            piece_x += dx;
            piece_y += dy;
            grid[piece_y][piece_x] = 'O';
        }
    }

    void tick() {
        spawn_piece();
        draw_grid();
        move_piece(0, 1);
    }

private:
    char grid[LCD_ROWS][LCD_COLUMNS] = {};
    int piece_x = 10, piece_y = 0;
    bool game_running = false;
};
