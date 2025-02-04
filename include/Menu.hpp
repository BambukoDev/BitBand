#pragma once

#include <pico/stdlib.h>
#include "DisplayRender.hpp"
#include "LCD_I2C.hpp"
#include <functional>
#include <iterator>
#include <string>
#include <map>
#include <vector>

typedef void (*menu_callback_t)();

class Menu {
public:
    // TODO: Make it work for less than 4 options
    void render() {
        std::vector<std::string> to_render;
        auto iter = options.begin();
        std::advance(iter, current_index);
        for (uint i = current_index; i <= current_index + 3; i++) {
            to_render.push_back(iter->first);
            std::advance(iter, 1);
        }

        for (uint i = 0; i < LCD_ROWS; i++) {
            if (i == current_inside_view) {
                render_set_row(i, ">" + to_render[i]);
            } else {
                render_set_row(i, to_render[i]);
            }
        }
    }

    void execute_current() {
        auto iter = options.begin();
        std::advance(iter, current_index + current_inside_view);
        iter->second();
    }

    // TODO: Make it work for scroll wheel
    // TODO: Make it work for less than 4 options
    void move_selection(int count) {
        if (count == 0) return;
        
        if (count > 0) {
            current_inside_view++;
            if (current_inside_view >= 4) {
                current_index++;
                if (current_index >= options.size() - 3) {
                    current_inside_view = 0;
                    current_index = 0;
                }
            }
            if (current_inside_view > 3) current_inside_view = 3;
        }
        
        if (count < 0) {
            current_inside_view--;
            if (current_inside_view < 0) {
                current_index--;
                if (current_index < 0) {
                    current_index = options.size() - 4;
                    current_inside_view = 3;
                }
            }
            if (current_inside_view < 0) current_inside_view = 0;
        }

        // printf("-------------\n");
        // printf("%i\n", current_index);
        // printf("%i\n", current_inside_view);
        // printf("-------------\n");
    }

    void set_as_current() {
        Menu::current_menu = this;
    }

    static Menu* get_current() {
        return current_menu;
    }

    static void remove_current() {
        Menu::current_menu = nullptr;
    }

    void add_option(std::string name, menu_callback_t callback) {
        options[name] = callback;
    }

    void remove_option(std::string name) {
        options.erase(name);
    }
    
private:
    static Menu *current_menu;
    std::map<std::string, menu_callback_t> options;

    // Control the rendering
    int current_index = 0;
    int current_inside_view = 0;
};

Menu *Menu::current_menu = nullptr;