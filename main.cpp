
#include <iostream>
#include<vector>
#include<Windows.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <array>
#include <algorithm>
#include <map>
#include "sha.h"
#include "filters.h"
#include "base64.h"
#include <nlohmann/json.hpp>
#include "sqlite3.h"
using json = nlohmann::json;
using namespace std;

void hide_input() {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hStdin, &mode);
	SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
	return;
}
void show_input() {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hStdin, &mode);
	SetConsoleMode(hStdin, mode);
	return;
}
string get_config(string conf) {
	string details;
	ifstream i("config.json");
	json j = json::parse(i); 
	map<string, string> m = j.at("Config").get<map<string, string>>();
	details = m[conf];
	if (details == "") {
		if (conf == "Allow_Adding_Users") { //defaulting to False if missing in Config
			details = "False"; 
		}
		else {
			std::cout << "Error in Config-File! Exiting..." << endl;
			exit(1);
		}
	}
	else {
		return details;
	}
}
string string_to_hex(const string& input)
{
	static const char* const lut = "0123456789ABCDEF";
	size_t len = input.length();

	string output;
	output.reserve(2 * len);
	for (size_t i = 0; i < len; ++i)
	{
		const unsigned char c = input[i];
		output.push_back(lut[c >> 4]);
		output.push_back(lut[c & 15]);
	}
	return output;
}
string SHA256(string data)
{
	CryptoPP::byte const* pbData = (CryptoPP::byte*)data.data();
	unsigned int nDataLen = data.length();
	CryptoPP::byte abDigest[CryptoPP::SHA256::DIGESTSIZE];

	CryptoPP::SHA256().CalculateDigest(abDigest, pbData, nDataLen);
	return string_to_hex(string((char*)abDigest, CryptoPP::SHA256::DIGESTSIZE));
}
string get_data_db(string username) {
	string statement = "SELECT password_hash FROM users WHERE username LIKE '" + username + "';";
	sqlite3* db;
	int cols = 0;
	string hash = "Empty";
		int rc = sqlite3_open(get_config("DB-Path").c_str(), &db);
		if (rc != 1) {
			sqlite3_stmt* t_statement;
			rc = sqlite3_prepare_v2(db, statement.c_str(), statement.length(), &t_statement, 0);

			if (rc != 1)
			{
				sqlite3_step(t_statement);

				if (sqlite3_column_text(t_statement, 0)) {
					hash = reinterpret_cast<const char*>(sqlite3_column_text(t_statement, 0));
				}
				else {
					hash = "Empty";
				}
				

			}
			sqlite3_finalize(t_statement);
		}
	sqlite3_close(db);

	return hash;

}
bool is_valid_credentials(string usr, string pw) {
	string hash = get_data_db(usr);
	string passwordHash = SHA256(pw);
	if (hash == passwordHash) {
		return true;
	}
	else {
		return false;
	}
}
void add_data_db(string username, string pw_hash) {
	sqlite3* db;
	char* zErrMsg = 0;
	int rc;
	string sql;
	rc = sqlite3_open(get_config("DB-Path").c_str(), &db);
	if (rc) {
		std::cout << "DB Error: " << sqlite3_errmsg(db) << endl;
		sqlite3_close(db);
		exit(1);
	}
	sql = "SELECT 1 from sqlite_master WHERE type = 'table' and name = 'users';";
	rc = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);
	if (rc == 0) {
		sql = "CREATE TABLE users (username VARCHAR, password_hash VARCHAR);"; //create Table if DB is fresh
		sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);
	}
	sql = "INSERT INTO users ('username', 'password_hash') VALUES ('" + username + "','" + pw_hash + "');";
	sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);
	sqlite3_close(db);
}
void user_add() {
	string new_user, new_pw, new_pw2, new_pwhash;
	while (true) {
		system("cls");
		std::cout << "User Registration" << endl;
		std::cout << "Enter Username: ";
		std::getline(cin, new_user);
		if (get_data_db(new_user) != "Empty") {
			std::cout << endl << "Username already in Use, please choose new one!";
			Sleep(1200);
			continue;
		}
		else {
			while (true) {
				hide_input();
				std::cout << endl << "Enter Password: ";
				std::getline(cin, new_pw);
				std::cout << endl << "Repeat Password: ";
				std::getline(cin, new_pw2);
				show_input();
				if (new_pw == new_pw2) {
					new_pwhash = SHA256(new_pw);
					break;
				}
				else {
					std::cout << endl << "Passwords do not match, please retry!";
					continue;
				}

			}
			add_data_db(new_user, new_pwhash);
			std::cout << endl << "User Added successfully!";
			break;
		}
		
	
	}
	return;
}

int main() {
	char input1;
	while (get_config("Allow_Adding_Users") == "True") {
		std::cout << "Add User? (y/n)";
		cin >> input1;
		if (input1 == 'y') {
			cin.ignore();
			user_add();
			break;
		}
		else if (input1 == 'n') {
			break;
		}
		else { 
			system("cls");
			std::cout << "Invalid Answer! Retry" << endl;
			continue;
		}
	}
	cin.ignore();
	system("cls");
	std::cout << "User Login, please Enter Login Credentials" << endl;
	string username, password;
	std::cout << "Username: ";
	std::getline(cin, username);
	std::cout << endl << "Password: ";
	hide_input();
	std::getline(cin, password);
	show_input();
	if (is_valid_credentials(username, password)) {
		std::cout << endl << "Nice to meet you" << endl;
	}
	else {
		std::cout << endl << "Login Credentials don't match!" << endl;
	}

	return 0;
}