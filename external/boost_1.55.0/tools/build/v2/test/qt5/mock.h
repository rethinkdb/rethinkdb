// (c) Copyright Juergen Hunold 2012
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <QtCore/QObject>

class Mock : public QObject
{
    /*!
      Test that the moc gets the necessary #defines
      Else the moc will not see the Q_OBJECT macro, issue a warning
      and linking will fail due to missing vtable symbols.
     */
#if defined(TEST_MOCK)
    Q_OBJECT
#endif
    public:

    Mock();
};
