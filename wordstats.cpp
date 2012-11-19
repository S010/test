#include <iostream>
#include <istream>
#include <fstream>
#include <string>
#include <unordered_map>

using namespace std;

void
word_stats(istream &in)
{
    unordered_map<string, unsigned int> stats;
    string word;

    while (in >> word) {
        auto i(stats.find(word));
        if (i == stats.end())
            stats[word] = 1;
        else
            ++i->second;
    }

    for (auto i(stats.begin()); i != stats.end(); ++i)
        cout << i->first << " " << i->second << endl;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        word_stats(cin);
    } else {
        ifstream in;

        for (char **p = argv + 1; --argc > 0; ++p) {
            in.open(*p);

            if (!in) {
                cerr << "Failed to open file \"" << *p << "\"" << endl;
                continue;
            }

            word_stats(in);
            in.close();
        }
    }

    return 0;
}

