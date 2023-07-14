#include <em/detail/header/MapHeader.h>
#include <utility/Exceptions.h>

#include <unordered_map>

using namespace em::detail::header;

MapHeader::MapHeader(std::unique_ptr<HeaderData> data) {
    this->data = std::move(data);
}

MapHeader::~MapHeader() = default;

unsigned int MapHeader::get_byte_size() const {
    if (byte_sizes.contains(get_data_type()) == false) {
        throw except::parse_error("MRCHeader::get_byte_size: Unknown data type.");
    };
    return byte_sizes.at(get_data_type());
}

HeaderData* MapHeader::get_data() const noexcept {
    return data.get();}

void MapHeader::set_data(std::unique_ptr<HeaderData> data) {this->data = std::move(data);}

std::ostream& em::detail::header::operator<<(std::ostream& os, const MapHeader& h) {
    return os << h.to_string();
}