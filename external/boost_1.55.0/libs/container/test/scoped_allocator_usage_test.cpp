#include <boost/container/detail/config_begin.hpp>
#include <memory>

#include <boost/move/utility.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/deque.hpp>
#include <boost/container/list.hpp>
#include <boost/container/slist.hpp>
#include <boost/container/stable_vector.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/map.hpp>
#include <boost/container/set.hpp>
#include <boost/container/detail/mpl.hpp>

#include <boost/container/scoped_allocator.hpp>

template <typename Ty>
class SimpleAllocator
{
public:
	typedef Ty value_type;

	SimpleAllocator(int value)
		: m_state(value)
	{}

	template <typename T>
	SimpleAllocator(const SimpleAllocator<T> &other)
		: m_state(other.m_state)
	{}

	Ty* allocate(std::size_t n)
	{
		return m_allocator.allocate(n);
	}

	void deallocate(Ty* p, std::size_t n)
	{
		m_allocator.deallocate(p, n);
	}

   int get_value() const
   {  return m_state;   }

   private:
	int m_state;
	std::allocator<Ty> m_allocator;

	template <typename T> friend class SimpleAllocator;
};

class alloc_int
{
   private: // Not copyable

   BOOST_MOVABLE_BUT_NOT_COPYABLE(alloc_int)

   public:
	typedef SimpleAllocator<int> allocator_type;

	alloc_int(BOOST_RV_REF(alloc_int)other)
		: m_value(other.m_value), m_allocator(boost::move(other.m_allocator))
	{
		other.m_value = -1;
	}

	alloc_int(BOOST_RV_REF(alloc_int)other, const allocator_type &allocator)
		: m_value(other.m_value), m_allocator(allocator)
	{
		other.m_value = -1;
	}

	alloc_int(int value, const allocator_type &allocator)
		: m_value(value), m_allocator(allocator)
	{}

	alloc_int & operator=(BOOST_RV_REF(alloc_int)other)
	{
		other.m_value = other.m_value;
      return *this;
	}

   int get_allocator_state() const
   {  return m_allocator.get_value();  }

   int get_value() const
   {  return m_value;   }

   friend bool operator < (const alloc_int &l, const alloc_int &r)
   {  return l.m_value < r.m_value;  }

   friend bool operator == (const alloc_int &l, const alloc_int &r)
   {  return l.m_value == r.m_value;  }

   private:
	int m_value;
	allocator_type m_allocator;
};

using namespace ::boost::container;

//general allocator
typedef scoped_allocator_adaptor<SimpleAllocator<alloc_int> > AllocIntAllocator;

//[multi]map/set
typedef std::pair<const alloc_int, alloc_int> MapNode;
typedef scoped_allocator_adaptor<SimpleAllocator<MapNode> > MapAllocator;
typedef map<alloc_int, alloc_int, std::less<alloc_int>, MapAllocator> Map;
typedef set<alloc_int, std::less<alloc_int>, AllocIntAllocator> Set;
typedef multimap<alloc_int, alloc_int, std::less<alloc_int>, MapAllocator> MultiMap;
typedef multiset<alloc_int, std::less<alloc_int>, AllocIntAllocator> MultiSet;

//[multi]flat_map/set
typedef std::pair<alloc_int, alloc_int> FlatMapNode;
typedef scoped_allocator_adaptor<SimpleAllocator<FlatMapNode> > FlatMapAllocator;
typedef flat_map<alloc_int, alloc_int, std::less<alloc_int>, MapAllocator> FlatMap;
typedef flat_set<alloc_int, std::less<alloc_int>, AllocIntAllocator> FlatSet;
typedef flat_multimap<alloc_int, alloc_int, std::less<alloc_int>, MapAllocator> FlatMultiMap;
typedef flat_multiset<alloc_int, std::less<alloc_int>, AllocIntAllocator> FlatMultiSet;

//vector, deque, list, slist, stable_vector.
typedef vector<alloc_int, AllocIntAllocator>          Vector;
typedef deque<alloc_int, AllocIntAllocator>           Deque;
typedef list<alloc_int, AllocIntAllocator>            List;
typedef slist<alloc_int, AllocIntAllocator>           Slist;
typedef stable_vector<alloc_int, AllocIntAllocator>   StableVector;

/////////
//is_unique_assoc
/////////

template<class T>
struct is_unique_assoc
{
   static const bool value = false;
};

template<class K, class V, class C, class A>
struct is_unique_assoc< map<K, V, C, A> >
{
   static const bool value = true;
};

template<class K, class V, class C, class A>
struct is_unique_assoc< flat_map<K, V, C, A> >
{
   static const bool value = true;
};

template<class V, class C, class A>
struct is_unique_assoc< set<V, C, A> >
{
   static const bool value = true;
};

template<class V, class C, class A>
struct is_unique_assoc< flat_set<V, C, A> >
{
   static const bool value = true;
};


/////////
//is_map
/////////

template<class T>
struct is_map
{
   static const bool value = false;
};

template<class K, class V, class C, class A>
struct is_map< map<K, V, C, A> >
{
   static const bool value = true;
};

template<class K, class V, class C, class A>
struct is_map< flat_map<K, V, C, A> >
{
   static const bool value = true;
};

template<class K, class V, class C, class A>
struct is_map< multimap<K, V, C, A> >
{
   static const bool value = true;
};

template<class K, class V, class C, class A>
struct is_map< flat_multimap<K, V, C, A> >
{
   static const bool value = true;
};

