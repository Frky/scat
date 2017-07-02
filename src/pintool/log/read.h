
#include <iostream>
#include <string>

string read_part(std::ifstream &ifile) {
    char m;
    string str = "";

    ifile.read(&m, 1);
    while (ifile && m != ':' && m != ',' && m != '\n') {
        str += m;
        ifile.read(&m, 1);
    }
    return str;
}

string read_line(std::ifstream &ifile) {
    char m;
    string str = "";
    ifile.read(&m, 1);
    while (ifile && m != '\n') {
        str += m;
        ifile.read(&m, 1);
    }
    return str;
}

void skip_line(std::ifstream &ifile) {
    char m = '!';
    while (ifile && m != '\n') {
        ifile.read(&m, 1);
    }
    return;
}
