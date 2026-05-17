#pragma once
#include <string>

struct User
{
    std::string username;
    std::string password;
    std::string email;
    int accessLevel;
};

class UserRepository
{
public:
    static bool InsertUser(const std::string& username, const std::string& password, const std::string& email, int accessLevel);
    static bool UserExists(const std::string& username);
    static bool ValidateUser(const std::string& username, const std::string& password);
    static int  GetUserAccessLevel(const std::string& username);
};