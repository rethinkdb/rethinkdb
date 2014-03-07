/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_OBJECT_HPP
#define BOOST_PHOENIX_OBJECT_HPP

#ifdef BOOST_PHOENIX_USE_V2_OVER_V3
#include <boost/spirit/home/phoenix/object.hpp>
#else
#include <boost/phoenix/object/const_cast.hpp>
#include <boost/phoenix/object/construct.hpp>
#include <boost/phoenix/object/delete.hpp>
#include <boost/phoenix/object/dynamic_cast.hpp>
#include <boost/phoenix/object/new.hpp>
#include <boost/phoenix/object/reinterpret_cast.hpp>
#include <boost/phoenix/object/static_cast.hpp>
#endif

#endif
