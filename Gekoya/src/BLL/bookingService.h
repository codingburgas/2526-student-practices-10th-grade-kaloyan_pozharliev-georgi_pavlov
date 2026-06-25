#pragma once
#include "../DAL/BookingRepository.h"
#include <string>
#include <vector>

class BookingService
{
public:
    static bool AddBooking(const std::string& username,
    const std::string& movieTitle,
    const std::string& showtime,
    const std::string& seatType,
    const std::string& bookingDate,
    int price);

    static std::vector <Booking> GetUserBookings(const std::string& username);
    static bool CancelBooking(const std::string& id);
};