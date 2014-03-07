// Document/View sample for Boost.Signals2.
// Expands on doc_view.cpp example by using automatic
// connection management.
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
#include <boost/signals2/signal.hpp>
#include <boost/shared_ptr.hpp>

class Document
{
public:
  typedef boost::signals2::signal<void ()>  signal_t;

public:
  Document()
  {}

  /* connect a slot to the signal which will be emitted whenever
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
  // static factory function that sets up automatic connection tracking
  static boost::shared_ptr<TextView> create(Document& doc)
  {
    boost::shared_ptr<TextView> new_view(new TextView(doc));
    {
      typedef Document::signal_t::slot_type slot_type;
      slot_type myslot(&TextView::refresh, new_view.get());
      doc.connect(myslot.track(new_view));
    }
    return new_view;
  }

  void refresh() const
  {
    std::cout << "TextView: " << m_document.getText() << std::endl;
  }
private:
    // private constructor to force use of static factory function
    TextView(Document &doc): m_document(doc)
    {}

    Document&               m_document;
};

class HexView
{
public:
  // static factory function that sets up automatic connection tracking
  static boost::shared_ptr<HexView> create(Document& doc)
  {
    boost::shared_ptr<HexView> new_view(new HexView(doc));
    {
      typedef Document::signal_t::slot_type slot_type;
      slot_type myslot(&HexView::refresh, new_view.get());
      doc.connect(myslot.track(new_view));
    }
    return new_view;
  }

  void refresh() const
  {
    const std::string&  s = m_document.getText();

    std::cout << "HexView:";

    for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
      std::cout << ' ' << std::hex << static_cast<int>(*it);

    std::cout << std::endl;
  }
private:
  // private constructor to force use of static factory function
  HexView(Document& doc): m_document(doc)
  {}

  Document&               m_document;
};

int main(int argc, char* argv[])
{
  Document    doc;
  boost::shared_ptr<TextView> v1 = TextView::create(doc);
  boost::shared_ptr<HexView> v2 = HexView::create(doc);

  doc.append(argc >= 2 ? argv[1] : "Hello world!");

  v2.reset(); // destroy the HexView, automatically disconnecting
  doc.append("  HexView should no longer be connected.");

  return 0;
}
