//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2013-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include "boost/container/set.hpp"
#include "boost/container/flat_set.hpp"
#include <set>
#include <vector>
#include <iostream>
#include <boost/timer/timer.hpp>
#include <algorithm>
#include <exception>

using boost::timer::cpu_timer;
using boost::timer::cpu_times;
using boost::timer::nanosecond_type;

#ifdef NDEBUG
static const std::size_t N = 5000;
#else
static const std::size_t N = 500;
#endif

void compare_times(cpu_times time_numerator, cpu_times time_denominator){
   std::cout << "----------------------------------------------" << '\n';
   std::cout << " wall        = " << ((double)time_numerator.wall/(double)time_denominator.wall) << std::endl;
   std::cout << "----------------------------------------------" << '\n' << std::endl;
}

std::vector<int> sorted_unique_range;
std::vector<int> sorted_range;
std::vector<int> random_unique_range;
std::vector<int> random_range;

void fill_ranges()
{
   sorted_unique_range.resize(N);
   sorted_range.resize(N);
   random_unique_range.resize(N);
   random_range.resize(N);
   std::srand (0);
   //random_range
   std::generate(random_unique_range.begin(), random_unique_range.end(), std::rand);
   random_unique_range.erase(std::unique(random_unique_range.begin(), random_unique_range.end()), random_unique_range.end());
   //random_range
   random_range = random_unique_range;
   random_range.insert(random_range.end(), random_unique_range.begin(), random_unique_range.end());
   //sorted_unique_range
   for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
      sorted_unique_range[i] = static_cast<int>(i);
   }
   //sorted_range
   sorted_range = sorted_unique_range;
   sorted_range.insert(sorted_range.end(), sorted_unique_range.begin(), sorted_unique_range.end());
   std::sort(sorted_range.begin(), sorted_range.end());
}

