#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

int main(int argc, char** argv) {
    if (argc < 3)
        return 0;

    int i, j;
    int width, height;

    width = atoi(argv[1]);
    height = atoi(argv[2]);

    srand(time(NULL));
    for (i = 0; i < height; ++i) {
        for (j = 0; j < width; ++j) {
            if (j > 0)
                std::cout << ' ';
            std::cout << rand();
        }
        std::cout << std::endl;
    }

    return 0;
}
