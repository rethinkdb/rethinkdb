// Document/View sample for Boost.Signals2.
// Expands on doc_view_acm.cpp example by using boost::signals2::deconstruct
// as a post-constructing factory function.
//
// Copyright Keith MacDonald 2005.
// Copyright Frank Mori Hess 2009.
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// For more information, see http://www.boost.org

#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/signals2/deconstruct.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/shared_ptr.hpp>

class Document
{
public:
  typedef boost::signals2::signal<void ()>  signal_t;

public:
  Document()
  {}

  /* Connect a slot to the signal which will be emitted whenever
    text is appended to the document. */
  boost::signals2::connection connect(const signal_t::slot_type &subscriber)
  {
    return m_sig.connect(subscriber);
  }

  void append(const char* s)
  {
    m_text += s;
    m_sig();
  }

  const std::string& getText() const
  {
    return m_text;
  }

private:
  signal_t    m_sig;
  std::string m_text;
};

class TextView
{
public:
  /* This adl_postconstruct function will be found
  via argument-dependent lookup when using boost::signals2::deconstruct. */
  template<typename T>
    friend void adl_postconstruct(const boost::shared_ptr<T> &view_sp, TextView *view, Document& doc)
  {
    view->m_document = &doc;
    {
      typedef Document::signal_t::slot_type slot_type;
      slot_type myslot(&TextView::refresh, view);
      doc.connect(myslot.track(view_sp));
    }
  }

  void refresh() const
  {
    std::cout << "TextView: " << m_document->getText() << std::endl;
  }
private:
  // give boost::signals2::deconstruct access to private constructor
  friend class boost::signals2::deconstruct_access;
  // private constructor to force use of deconstruct
  TextView()
  {}

  Document*               m_document;
};

class HexView
{
public:
  /* This adl_postconstruct function will be found
  via argument-dependent lookup when using boost::signals2::deconstruct. */
  template<typename T>
    friend void adl_postconstruct(const boost::shared_ptr<T> &view_sp, HexView *view, Document& doc)
  {
    view->m_document = &doc;
    {
      typedef Document::signal_t::slot_type slot_type;
      slot_type myslot(&HexView::refresh, view);
      doc.connect(myslot.track(view_sp));
    }
  }

  void refresh() const
  {
    const std::string&  s = m_document->getText();

    std::cout << "HexView:";

    for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
      std::cout << ' ' << std::hex << static_cast<int>(*it);

    std::cout << std::endl;
  }
private:
  // give boost::signals2::deconstruct access to private constructor
  friend class boost::signals2::deconstruct_access;
  // private constructor to force use of deconstruct
  HexView()
  {}

  Document*               m_document;
};

namespace bs2 = boost::signals2;

int main(int argc, char* argv[])
{
  Document    doc;
  boost::shared_ptr<TextView> v1 = bs2::deconstruct<TextView>().postconstruct(boost::ref(doc));
  boost::shared_ptr<HexView> v2 = bs2::deconstruct<HexView>().postconstruct(boost::ref(doc));

  doc.append(argc >= 2 ? argv[1] : "Hello world!");

  v2.reset(); // destroy the HexView, automatically disconnecting
  doc.append("  HexView should no longer be connected.");

  return 0;
}
