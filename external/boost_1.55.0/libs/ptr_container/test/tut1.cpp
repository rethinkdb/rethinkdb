//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2003-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//

//
// This example is intended to get you started.
// Notice how the smart container
//
// 1. takes ownership of objects
// 2. transfers ownership
// 3. applies indirection to iterators 
// 4. clones objects from other smart containers
// 

//
// First we select which container to use.
//
#include <boost/ptr_container/ptr_deque.hpp>

//
// we need these later in the example
//
#include <boost/assert.hpp>
#include <string>
#include <exception>


//
// Then we define a small polymorphic class
// hierarchy.
// 

class animal : boost::noncopyable
{
    virtual std::string do_speak() const = 0;
    std::string name_;

protected:
    //
    // Animals cannot be copied...
    //
    animal( const animal& r ) : name_( r.name_ )           { }
    void operator=( const animal& );

private:
    //
    // ...but due to advances in genetics, we can clone them!
    //

    virtual animal* do_clone() const = 0;
        
public:
    animal( const std::string& name ) : name_(name)        { }
    virtual ~animal() throw()                              { }
    
    std::string speak() const
    {
        return do_speak();
    }

    std::string name() const
    {
        return name_;
    }

    animal* clone() const
    {
        return do_clone();
    }
};

//
// An animal is still not Clonable. We need this last hook.
//
// Notice that we pass the animal by const reference
// and return by pointer.
//

animal* new_clone( const animal& a )
{
    return a.clone();
}

//
// We do not need to define 'delete_clone()' since
// since the default is to call the default 'operator delete()'.
//

const std::string muuuh = "Muuuh!";
const std::string oiink = "Oiiink";

class cow : public animal
{
    virtual std::string do_speak() const
    {
        return muuuh;
    }

    virtual animal* do_clone() const
    {
        return new cow( *this );
    }

public:
    cow( const std::string& name ) : animal(name)          { }
};

class pig : public animal
{
    virtual std::string do_speak() const
    {
        return oiink;
    }

    virtual animal* do_clone() const
    {
        return new pig( *this );
    }
    
public:
    pig( const std::string& name ) : animal(name)          { }
};

//
// Then we, of course, need a place to put all
// those animals.
//

class farm
{
    //
    // This is where the smart containers are handy
    //
    typedef boost::ptr_deque<animal> barn_type;
    barn_type                        barn;

    //
    // An error type
    //
    struct farm_trouble : public std::exception           { };

public:
    // 
    // We would like to make it possible to
    // iterate over the animals in the farm
    //
    typedef barn_type::iterator  animal_iterator;

    //
    // We also need to count the farm's size...
    //
    typedef barn_type::size_type size_type;
    
    //
    // And we also want to transfer an animal
    // safely around. The easiest way to think
    // about '::auto_type' is to imagine a simplified
    // 'std::auto_ptr<T>' ... this means you can expect
    // 
    //   T* operator->()
    //   T* release()
    //   deleting destructor
    //
    // but not more.
    //
    typedef barn_type::auto_type  animal_transport;

    // 
    // Create an empty farm.
    //
    farm()                                                 { }
    
    //
    // We need a constructor that can make a new
    // farm by cloning a range of animals.
    //
    farm( animal_iterator begin, animal_iterator end )
     : 
        //
        // Objects are always cloned before insertion
        // unless we explicitly add a pointer or 
        // use 'release()'. Therefore we actually
        // clone all animals in the range
        //
        barn( begin, end )                               { }
    
    //
    // ... so we need some other function too
    //

    animal_iterator begin()
    {
        return barn.begin();
    }

    animal_iterator end()
    {
        return barn.end();
    }
    
    //
    // Here it is quite ok to have an 'animal*' argument.
    // The smart container will handle all ownership
    // issues.
    //
    void buy_animal( animal* a )
    {
        barn.push_back( a );
    }

    //
    // The farm can also be in economical trouble and
    // therefore be in the need to sell animals.
    //
    animal_transport sell_animal( animal_iterator to_sell )
    {
        if( to_sell == end() )
            throw farm_trouble();

        //
        // Here we remove the animal from the barn,
        // but the animal is not deleted yet...it's
        // up to the buyer to decide what
        // to do with it.
        //
        return barn.release( to_sell );
    }

    //
    // How big a farm do we have?
    //
    size_type size() const
    {
        return barn.size();
    }

    //
    // If things are bad, we might choose to sell all animals :-(
    //
    std::auto_ptr<barn_type> sell_farm()
    {
        return barn.release();
    }

    //
    // However, if things are good, we might buy somebody
    // else's farm :-)
    //

    void buy_farm( std::auto_ptr<barn_type> other )
    {
        //
        // This line inserts all the animals from 'other'
        // and is guaranteed either to succeed or to have no
        // effect
        //
        barn.transfer( barn.end(), // insert new animals at the end
                         *other );     // we want to transfer all animals,
                                       // so we use the whole container as argument
        //
        // You might think you would have to do
        //
        // other.release();
        //
        // but '*other' is empty and can go out of scope as it wants
        //
        BOOST_ASSERT( other->empty() );
    }
    
}; // class 'farm'.

int main()
{
    //
    // First we make a farm
    //
    farm animal_farm;
    BOOST_ASSERT( animal_farm.size() == 0u );
    
    animal_farm.buy_animal( new pig("Betty") );
    animal_farm.buy_animal( new pig("Benny") );
    animal_farm.buy_animal( new pig("Jeltzin") );
    animal_farm.buy_animal( new cow("Hanz") );
    animal_farm.buy_animal( new cow("Mary") );
    animal_farm.buy_animal( new cow("Frederik") );
    BOOST_ASSERT( animal_farm.size() == 6u );

    //
    // Then we make another farm...it will actually contain
    // a clone of the other farm.
    //
    farm new_farm( animal_farm.begin(), animal_farm.end() );
    BOOST_ASSERT( new_farm.size() == 6u );

    //
    // Is it really clones in the new farm?
    //
    BOOST_ASSERT( new_farm.begin()->name() == "Betty" );
    
    //
    // Then we search for an animal, Mary (the Crown Princess of Denmark),
    // because we would like to buy her ...
    //
    typedef farm::animal_iterator iterator;
    iterator to_sell;
    for( iterator i   = animal_farm.begin(),
                  end = animal_farm.end();
         i != end; ++i )
    {
        if( i->name() == "Mary" )
        {
            to_sell = i;
            break;
        }
    }

    farm::animal_transport mary = animal_farm.sell_animal( to_sell );


    if( mary->speak() == muuuh )
        //
        // Great, Mary is a cow, and she may live longer
        //
        new_farm.buy_animal( mary.release() );
    else
        //
        // Then the animal would be destroyed (!)
        // when we go out of scope.
        //
        ;

    //
    // Now we can observe some changes to the two farms...
    //
    BOOST_ASSERT( animal_farm.size() == 5u );
    BOOST_ASSERT( new_farm.size()    == 7u );

    //
    // The new farm has however underestimated how much
    // it cost to feed Mary and its owner is forced to sell the farm...
    //
    animal_farm.buy_farm( new_farm.sell_farm() );

    BOOST_ASSERT( new_farm.size()    == 0u );
    BOOST_ASSERT( animal_farm.size() == 12u );     
}