template<typename T>
cpu_times construct_time()
{
   cpu_timer sur_timer, sr_timer, rur_timer, rr_timer, copy_timer, assign_timer, destroy_timer;
   //sur_timer.stop();sr_timer.stop();rur_timer.stop();rr_timer.stop();destroy_timer.stop();

   cpu_timer total_time;
   total_time.resume();

   for(std::size_t i = 0; i != N; ++i){
      {
         sur_timer.resume();
         T t(sorted_unique_range.begin(), sorted_unique_range.end());
         sur_timer.stop();
      }
      {
         sr_timer.resume();
         T t(sorted_range.begin(), sorted_range.end());
         sr_timer.stop();
         copy_timer.resume();
         T taux(t);
         copy_timer.stop();
         assign_timer.resume();
         t = taux;;
         assign_timer.stop();
      }
      {
         rur_timer.resume();
         T t(random_unique_range.begin(), random_unique_range.end());
         rur_timer.stop();
      }
      {
         rr_timer.resume();
         T t(random_range.begin(), random_range.end());
         rr_timer.stop();
         destroy_timer.resume();
      }
      destroy_timer.stop();
   }
   total_time.stop();

   std::cout << " Construct sorted_unique_range " << boost::timer::format(sur_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Construct sorted_range        " << boost::timer::format(sr_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Copy sorted range             " << boost::timer::format(copy_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Assign sorted range           " << boost::timer::format(assign_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Construct random_unique_range " << boost::timer::format(rur_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Construct random_range        " << boost::timer::format(rr_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Destroy                       " << boost::timer::format(destroy_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Total time =                  " << boost::timer::format(total_time.elapsed(), boost::timer::default_places, "%ws wall\n") << std::endl;
   return total_time.elapsed();
}

template<typename T>
cpu_times insert_time()
{
   cpu_timer sur_timer,sr_timer,rur_timer,rr_timer,destroy_timer;
   sur_timer.stop();sr_timer.stop();rur_timer.stop();rr_timer.stop();

   cpu_timer total_time;
   total_time.resume();

   for(std::size_t i = 0; i != N; ++i){
      {
         sur_timer.resume();
         T t;
         t.insert(sorted_unique_range.begin(), sorted_unique_range.end());
         sur_timer.stop();
      }
      {
         sr_timer.resume();
         T t;
         t.insert(sorted_range.begin(), sorted_range.end());
         sr_timer.stop();
      }
      {
         rur_timer.resume();
         T t;
         t.insert(random_unique_range.begin(), random_unique_range.end());
         rur_timer.stop();
      }
      {
         rr_timer.resume();
         T t;
         t.insert(random_range.begin(), random_range.end());
         rr_timer.stop();
      }
   }
   total_time.stop();

   std::cout << " Insert sorted_unique_range " << boost::timer::format(sur_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Insert sorted_range        " << boost::timer::format(sr_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Insert random_unique_range " << boost::timer::format(rur_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Insert random_range        " << boost::timer::format(rr_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Total time =               " << boost::timer::format(total_time.elapsed(), boost::timer::default_places, "%ws wall\n") << std::endl;
   return total_time.elapsed();
}

template<typename T>
cpu_times search_time()
{
   cpu_timer find_timer, lower_timer, upper_timer, equal_range_timer, count_timer;

   T t(sorted_unique_range.begin(), sorted_unique_range.end());

   cpu_timer total_time;
   total_time.resume();

   for(std::size_t i = 0; i != N; ++i){
      //Find
      {
         std::size_t found = 0;
         find_timer.resume();
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            found += static_cast<std::size_t>(t.end() != t.find(sorted_unique_range[i]));
         }
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            found += static_cast<std::size_t>(t.end() != t.find(sorted_unique_range[i]));
         }
         find_timer.stop();
         if(found/2 != t.size()){
            std::cout << "ERROR! all elements not found" << std::endl;
         }
      }
      //Lower
      {
         std::size_t found = 0;
         lower_timer.resume();
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            found += static_cast<std::size_t>(t.end() != t.lower_bound(sorted_unique_range[i]));
         }
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            found += static_cast<std::size_t>(t.end() != t.lower_bound(sorted_unique_range[i]));
         }
         lower_timer.stop();
         if(found/2 != t.size()){
            std::cout << "ERROR! all elements not found" << std::endl;
         }
      }
      //Upper
      {
         std::size_t found = 0;
         upper_timer.resume();
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            found += static_cast<std::size_t>(t.end() != t.upper_bound(sorted_unique_range[i]));
         }
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            found += static_cast<std::size_t>(t.end() != t.upper_bound(sorted_unique_range[i]));
         }
         upper_timer.stop();
         if(found/2 != (t.size()-1)){
            std::cout << "ERROR! all elements not found" << std::endl;
         }
      }
      //Equal
      {
         std::size_t found = 0;
         std::pair<typename T::iterator,typename T::iterator> ret;
         equal_range_timer.resume();
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            ret = t.equal_range(sorted_unique_range[i]);
            found += static_cast<std::size_t>(ret.first != ret.second);
         }
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            ret = t.equal_range(sorted_unique_range[i]);
            found += static_cast<std::size_t>(ret.first != ret.second);
         }
         equal_range_timer.stop();
         if(found/2 != t.size()){
            std::cout << "ERROR! all elements not found" << std::endl;
         }
      }
      //Count
      {
         std::size_t found = 0;
         std::pair<typename T::iterator,typename T::iterator> ret;
         count_timer.resume();
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            found += static_cast<std::size_t>(t.count(sorted_unique_range[i]));
         }
         for(std::size_t i = 0, max = sorted_unique_range.size(); i != max; ++i){
            found += static_cast<std::size_t>(t.count(sorted_unique_range[i]));
         }
         count_timer.stop();
         if(found/2 != t.size()){
            std::cout << "ERROR! all elements not found" << std::endl;
         }
      }
   }
   total_time.stop();

   std::cout << " Find         " << boost::timer::format(find_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Lower Bound  " << boost::timer::format(lower_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Upper Bound  " << boost::timer::format(upper_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Equal Range  " << boost::timer::format(equal_range_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Count        " << boost::timer::format(count_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Total time = " << boost::timer::format(total_time.elapsed(), boost::timer::default_places, "%ws wall\n") << std::endl;
   return total_time.elapsed();
}

template<typename T>
void extensions_time()
{
   cpu_timer sur_timer,sur_opt_timer;
   sur_timer.stop();sur_opt_timer.stop();

   for(std::size_t i = 0; i != N; ++i){
      {
         sur_timer.resume();
         T t(sorted_unique_range.begin(), sorted_unique_range.end());
         sur_timer.stop();
      }
      {
         sur_opt_timer.resume();
         T t(boost::container::ordered_unique_range, sorted_unique_range.begin(), sorted_unique_range.end());
         sur_opt_timer.stop();
      }

   }
   std::cout << " Construct sorted_unique_range             " << boost::timer::format(sur_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << " Construct sorted_unique_range (extension) " << boost::timer::format(sur_opt_timer.elapsed(), boost::timer::default_places, "%ws wall\n");
   std::cout << "Total time (Extension/Standard):\n";
   compare_times(sur_opt_timer.elapsed(), sur_timer.elapsed());
}

template<class BoostClass, class StdClass>
void launch_tests(const char *BoostContName, const char *StdContName)
{
   try {
      fill_ranges();
      {
         std::cout << "Construct benchmark:" << BoostContName << std::endl;
         cpu_times boost_set_time = construct_time< BoostClass >();

         std::cout << "Construct benchmark:" << StdContName << std::endl;
         cpu_times std_set_time = construct_time< StdClass >();

         std::cout << "Total time (" << BoostContName << "/" << StdContName << "):\n";
         compare_times(boost_set_time, std_set_time);
      }
      {
         std::cout << "Insert benchmark:" << BoostContName << std::endl;
         cpu_times boost_set_time = insert_time< BoostClass >();

         std::cout << "Insert benchmark:" << StdContName << std::endl;
         cpu_times std_set_time = insert_time< StdClass >();

         std::cout << "Total time (" << BoostContName << "/" << StdContName << "):\n";
         compare_times(boost_set_time, std_set_time);
      }
      {
         std::cout << "Search benchmark:" << BoostContName << std::endl;
         cpu_times boost_set_time = search_time< BoostClass >();

         std::cout << "Search benchmark:" << StdContName << std::endl;
         cpu_times std_set_time = search_time< StdClass >();

         std::cout << "Total time (" << BoostContName << "/" << StdContName << "):\n";
         compare_times(boost_set_time, std_set_time);
      }
      {
         std::cout << "Extensions benchmark:" << BoostContName << std::endl;
         extensions_time< BoostClass >();
      }

   }catch(std::exception e){
      std::cout << e.what();
   }
}

int main()
{
   //set vs std::set
   launch_tests< boost::container::set<int> , std::set<int> >
      ("boost::container::set<int>", "std::set<int>");/*
   //multiset vs std::set
   launch_tests< boost::container::multiset<int> , std::multiset<int> >
      ("boost::container::multiset<int>", "std::multiset<int>");*/
   //flat_set vs set
   //launch_tests< boost::container::flat_set<int> , boost::container::set<int> >
      //("boost::container::flat_set<int>", "boost::container::set<int>");
   //flat_multiset vs multiset
   //launch_tests< boost::container::flat_multiset<int> , boost::container::multiset<int> >
      //("boost::container::flat_multiset<int>", "boost::container::multiset<int>");
   return 1;
}
