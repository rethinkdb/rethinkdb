// Document/View sample for Boost.Signals
// Copyright Keith MacDonald 2005. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// For more information, see http://www.boost.org

#include <iostream>
#include <string>
#include <boost/signal.hpp>
#include <boost/bind.hpp>

class Document
{
public:
    typedef boost::signal<void (bool)>  signal_t;
    typedef boost::signals::connection  connection_t;

public:
    Document()
    {}

    connection_t connect(signal_t::slot_function_type subscriber)
    {
        return m_sig.connect(subscriber);
    }

    void disconnect(connection_t subscriber)
    {
        subscriber.disconnect();
    }

    void append(const char* s)
    {
        m_text += s;
        m_sig(true);
    }

    const std::string& getText() const
    {
        return m_text;
    }

private:
    signal_t    m_sig;
    std::string m_text;
};

class View
{
public:
    View(Document& m)
        : m_document(m)
    {
        m_connection = m_document.connect(boost::bind(&View::refresh, this, _1));
    }

    virtual ~View()
    {
        m_document.disconnect(m_connection);
    }

    virtual void refresh(bool bExtended) const = 0;

protected:
    Document&               m_document;

private:
    Document::connection_t  m_connection;
};

class TextView : public View
{
public:
    TextView(Document& doc)
        : View(doc)
    {}

    virtual void refresh(bool bExtended) const
    {
        std::cout << "TextView: " << m_document.getText() << std::endl;
    }
};

class HexView : public View
{
public:
    HexView(Document& doc)
        : View(doc)
    {}

    virtual void refresh(bool bExtended) const
    {
        const std::string&  s = m_document.getText();

        std::cout << "HexView:";

        for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
            std::cout << ' ' << std::hex << static_cast<int>(*it);

        std::cout << std::endl;
    }
};

int main(int argc, char* argv[])
{
    Document    doc;
    TextView    v1(doc);
    HexView     v2(doc);

    doc.append(argc == 2 ? argv[1] : "Hello world!");
    return 0;
}
