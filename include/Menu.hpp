#pragma once

#include <pico/stdlib.h>
#include "DisplayRender.hpp"
#include "LCD_I2C.hpp"
#include <functional>
#include <iterator>
#include <string>
#include <map>
#include <vector>

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
        printf("%s: %lu\n", "Options size", options.size());
        printf("%s: %i\n", "Executing option", current_index + current_inside_view);
        std::advance(iter, current_index + current_inside_view);
        printf("%s: %s\n", "Executing option", iter->first.c_str());
        int f = *(int*)iter->second.second;
        printf("%s: %i\n", "Param to function: ", f);
        iter->second.first(iter->second.second);
        printf("%s: %i\n", "Executed option", current_index + current_inside_view);
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

    void add_option(std::string name, std::function<void(void*)> callback, void* params) {
        // /options[name] = callback;
        // options.emplace(std::pair(std::string(name), callback));
        options.push_back(std::pair(std::string(name), std::pair(callback, params)));
    }

    void remove_option(std::string name) {
        for (int i = 0; i < options.size(); i++) {
            if (options[i].first == name) {
                options.erase(options.begin() + i);
                break;
            }
        }
    }
    
private:
    static Menu *current_menu;
    // std::map<std::string, std::function<void()>> options;
    std::vector<std::pair<std::string, std::pair<std::function<void(void*)>, void*>>> options;

    // Control the rendering
    int current_index = 0;
    int current_inside_view = 0;
};

Menu *Menu::current_menu = nullptr;