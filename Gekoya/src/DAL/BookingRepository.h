#pragma once
#include <string>
#include <vector>

struct Booking
{
    std::string id;
    std::string username;
    std::string movieTitle;
    std::string showtime;
    std::string seatType;
    std::string bookingDate;
    int         price;
};

class BookingRepository
{
public:
    static bool                 InsertBooking(const Booking& booking);
    static std::vector<Booking> GetBookingsByUser(const std::string& username);
    static bool                 DeleteBooking(const std::string& id);
};
