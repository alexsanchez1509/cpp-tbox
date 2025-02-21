#include <iostream> 
#include <string> 

void showInstructions() { 
    std::cout << "Welcome to Escape the Dungeon!\n"; 
    std::cout << "Commands: look, go [direction], take [item], exit\n"; 
} 
 
void gameLoop() { 
    std::string command; 
    showInstructions(); 
 
    while (true) { 
        std::cout << "> "; 
        std::getline(std::cin, command); 
 
        if (command == "exit") { 
            std::cout << "Thank you for playing!\n"; 
            break; 
        } else if (command == "look") { 
            std::cout << "You are in a dark room. There is a door to the north.\n"; 
        } else { 
            std::cout << "Unknown command: " << command << "\n"; 
        } 
    } 
} 
 
int main() { 
    gameLoop(); 
    return 0; 
} 