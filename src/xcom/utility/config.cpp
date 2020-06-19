//
// Created by cx on 2020-06-19.
//
#include <xcom/utility/config.hpp>




namespace cx::config
{
    KeyConfiguration::KeyConfiguration(xcb_keysym_t symbol, cx::u16 modifier) : symbol(symbol), modifier(modifier) {

    }
    bool operator<(const KeyConfiguration& lhs, const KeyConfiguration& rhs) {
        if(lhs.symbol == rhs.symbol) {
            return lhs.modifier < rhs.modifier;
        }
        return lhs.symbol < rhs.symbol;
    }
    bool operator==(const KeyConfiguration& lhs, const KeyConfiguration& rhs) {
        return lhs.symbol == rhs.symbol && lhs.modifier == rhs.modifier;
    }
    bool operator>(const KeyConfiguration& lhs, const KeyConfiguration& rhs) {
        return !(lhs < rhs);
    }

} // namespace cx::config