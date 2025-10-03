/*
* HL2Q3 - Half-Life 1 .map to Quake 1|2|3 .map converter
* Copyright (C) 2025  motorsep
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

int main(int argc, char* argv[]) {
    if (argc != 5 || std::string(argv[3]) != "--game") {
        std::cerr << "Usage: " << argv[0] << " input.map output.map --game [quake1|quake2|quake3]" << std::endl;
        return 1;
    }

    std::string game = argv[4];
    if (game != "quake1" && game != "quake2" && game != "quake3") {
        std::cerr << "Invalid game: " << game << ". Use quake1, quake2, or quake3." << std::endl;
        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "Error opening input file: " << argv[1] << std::endl;
        return 1;
    }

    std::ofstream out(argv[2]);
    if (!out) {
        std::cerr << "Error opening output file: " << argv[2] << std::endl;
        return 1;
    }

    std::string line;
    std::vector<std::string> entity_lines;
    bool parsing_entity = false;
    int entity_depth = 0;
    std::string classname;
    bool found_worldspawn = false;

    while (std::getline(in, line)) {
        // Strip comments
        size_t comment_pos = line.find("//");
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        // Trim whitespace
        line = trim(line);
        if (line.empty()) continue;

        if (line == "{") {
            if (entity_depth == 0) {
                parsing_entity = true;
                entity_lines.clear();
                classname = "";
            }
            entity_depth++;
            if (parsing_entity) entity_lines.push_back("{");
        }
        else if (line == "}") {
            if (parsing_entity) entity_lines.push_back("}");
            entity_depth--;
            if (entity_depth == 0 && parsing_entity) {
                parsing_entity = false;
                if (classname == "worldspawn") {
                    // Check if mapversion is present, add if missing (for all games, to preserve V220)
                    bool has_mapversion = false;
                    for (const auto& eline : entity_lines) {
                        if (eline.find("\"mapversion\"") != std::string::npos) {
                            has_mapversion = true;
                            break;
                        }
                    }
                    if (!has_mapversion) {
                        // Insert after classname
                        auto it = entity_lines.begin() + 1; // After first {
                        while (it != entity_lines.end() && (it->find("\"classname\"") == std::string::npos)) {
                            ++it;
                        }
                        if (it != entity_lines.end()) {
                            ++it; // After classname line
                            entity_lines.insert(it, "\"mapversion\" \"220\"");
                        }
                    }

                    // Output the entity lines
                    for (const auto& eline : entity_lines) {
                        out << eline << std::endl;
                    }
                    found_worldspawn = true;
                }
                // Else ignore other entities
            }
        }
        else {
            if (parsing_entity) {
                entity_lines.push_back(line);
                if (classname.empty() && line.find("\"classname\"") != std::string::npos) {
                    // Parse classname
                    std::istringstream iss(line);
                    std::string key, value;
                    std::string temp;
                    while (iss >> temp) {
                        if (key.empty() && temp == "\"classname\"") {
                            key = temp;
                        }
                        else if (!key.empty() && value.empty()) {
                            value = temp;
                            break;
                        }
                    }
                    if (key == "\"classname\"") {
                        // Strip quotes from value
                        if (!value.empty() && value.front() == '"' && value.back() == '"') {
                            classname = value.substr(1, value.size() - 2);
                        }
                    }
                }
            }
        }
    }

    if (!found_worldspawn) {
        std::cerr << "Warning: No worldspawn entity found in input map." << std::endl;
    }

    std::cout << "Conversion to " << game << " format completed." << std::endl;

    return 0;
}