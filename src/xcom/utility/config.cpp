//
// Created by cx on 2020-06-19.
//
#include <xcom/utility/config.hpp>
namespace cx::config {
    bool operator<(const KeyConfiguration& lhs,const KeyConfiguration& rhs) {
        if(lhs.symbol == rhs.symbol) {
            return lhs.modifier < rhs.modifier;
        }
        return lhs.symbol < rhs.symbol;
    }
}