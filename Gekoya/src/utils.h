#pragma once
#include <raylib.h>
#include <string>

enum AppState
{
    NIL = 0,
    AUTH,
    REG,
    MAIN,
    CINEMAS,
    TICKETS,
    PROFILE,
    SEARCH,
    BOOKING,
    PAYMENT,
    ADMIN,
};

struct SessionUser
{
    std::string username = "";
    std::string email = "";
    int         accessLevel = 1;
};