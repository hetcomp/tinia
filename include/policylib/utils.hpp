#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>


/** \file utils.hpp
    This file contains various utilty function that is used throughout policylib.
    They are mostly intended for internal-use, but might also be convenient for
    other applications where libxml is used.
  */
namespace policylib {


template<typename U>
void print0(const boost::property_tree::basic_ptree<std::string, U> &pt, const int level) {
    //boost::property_tree::basic_ptree::const_iterator end = pt.end();
    auto end = pt.end();
    //boost::property_tree::basic_ptree<T, U>::const_iterator it = pt.begin();
    auto it = pt.begin();
    for ( ; it != end; ++it) {
        for (int i=0; i<level; i++)
            std::cout << " ";
        auto val = it->second;
        std::string valstr = "qwqw";
        //std::string valstr = it->second.get_value<std::string>();
        std::cout << it->first << ": " << valstr << std::endl;
        print0(it->second, level + 4);
    }
}

template<typename T, typename U>
void print(const boost::property_tree::basic_ptree<T, U> &pt) {
    std::cout << "------------- ptree -----------" << std::endl;
    print0(pt, 0);
    std::cout << "------------- ptree -----------" << std::endl;
}


}

#endif // UTILS_HPP






























