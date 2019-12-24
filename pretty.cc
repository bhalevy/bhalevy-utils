#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <stack>
#include <unordered_map>
#include <cxxabi.h>

/*
 * Inspired by reply to https://stackoverflow.com/questions/2885597/c-template-name-pretty-print
 * by Andrey Asadchev (https://stackoverflow.com/users/206328/anycorn)
 */

static size_t max_elem_size = 32;
static std::unordered_map<char, char> closing_map = {
    {'(', ')'},
    {'{', '}'},
    {'<', '>'},
};

size_t find_closing(const std::string str, size_t token, char opening, char closing)
{
    int count = 0;

    for (;;) {
        auto close = str.find(closing, token + 1);
        auto open = str.find(opening, token + 1);
        assert(close != std::string::npos);

        while (open != std::string::npos && open < close) {
            count++;
            open = str.find(opening, open + 1);
        }

        if (!count) {
            return close;
        }

        count--;
        token = close;
    }
    return std::string::npos;
}

void do_indent(std::string str, size_t token, const std::string indentation) {
    str.insert(token, indentation);
    token += indentation.size();

}

std::string indent(std::string str, const std::string &indent_by = "  ") {
    std::string indentation = std::string("\n");
    size_t token = 0;
    std::stack<char> closing;
    std::stack<bool> one_line;

    while ((token = str.find_first_of("(){}<>,", token)) != std::string::npos) {
        size_t size = str.size();
        size_t close;
        bool skip_space = true;

        auto ch = str[token];
        auto close_ch = closing_map[ch];

        switch (ch) {
        case '(':
        case '{':
        case '<':
            closing.push(close_ch);
            close = find_closing(str, token, ch, close_ch);
            one_line.push(close - token <= max_elem_size);

            if (one_line.top()) {
                break;
            }
            indentation.append(indent_by);
            [[fallthrough]];
        case ',':
            str.insert(token + 1, one_line.top() ? " " : indentation);
            break;

        case ')':
        case '}':
        case '>':
            if (ch != closing.top()) {
                std::cerr << "Unmatched closing '" << ch << "'. Expected '" << closing.top() << "'" << std::endl;
                break;
            }
            closing.pop();
            if (!one_line.top()) {
                if (indentation.size() >= indent_by.size()) {
                    indentation.erase(indentation.size() - indent_by.size());
                } else {
                    str.insert(token, "<UF>\n");
                }
                str.insert(token, indentation);
                skip_space = false;
            }
            one_line.pop();
            break;

        default:
            assert(false); 
        }

        token += 1 + str.size() - size;

        if (skip_space) {
            const size_t nw = str.find_first_not_of(" ", token);
            if (nw != std::string::npos) {
                str.erase(token, nw - token);
            }
        }
    }

    return str;
}

void pretty(std::string str) {
    std::cout << indent(str) << std::endl;
}

void usage(const char*argv0, const char* msg = nullptr, const char* arg = nullptr) {
    if (msg) {
        std::cerr << argv0 << ": ";
        if (arg) {
            std::cerr << arg << ": ";
        }
        std::cerr << msg << std::endl;
    }
    std::cerr << "Usage: " << argv0 << " [-h] [-s wax_elem_size (default=" << max_elem_size << ")] [-f file]" << std::endl;
    exit(2);
}

char* getarg(int& argc, char**& argv) {
    auto arg = &(*argv)[2];
    if (!arg[0]) {
        argc--;
        arg = *(++argv);
    }
    return arg;
}

int main(int argc, char** argv) {
    auto argv0 = argv[0];
    auto* in = &std::cin;
    std::ifstream infile;

    while (argc > 1 && argv[1][0] == '-') {
        const char *arg;
        argc--;
        argv++;
        switch ((*argv)[1]) {
        case 'f':
            arg = getarg(argc, argv);
            if (!arg) {
                usage(argv0, "missing file argument");
            }
            std::cerr << "opening " << arg << std::endl;
            try {
                infile.open(arg);
            } catch (const std::exception& e) {
                usage(argv0, e.what(), arg);
            }
            if (!infile.is_open()) {
                usage(argv0, "file could not be opened", arg);
            }
            in = &infile;
            break;

        case 's':
            arg = getarg(argc, argv);
            if (!arg) {
                usage(argv0, "missing size argument");
            }
            if (arg[0] < '0' || arg[0] > '9') {
                usage(argv0, "invalid size argument", arg);
            }
            max_elem_size = atoi(arg);
            break;

        default:
            usage(argv0);
        }
    }

    if (argc > 1) {
        pretty(argv[1]);
    } else {
        std::string str;
        while (!in->eof()) {
            getline(*in, str);
            pretty(str);
        }
    }

    return 0;
}

