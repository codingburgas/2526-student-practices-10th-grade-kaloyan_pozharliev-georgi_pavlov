#include "AuthService.h"
#include "../DAL/UserRepository.h"

bool AuthService::Login(const std::string& username, const std::string& password)
{
    return UserRepository::ValidateUser(username, password);
}

bool AuthService::Register(const std::string& username, const std::string& password, int accessLevel)
{
    if (UserRepository::UserExists(username))
        return false;
    return UserRepository::InsertUser(username, password, accessLevel);
}

int AuthService::GetAccessLevel(const std::string& username)
{
    return UserRepository::GetUserAccessLevel(username);
}
