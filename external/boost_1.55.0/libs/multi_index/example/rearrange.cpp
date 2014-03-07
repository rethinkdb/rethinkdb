/* Boost.MultiIndex example of use of rearrange facilities.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#include <boost/config.hpp>
#include <boost/detail/iterator.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/random/binomial_distribution.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

using boost::multi_index_container;
using namespace boost::multi_index;

/* We model a card deck with a random access array containing
 * card numbers (from 0 to 51), supplemented with an additional
 * index which retains the start ordering.
 */

class deck
{
  BOOST_STATIC_CONSTANT(std::size_t,num_cards=52);

  typedef multi_index_container<
    int,
    indexed_by<
      random_access<>, /* base index    */
      random_access<>  /* "start" index */
    >
  >                              container_type;
  container_type cont;

public:
  deck()
  {
    cont.reserve(num_cards);
    get<1>(cont).reserve(num_cards);
    for(std::size_t i=0;i<num_cards;++i)cont.push_back(i);
  }

  typedef container_type::iterator  iterator;
  typedef container_type::size_type size_type;

  iterator  begin()const{return cont.begin();}
  iterator  end()const{return cont.end();}
  size_type size()const{return cont.size();}

  template<typename InputIterator>
  void rearrange(InputIterator it)
  {
    cont.rearrange(it);
  }

  void reset()
  {
    /* simply rearrange the base index like the start index */

    cont.rearrange(get<1>(cont).begin());
  }

  std::size_t position(int i)const
  {
    /* The position of a card in the deck is calculated by locating
     * the card through the start index (which is ordered), projecting
     * to the base index and diffing with the begin position.
     * Resulting complexity: constant.
     */

    return project<0>(cont,get<1>(cont).begin()+i)-cont.begin();
  }

  std::size_t rising_sequences()const
  {
    /* Iterate through all cards and increment the sequence count
     * when the current position is left to the previous.
     * Resulting complexity: O(n), n=num_cards.
     */

    std::size_t s=1;
    std::size_t last_pos=0;

    for(std::size_t i=0;i<num_cards;++i){
      std::size_t pos=position(i);
      if(pos<last_pos)++s;
      last_pos=pos;
    }

    return s;
  }
};

/* A vector of reference_wrappers to deck elements can be used
 * as a view to the deck container.
 * We use a special implicit_reference_wrapper having implicit
 * ctor from its base type, as this simplifies the use of generic
 * techniques on the resulting data structures.
 */

template<typename T>
class implicit_reference_wrapper:public boost::reference_wrapper<T>
{
private:
  typedef boost::reference_wrapper<T> super;
public:
  implicit_reference_wrapper(T& t):super(t){}
};

typedef std::vector<implicit_reference_wrapper<const int> > deck_view;

/* Riffle shuffle is modeled like this: A cut is selected in the deck
 * following a binomial distribution. Then, cards are randomly selected
 * from one packet or the other with probability proportional to
 * packet size.
 */

template<typename RandomAccessIterator,typename OutputIterator>
void riffle_shuffle(
  RandomAccessIterator first,RandomAccessIterator last,
  OutputIterator out)
{
  static boost::mt19937 rnd_gen;

  typedef typename boost::detail::iterator_traits<
    RandomAccessIterator>::difference_type         difference_type;
  typedef boost::binomial_distribution<
    difference_type>                               rnd_cut_select_type;
  typedef boost::uniform_real<>                    rnd_deck_select_type;

  rnd_cut_select_type  cut_select(last-first);
  RandomAccessIterator middle=first+cut_select(rnd_gen);
  difference_type      s0=middle-first;
  difference_type      s1=last-middle;
  rnd_deck_select_type deck_select;

  while(s0!=0&&s1!=0){
    if(deck_select(rnd_gen)<(double)s0/(s0+s1)){
      *out++=*first++;
      --s0;
    }
    else{
      *out++=*middle++;
      --s1;
    }
  }
  std::copy(first,first+s0,out);
  std::copy(middle,middle+s1,out);
}

struct riffle_shuffler
{
  void operator()(deck& d)const
  {
    dv.clear();
    dv.reserve(d.size());
    riffle_shuffle(
      d.begin(),d.end(),std::back_inserter(dv)); /* do the shuffling  */
    d.rearrange(dv.begin());                     /* apply to the deck */
  }

private:
  mutable deck_view dv;
};

/* A truly random shuffle (up to stdlib implementation quality) using
 * std::random_shuffle.
 */

struct random_shuffler
{
  void operator()(deck& d)const
  {
    dv.clear();
    dv.reserve(d.size());
    std::copy(d.begin(),d.end(),std::back_inserter(dv));
    std::random_shuffle(dv.begin(),dv.end()); /* do the shuffling  */
    d.rearrange(dv.begin());                  /* apply to the deck */
  }

private:
  mutable deck_view dv;
};

/* Repeat a given shuffling algorithm repeats_num times
 * and obtain the resulting rising sequences number. Average
 * for tests_num trials.
 */

template<typename Shuffler>
double shuffle_test(
 unsigned int repeats_num,unsigned int tests_num
 BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Shuffler))
{
  deck          d;
  Shuffler      sh;
  unsigned long total=0;

  for(unsigned int n=0;n<tests_num;++n){
    for(unsigned m=0;m<repeats_num;++m)sh(d);
    total+=d.rising_sequences();
    d.reset();
  }

  return (double)total/tests_num;
}

int main()
{
  unsigned rifs_num=0;
  unsigned tests_num=0;

  std::cout<<"number of riffle shuffles (vg 5):";
  std::cin>>rifs_num;
  std::cout<<"number of tests (vg 1000):";
  std::cin>>tests_num;

  std::cout<<"shuffling..."<<std::endl;

  std::cout<<"riffle shuffling\n"
             "  avg number of rising sequences: "
           <<shuffle_test<riffle_shuffler>(rifs_num,tests_num)
           <<std::endl;

  std::cout<<"random shuffling\n"
             "  avg number of rising sequences: "
           <<shuffle_test<random_shuffler>(1,tests_num)
           <<std::endl;

  return 0;
}