template<class T>
struct is_set
{
   static const bool value = false;
};

template<class V, class C, class A>
struct is_set< set<V, C, A> >
{
   static const bool value = true;
};

template<class V, class C, class A>
struct is_set< flat_set<V, C, A> >
{
   static const bool value = true;
};

template<class V, class C, class A>
struct is_set< multiset<V, C, A> >
{
   static const bool value = true;
};

template<class V, class C, class A>
struct is_set< flat_multiset<V, C, A> >
{
   static const bool value = true;
};

/////////
//container_wrapper
/////////

template< class Container
        , bool Assoc = is_set<Container>::value || is_map<Container>::value
        , bool UniqueAssoc = is_unique_assoc<Container>::value
        , bool Map  = is_map<Container>::value
        >
struct container_wrapper
   : public Container
{
   typedef typename Container::allocator_type   allocator_type;
  
   container_wrapper(const allocator_type &a)
      : Container(a)
   {}
};

template<class Container>  //map
struct container_wrapper<Container, true, true, true>
   : public Container
{
   typedef typename Container::allocator_type   allocator_type;
   typedef typename Container::key_compare      key_compare;
   typedef typename Container::value_type       value_type;
   typedef typename Container::const_iterator   const_iterator;
   typedef typename Container::iterator         iterator;

   container_wrapper(const allocator_type &a)
      : Container(key_compare(), a)
   {}

   template<class Arg>
   iterator emplace(const_iterator, const Arg &arg)
   {
      return this->Container::emplace(arg, arg).first;
   }
};

template<class Container>  //set
struct container_wrapper<Container, true, true, false>
   : public Container
{
   typedef typename Container::allocator_type   allocator_type;
   typedef typename Container::key_compare      key_compare;
   typedef typename Container::value_type       value_type;
   typedef typename Container::const_iterator   const_iterator;
   typedef typename Container::iterator         iterator;

   container_wrapper(const allocator_type &a)
      : Container(key_compare(), a)
   {}

   template<class Arg>
   iterator emplace(const_iterator, const Arg &arg)
   {
      return this->Container::emplace(arg).first;
   }
};

template<class Container>  //multimap
struct container_wrapper<Container, true, false, true>
   : public Container
{
   typedef typename Container::value_type       value_type;
   typedef typename Container::key_compare      key_compare;
   typedef typename Container::allocator_type   allocator_type;
   typedef typename Container::const_iterator   const_iterator;
   typedef typename Container::iterator         iterator;

   container_wrapper(const allocator_type &a)
      : Container(key_compare(), a)
   {}

   template<class Arg>
   iterator emplace(const_iterator, const Arg &arg)
   {
      return this->Container::emplace(arg, arg);
   }
};

//multiset
template<class Container>  //multimap
struct container_wrapper<Container, true, false, false>
   : public Container
{
   typedef typename Container::value_type       value_type;
   typedef typename Container::key_compare      key_compare;
   typedef typename Container::allocator_type   allocator_type;
   typedef typename Container::const_iterator   const_iterator;
   typedef typename Container::iterator         iterator;

   container_wrapper(const allocator_type &a)
      : Container(key_compare(), a)
   {}

   template<class Arg>
   iterator emplace(const_iterator, const Arg &arg)
   {
      return this->Container::emplace(arg);
   }
};

bool test_value_and_state_equals(const alloc_int &r, int value, int state)
{  return r.get_value() == value && r.get_allocator_state() == state;  }

template<class F, class S>
bool test_value_and_state_equals(const container_detail::pair<F, S> &p, int value, int state)
{  return test_value_and_state_equals(p.first, value, state) && test_alloc_state_equals(p.second, value, state);  }

template<class F, class S>
bool test_value_and_state_equals(const std::pair<F, S> &p, int value, int state)
{  return test_value_and_state_equals(p.first, value, state) && test_value_and_state_equals(p.second, value, state);  }

template<class Container>
bool one_level_allocator_propagation_test()
{
   typedef container_wrapper<Container> ContainerWrapper;
   typedef typename ContainerWrapper::iterator iterator;
	ContainerWrapper c(SimpleAllocator<MapNode>(5));

   c.clear();
	iterator it = c.emplace(c.cbegin(), 42);

   if(!test_value_and_state_equals(*it, 42, 5))
      return false;

   return true;
}

int main()
{
   //unique assoc
   if(!one_level_allocator_propagation_test<FlatMap>())
      return 1;
   if(!one_level_allocator_propagation_test<Map>())
      return 1;
   if(!one_level_allocator_propagation_test<FlatSet>())
      return 1;
   if(!one_level_allocator_propagation_test<Set>())
      return 1;
   //multi assoc
   if(!one_level_allocator_propagation_test<FlatMultiMap>())
      return 1;
   if(!one_level_allocator_propagation_test<MultiMap>())
      return 1;
   if(!one_level_allocator_propagation_test<FlatMultiSet>())
      return 1;
   if(!one_level_allocator_propagation_test<MultiSet>())
      return 1;
   //sequence containers
   if(!one_level_allocator_propagation_test<Vector>())
      return 1;
   if(!one_level_allocator_propagation_test<Deque>())
      return 1;
   if(!one_level_allocator_propagation_test<List>())
      return 1;
   if(!one_level_allocator_propagation_test<Slist>())
      return 1;
   if(!one_level_allocator_propagation_test<StableVector>())
      return 1;
	return 0;
}

#include <boost/container/detail/config_end.hpp>
