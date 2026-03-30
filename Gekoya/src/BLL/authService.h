#pragma once
#include <string>
class AuthService
{
public:
    static bool Login(const std::string& username, const std::string& password);
    static bool Register(const std::string& username, const std::string& password, int accessLevel);
    static int GetAccessLevel(const std::string& username);
};