#include <iostream>
#include <conio.h>
#include <fstream>
#include <windows.h>

using namespace std;

int main() {
	ifstream fin;
	char buffer[100];
	while (!_kbhit()) {
		fin.open("monitor.txt", ios_base::in);
		if (fin.is_open()) {
			system("cls");
			while (!fin.eof()) {
				ZeroMemory(buffer, 100);
				fin.getline(buffer, 100);
				cout << buffer << endl;
			}
			fin.close();
			Sleep(100);
		} else {
			cout << "Error: file can't open!\n";
			break;
		}
	}
	return 0;
}

