#include "BookingService.h"

bool BookingService::AddBooking(const std::string& username,
    const std::string& movieTitle,
    const std::string& showtime,
    const std::string& seatType,
    const std::string& bookingDate,
    int price)
{
    Booking b;
    b.username = username;
    b.movieTitle = movieTitle;
    b.showtime = showtime;
    b.seatType = seatType;
    b.bookingDate = bookingDate;
    b.price = price;
    return BookingRepository::InsertBooking(b);
}

std::vector<Booking> BookingService::GetUserBookings(const std::string& username)
{
    return BookingRepository::GetBookingsByUser(username);
}

bool BookingService::CancelBooking(const std::string& id)
{
    return BookingRepository::DeleteBooking(id);
}