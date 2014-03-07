/*
 *
 * Copyright (c) 2003 Dr John Maddock
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 *  boostinspect:noascii
 */

#include "licence_info.hpp"


std::pair<const license_info*, int> get_licenses()
{
   static const char* generic_author_sig = 
      "(?:"
         "(?:"
            "Copyright|\\(c\\)|\xA9"
         ")[[:blank:]]+"
      "){1,2}"
      "(?:"
         "\\d[^[:alpha:]]+"
            "([[:alpha:]]"
               "(?:"
               "(?!Use\\b|Permission\\b|All\\b|<P|(?:-\\s*)\\w+(?:://|@)|\\\\"
               ")[^\\n\\d]"
            ")+"
         ")"
         "|"
            "([[:alpha:]][^\\n\\d]+"
               "(?:\\n[^\\n\\d]+"
            ")??"
         ")(?:19|20)\\d{2}"
      ")"
      "|"
      "Authors:[[:blank:]]+"
         "([[:alpha:]][^\\n\\d]+"
      "|"
      "((?:The|This) code is considered to be in the public domain)"
      ")";

   static const char* generic_author_format = 
      "(?1$1)(?2$2)(?3$3)(?4Public Domain)";

   static const license_info licenses[] = 
   {
      license_info( boost::regex("distributed\\W+under"
         "(\\W+the)?[^\"[:word:]]+Boost\\W+Software\\W+License\\W+Version\\W+1.0", boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Boost Software License, Version 1.0"
         ,
         "<P>Copyright (c) <I>Date</I> <I>Author</I></P>"
         "<P>Distributed under the "
         "Boost Software License, Version 1.0. (See accompanying file "
         "LICENSE_1_0.txt or copy at <a href=\"http://www.boost.org/LICENSE_1_0.txt\">http://www.boost.org/LICENSE_1_0.txt)</a></P>"
       )
      ,
      license_info( boost::regex("Use\\W+\\modification\\W+and\\W+distribution(\\W+is|\\W+are)\\W+subject\\W+to"
         "(\\W+the)?[^\"[:word:]]+Boost\\W+Software\\W+License\\W+Version\\W+1.0", boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Boost Software License, Version 1.0 (variant #1)"
         ,
         "<P>Copyright (c) <I>Date</I> <I>Author</I></P>"
         "<P>Use, modification and distribution is subject to the "
         "Boost Software License, Version 1.0. (See accompanying file "
         "LICENSE_1_0.txt or copy at <a href=\"http://www.boost.org/LICENSE_1_0.txt\">http://www.boost.org/LICENSE_1_0.txt)</a></P>"
       )
      ,
      license_info( boost::regex("(?!is)\\w\\w\\W+subject\\W+to"
         "(\\W+the)?[^\"[:word:]]+Boost\\W+Software\\W+License\\W+Version\\W+1.0", boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Boost Software License, Version 1.0 (variant #2)"
         ,
         "<P>Copyright (c) <I>Date</I> <I>Author</I></P>"
         "<P>Subject to the "
         "Boost Software License, Version 1.0. (See accompanying file "
         "LICENSE_1_0.txt or copy at <a href=\"http://www.boost.org/LICENSE_1_0.txt\">http://www.boost.org/LICENSE_1_0.txt)</a></P>"
       )
      ,
      license_info( boost::regex("Copyright\\W+(c)\\W+2001\\W+2002\\W+Python\\W+Software\\W+Foundation\\W+All\\W+Rights\\W+Reserved", boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Python Software License"
         ,
         "<p>Copyright (c) 2001, 2002 Python Software Foundation;</p>"
         "<P>All Rights Reserved</P>"
       )
      ,
      license_info( boost::regex("Permission\\W+to\\W+use\\W+copy\\W+modify\\W+distribute\\W+and\\W+sell\\W+this\\W+software\\W+and\\W+its\\W+documentation"
         "\\W+for\\W+any\\W+purpose\\W+is\\W+hereby\\W+granted\\W+without\\W+fee"
         "\\W+provided\\W+that\\W+the\\W+above\\W+copyright\\W+notice\\W+appears?\\W+in\\W+all\\W+copies\\W+and"
         "\\W+that\\W+both\\W+(the|that)\\W+copyright\\W+notice\\W+and\\W+this\\W+permission\\W+notice\\W+appears?"
         "\\W+in\\W+supporting\\W+documentation[^<>]{1, 100}\\W+no\\W+representations"
         "\\W+(are\\W+made\\W+)?about\\W+the\\W+suitability\\W+of\\W+this\\W+software\\W+for\\W+any\\W+purpose"
         "\\W+It\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty"
         , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "SGI Style License"
         ,
         "<P>Copyright (c) <I>Date</I><BR>"
         "<I>Author</I><BR>"
         "<BR>"
         "Permission to use, copy, modify, distribute and sell this software "
         "and its documentation for any purpose is hereby granted without fee, "
         "provided that the above copyright notice appear in all copies and "
         "that both that copyright notice and this permission notice appear "
         "in supporting documentation.  <I>Author</I> makes no representations "
         "about the suitability of this software for any purpose. "
         "It is provided \"as is\" without express or implied warranty.</P>"
       )
      ,
      license_info( boost::regex("Permission\\W+to\\W+use\\W+copy\\W+modify\\W+distribute\\W+and\\W+sell\\W+this\\W+software"
         "\\W+for\\W+any\\W+purpose\\W+is\\W+hereby\\W+granted\\W+without\\W+fee"
         "\\W+provided\\W+that\\W+the\\W+above\\W+copyright\\W+notice\\W+appears?\\W+in\\W+all\\W+copies\\W+and"
         "\\W+that\\W+both\\W+(the|that)\\W+copyright\\W+notice\\W+and\\W+this\\W+permission\\W+notice\\W+appears?"
         "\\W+in\\W+supporting\\W+documentation[^<>]{1, 100}\\W+no\\W+representations"
         "\\W+(are\\W+made\\W+)?about\\W+the\\W+suitability\\W+of\\W+this\\W+software\\W+for\\W+any\\W+purpose"
         "\\W+It\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express(ed)?\\W+or\\W+implied\\W+warranty", boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #1"
         ,
         "<P>Copyright (c) <I>Date</I><BR>"
         "<I>Author</I><BR>"
         "<BR>"
         "Permission to use, copy, modify, distribute and sell this software "
         "for any purpose is hereby granted without fee, "
         "provided that the above copyright notice appear in all copies and "
         "that both that copyright notice and this permission notice appears? "
         "in supporting documentation.  <I>Author</I> makes no representations "
         "about the suitability of this software for any purpose. "
         "It is provided \"as is\" without express or implied warranty.</P>"
       )
      ,
      license_info(
         boost::regex(
            "Permission\\W+to\\W+copy\\W+use\\W+modify\\W+sell\\W+and\\W+distribute\\W+this\\W+software"
            "\\W+is\\W+granted\\W+provided\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies"
            "\\W+This\\W+software\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied"
            "\\W+warranty\\W+and\\W+with\\W+no\\W+claim\\W+as\\W+to\\W+its\\W+suitability\\W+for\\W+any\\W+purpose"
            , boost::regex::perl | boost::regex::icase
         )
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #2"
         ,
         "<P>Copyright (c) <I>Date</I> <I>Author</I>.<BR><BR>\n"
         "Permission to copy, use, modify, sell and distribute this software<BR>\n"
         "is granted provided this copyright notice appears in all copies.<BR>\n"
         "This software is provided \"as is\" without express or implied<BR>\n"
         "warranty, and with no claim as to its suitability for any purpose.</P>\n"
       )
      ,
      license_info(
         boost::regex(
            "Permission\\W+to\\W+copy\\W+use[^\"[:word:]]+modify\\W+sell\\W+and\\W+distribute\\W+this\\W+software\\W+is\\W+granted\\W+provided"
            "\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies\\W+This\\W+software\\W+is"
            "\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty\\W+and\\W+with"
            "\\W+no\\W+claim\\W+at\\W+to\\W+its\\W+suitability\\W+for\\W+any\\W+purpose"
            , boost::regex::perl | boost::regex::icase
         )
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #3"
         ,
         "<P>(C) Copyright <I>Author</I> <I>Date</I>.  Permission to copy, use, "
         "modify, sell, and distribute this software is granted provided "
         "this copyright notice appears in all copies.  This software is "
         "provided \"as is\" without express or implied warranty, and with "
         "no claim at to its suitability for any purpose.</p>\n"
       )
      ,
      license_info( boost::regex("Permission\\W+to\\W+copy\\W+use\\W+sell\\W+and\\W+distribute\\W+this\\W+software\\W+is\\W+granted"
                     "\\W+provided\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies"
                     "\\W+Permission\\W+to\\W+modify\\W+the\\W+code\\W+and\\W+to\\W+distribute\\W+modified\\W+code\\W+is\\W+granted"
                     "\\W+provided\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies\\W+and\\W+a\\W+notice"
                     "\\W+that\\W+the\\W+code\\W+was\\W+modified\\W+is\\W+included\\W+with\\W+the\\W+copyright\\W+notice"
                     "\\W+This\\W+software\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty\\W+and\\W+with\\W+no\\W+claim\\W+as\\W+to\\W+its\\W+suitability\\W+for\\W+any\\W+purpose"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #4"
         ,
         "<P>Copyright (C) <I>Date Author</I><BR>"
         "<BR>"
         "Permission to copy, use, sell and distribute this software is granted\n"
         "provided this copyright notice appears in all copies.\n"
         "Permission to modify the code and to distribute modified code is granted\n"
         "provided this copyright notice appears in all copies, and a notice\n"
         "that the code was modified is included with the copyright notice.</P>\n"
         "<P>This software is provided \"as is\" without express or implied warranty,\n"
         "and with no claim as to its suitability for any purpose.</P>"
       )
      ,
      license_info( boost::regex("This\\W+file\\W+is\\W+part\\W+of\\W+the\\W+(Boost\\W+Graph|Generic\\W+Graph\\W+Component)\\W+Library"
                     "\\W+You\\W+should\\W+have\\W+received\\W+a\\W+copy\\W+of\\W+the\\W+License\\W+Agreement\\W+for\\W+the"
                     "\\W+(Boost|Generic)\\W+Graph\\W+(Component\\W+)?Library\\W+along\\W+with\\W+the\\W+software;\\W+see\\W+the\\W+file\\W+LICENSE"
                     "(\\W+If\\W+not\\W+contact\\W+Office\\W+of\\W+Research\\W+University\\W+of\\W+Notre\\W+Dame\\W+Notre"
                     "\\W+Dame\\W+IN\\W+46556)?"
                     "\\W+Permission\\W+to\\W+modify\\W+the\\W+code\\W+and\\W+to\\W+distribute(\\W+modified|\\W+the)\\W+code\\W+is"
                     "\\W+granted\\W+provided\\W+the\\W+text\\W+of\\W+this\\W+NOTICE\\W+is\\W+retained\\W+a\\W+notice\\W+(that|if)"
                     "\\W+the\\W+code\\W+was\\W+modified\\W+is\\W+included\\W+with\\W+the\\W+above\\W+COPYRIGHT\\W+NOTICE\\W+and"
                     "\\W+with\\W+the\\W+COPYRIGHT\\W+NOTICE\\W+in\\W+the\\W+LICENSE\\W+file\\W+and\\W+that\\W+the\\W+LICENSE"
                     "\\W+file\\W+is\\W+distributed\\W+with\\W+the\\W+modified\\W+code\\W+"
                     "\\W+LICENSOR\\W+MAKES\\W+NO\\W+REPRESENTATIONS\\W+OR\\W+WARRANTIES\\W+EXPRESS\\W+OR\\W+IMPLIED"
                     "\\W+By\\W+way\\W+of\\W+example\\W+but\\W+not\\W+limitation\\W+Licensor\\W+MAKES\\W+NO"
                     "\\W+REPRESENTATIONS\\W+OR\\W+WARRANTIES\\W+OF\\W+MERCHANTABILITY\\W+OR\\W+FITNESS\\W+FOR\\W+ANY"
                     "\\W+PARTICULAR\\W+PURPOSE\\W+OR\\W+THAT\\W+THE\\W+USE\\W+OF\\W+THE\\W+LICENSED\\W+SOFTWARE\\W+COMPONENTS"
                     "\\W+OR\\W+DOCUMENTATION\\W+WILL\\W+NOT\\W+INFRINGE\\W+ANY\\W+PATENTS\\W+COPYRIGHTS\\W+TRADEMARKS"
                     "\\W+OR\\W+OTHER\\W+RIGHTS"
            , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Boost.Graph license (Notre Dame)"
         ,
         "<P>Copyright <I>Date</I> University of Notre Dame.<BR>"
         "Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek</P>"
         "<P>This file is part of the Boost Graph Library</P>"
         "<P>You should have received a copy of the <A href=\"http://www.boost.org/libs/graph/LICENCE\">License Agreement</a> for the "
         "Boost Graph Library along with the software; see the file <A href=\"http://www.boost.org/libs/graph/LICENCE\">LICENSE</a>. "
         "If not, contact Office of Research, University of Notre Dame, Notre "
         "Dame, IN 46556.</P>"
         "<P>Permission to modify the code and to distribute modified code is "
         "granted, provided the text of this NOTICE is retained, a notice that "
         "the code was modified is included with the above COPYRIGHT NOTICE and "
         "with the COPYRIGHT NOTICE in the <A href=\"http://www.boost.org/libs/graph/LICENCE\">LICENSE</a> file, and that the <A href=\"http://www.boost.org/libs/graph/LICENCE\">LICENSE</a> "
         "file is distributed with the modified code.</P>"
         "<P>LICENSOR MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.<BR> "
         "By way of example, but not limitation, Licensor MAKES NO "
         "REPRESENTATIONS OR WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY "
         "PARTICULAR PURPOSE OR THAT THE USE OF THE LICENSED SOFTWARE COMPONENTS "
         "OR DOCUMENTATION WILL NOT INFRINGE ANY PATENTS, COPYRIGHTS, TRADEMARKS "
         "OR OTHER RIGHTS.</P>"
       )
      ,
      license_info( boost::regex("This\\W+file\\W+is\\W+part\\W+of\\W+the\\W+(Boost\\W+Graph|Generic\\W+Graph\\W+Component)\\W+Library"
                     "\\W+You\\W+should\\W+have\\W+received\\W+a\\W+copy\\W+of\\W+the\\W+License\\W+Agreement\\W+for\\W+the"
                     "\\W+(Boost|Generic)\\W+Graph\\W+(Component\\W+)?Library\\W+along\\W+with\\W+the\\W+software;\\W+see\\W+the\\W+file\\W+LICENSE"
                     "(\\W+If\\W+not\\W+contact\\W+Office\\W+of\\W+Research\\W+Indiana\\W+University\\W+Bloomington\\W+IN\\W+47405)?"
                     "\\W+Permission\\W+to\\W+modify\\W+the\\W+code\\W+and\\W+to\\W+distribute(\\W+modified|\\W+the)\\W+code\\W+is"
                     "\\W+granted\\W+provided\\W+the\\W+text\\W+of\\W+this\\W+NOTICE\\W+is\\W+retained\\W+a\\W+notice\\W+(that|if)"
                     "\\W+the\\W+code\\W+was\\W+modified\\W+is\\W+included\\W+with\\W+the\\W+above\\W+COPYRIGHT\\W+NOTICE\\W+and"
                     "\\W+with\\W+the\\W+COPYRIGHT\\W+NOTICE\\W+in\\W+the\\W+LICENSE\\W+file\\W+and\\W+that\\W+the\\W+LICENSE"
                     "\\W+file\\W+is\\W+distributed\\W+with\\W+the\\W+modified\\W+code\\W+"
                     "\\W+LICENSOR\\W+MAKES\\W+NO\\W+REPRESENTATIONS\\W+OR\\W+WARRANTIES\\W+EXPRESS\\W+OR\\W+IMPLIED"
                     "\\W+By\\W+way\\W+of\\W+example\\W+but\\W+not\\W+limitation\\W+Licensor\\W+MAKES\\W+NO"
                     "\\W+REPRESENTATIONS\\W+OR\\W+WARRANTIES\\W+OF\\W+MERCHANTABILITY\\W+OR\\W+FITNESS\\W+FOR\\W+ANY"
                     "\\W+PARTICULAR\\W+PURPOSE\\W+OR\\W+THAT\\W+THE\\W+USE\\W+OF\\W+THE\\W+LICENSED\\W+SOFTWARE\\W+COMPONENTS"
                     "\\W+OR\\W+DOCUMENTATION\\W+WILL\\W+NOT\\W+INFRINGE\\W+ANY\\W+PATENTS\\W+COPYRIGHTS\\W+TRADEMARKS"
                     "\\W+OR\\W+OTHER\\W+RIGHTS"
            , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Boost.Graph license (Indiana University)"
         ,
         "<P>Copyright <I>Date</I> Indiana University.<BR>"
         "Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek</P>"
         "<P>This file is part of the Boost Graph Library</P>"
         "<P>You should have received a copy of the <A href=\"http://www.boost.org/libs/graph/LICENCE\">License Agreement</a> for the "
         "Boost Graph Library along with the software; see the file <A href=\"http://www.boost.org/libs/graph/LICENCE\">LICENSE</a>. "
         "If not, contact Office of Research, Indiana University, Bloomington,"
         "IN 47404.</P>"
         "<P>Permission to modify the code and to distribute modified code is "
         "granted, provided the text of this NOTICE is retained, a notice that "
         "the code was modified is included with the above COPYRIGHT NOTICE and "
         "with the COPYRIGHT NOTICE in the <A href=\"http://www.boost.org/libs/graph/LICENCE\">LICENSE</a> file, and that the <A href=\"http://www.boost.org/libs/graph/LICENCE\">LICENSE</a> "
         "file is distributed with the modified code.</P>"
         "<P>LICENSOR MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.<BR> "
         "By way of example, but not limitation, Licensor MAKES NO "
         "REPRESENTATIONS OR WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY "
         "PARTICULAR PURPOSE OR THAT THE USE OF THE LICENSED SOFTWARE COMPONENTS "
         "OR DOCUMENTATION WILL NOT INFRINGE ANY PATENTS, COPYRIGHTS, TRADEMARKS "
         "OR OTHER RIGHTS.</P>"
       )
      ,
      license_info( boost::regex("Permission\\W+to\\W+copy\\W+use\\W+modify\\W+sell\\W+and\\W+distribute\\W+this\\W+software\\W+is"
                     "[^\"[:word:]]+granted\\W+provided\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies\\W+and"
                     "\\W+modified\\W+version\\W+are\\W+clearly\\W+marked\\W+as\\W+such\\W+This\\W+software\\W+is\\W+provided"
                     "\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty\\W+and\\W+with\\W+no\\W+claim\\W+as\\W+to\\W+its"
                     "\\W+suitability\\W+for\\W+any\\W+purpose"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #5"
         ,
         "<P>Copyright (C) <I>Date Author</I></P>"
         "<p>Permission to copy, use, modify, sell and distribute this software is "
         "granted, provided this copyright notice appears in all copies and "
         "modified version are clearly marked as such. This software is provided "
         "\"as is\" without express or implied warranty, and with no claim as to its "
         "suitability for any purpose.</P>"
       )
      ,
      license_info( boost::regex("This\\W+file\\W+can\\W+be\\W+redistributed\\W+and\\W+or\\W+modified\\W+under\\W+the\\W+terms\\W+found"
                     "\\W+in\\W+copyright\\W+html"
                     "\\W+This\\W+software\\W+and\\W+its\\W+documentation\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or"
                     "\\W+implied\\W+warranty\\W+and\\W+with\\W+no\\W+claim\\W+as\\W+to\\W+its\\W+suitability\\W+for\\W+any\\W+purpose"
            , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Boost.Pool license"
         ,
         "<P>This file can be redistributed and/or modified under the terms found "
         "in <a href=\"http://www.boost.org/libs/pool/doc/copyright.html\">copyright.html</a></P>\n"
         "<P>This software and its documentation is provided \"as is\" without express or "
         "implied warranty, and with no claim as to its suitability for any purpose</P>"
       )
      ,
      license_info(boost::regex("Permission\\W+to\\W+use\\W+copy\\W+modify\\W+sell\\W+and\\W+distribute\\W+this\\W+software"
                     "\\W+is\\W+hereby\\W+granted\\W+without\\W+fee\\W+provided\\W+that\\W+the\\W+above\\W+copyright\\W+notice"
                     "\\W+appears\\W+in\\W+all\\W+copies\\W+and\\W+that\\W+both\\W+that\\W+copyright\\W+notice\\W+and\\W+this"
                     "\\W+permission\\W+notice\\W+appear\\W+in\\W+supporting\\W+documentation"
                     "[^<>]{1,100}\\W+(make\\W+any\\W+representation|makes\\W+no\\W+representations)\\W+about\\W+the\\W+suitability\\W+of\\W+this"
                     "\\W+software\\W+for\\W+any\\W+purpose\\W+It\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or"
                     "\\W+implied\\W+warranty"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #6"
         ,
         "<P>Copyright <I>Author Data</I></P>"
         "<P>Permission to use, copy, modify, sell, and distribute this software "
         "is hereby granted without fee provided that the above copyright notice "
         "appears in all copies and that both that copyright notice and this "
         "permission notice appear in supporting documentation, "
         "<I>Author</I> makes no representations about the suitability of this "
         "software for any purpose. It is provided \"as is\" without express or "
         "implied warranty.</P>"
       )
      ,
      license_info( boost::regex("Permission\\W+to\\W+copy"
                     "[^\"[:word:]]+use\\W+modify\\W+sell\\W+and\\W+distribute\\W+this\\W+software\\W+is\\W+granted\\W+provided"
                     "\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies\\W+of\\W+the\\W+source\\W+This"
                     "\\W+software\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty"
                     "\\W+and\\W+with\\W+no\\W+claim\\W+as\\W+to\\W+its\\W+suitability\\W+for\\W+any\\W+purpose"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #7"
         ,
         "<P>Copyright <I>Author Date</I>. Permission to copy, "
         "use, modify, sell and distribute this software is granted provided "
         "this copyright notice appears in all copies of the source. This "
         "software is provided \"as is\" without express or implied warranty, "
         "and with no claim as to its suitability for any purpose."
       )
      ,
      license_info(boost::regex("This\\W+software\\W+is\\W+provided\\W+as-is\\W+without\\W+any\\W+express\\W+or\\W+implied"
      "\\W+warranty\\W+In\\W+no\\W+event\\W+will\\W+the\\W+copyright\\W+holder\\W+be\\W+held\\W+liable\\W+for"
      "\\W+any\\W+damages\\W+arising\\W+from\\W+the\\W+use\\W+of\\W+this\\W+software"
      "\\W+Permission\\W+is\\W+granted\\W+to\\W+anyone\\W+to\\W+use\\W+this\\W+software\\W+for\\W+any\\W+purpose"
      "\\W+including\\W+commercial\\W+applications\\W+and\\W+to\\W+alter\\W+it\\W+and\\W+redistribute"
      "\\W+it\\W+freely\\W+subject\\W+to\\W+the\\W+following\\W+restrictions:"
      "\\W+1\\W+The\\W+origin\\W+of\\W+this\\W+software\\W+must\\W+not\\W+be\\W+misrepresented;\\W+you\\W+must"
      "\\W+not\\W+claim\\W+that\\W+you\\W+wrote\\W+the\\W+original\\W+software\\W+If\\W+you\\W+use\\W+this"
      "\\W+software\\W+in\\W+a\\W+product\\W+an\\W+acknowledgment\\W+in\\W+the\\W+product\\W+documentation"
      "\\W+would\\W+be\\W+appreciated\\W+but\\W+is\\W+not\\W+required"
      "\\W+2\\W+Altered\\W+source\\W+versions\\W+must\\W+be\\W+plainly\\W+marked\\W+as\\W+such\\W+and\\W+must"
      "\\W+not\\W+be\\W+misrepresented\\W+as\\W+being\\W+the\\W+original\\W+software"
      "\\W+3\\W+This\\W+notice\\W+may\\W+not\\W+be\\W+removed\\W+or\\W+altered\\W+from\\W+any\\W+source"
      "\\W+distribution"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #8"
         ,
         "<P>Phoenix V0.9<BR>Copyright (c) <I>Date</I> Joel de Guzman</P>"
         "<P>This software is provided 'as-is', without any express or implied "
         "warranty. In no event will the copyright holder be held liable for "
         "any damages arising from the use of this software.</P>"
         "<P>Permission is granted to anyone to use this software for any purpose, "
         "including commercial applications, and to alter it and redistribute "
         "it freely, subject to the following restrictions:</P>"
         "<P>1.  The origin of this software must not be misrepresented; you must "
         "not claim that you wrote the original software. If you use this "
         "software in a product, an acknowledgment in the product documentation "
         "would be appreciated but is not required.</P>"
         "2.  Altered source versions must be plainly marked as such, and must "
         "not be misrepresented as being the original software. </P>"
         "<P>3.  This notice may not be removed or altered from any source "
         "distribution. "
       )
      ,
      license_info( boost::regex("Permission\\W+to\\W+use\\W+copy\\W+modify\\W+sell\\W+and\\W+distribute\\W+this\\W+software"
                     "\\W+is\\W+hereby\\W+granted\\W+without\\W+fee\\W+provided\\W+that\\W+the\\W+above\\W+copyright\\W+notice"
                     "\\W+appears\\W+in\\W+all\\W+copies\\W+and\\W+that\\W+both\\W+that\\W+copyright\\W+notice\\W+and\\W+this"
                     "\\W+permission\\W+notice\\W+appear\\W+in\\W+supporting\\W+documentation"
                     "\\W+None\\W+of\\W+the\\W+above\\W+authors\\W+nor.{1,100}make\\W+any"
                     "\\W+representation\\W+about\\W+the\\W+suitability\\W+of\\W+this\\W+software\\W+for\\W+any"
                     "\\W+purpose\\W+It\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #9"
         ,
         "<P>Copyright <I> Author Date</I><BR>"
         "Permission to use, copy, modify, sell, and distribute this software "
         "is hereby granted without fee provided that the above copyright notice "
         "appears in all copies and that both that copyright notice and this "
         "permission notice appear in supporting documentation, <BR>"
         "None of the above authors nor <I>Author's Organisation</I> make any "
         "representation about the suitability of this software for any "
         "purpose. It is provided \"as is\" without express or implied warranty."
       )
      ,
      license_info( boost::regex("Permission\\W+to\\W+use\\W+copy\\W+modify\\W+and\\W+distribute\\W+this\\W+software\\W+for\\W+any"
                     "\\W+purpose\\W+is\\W+hereby\\W+granted\\W+without\\W+fee\\W+provided\\W+that\\W+this\\W+copyright\\W+and"
                     "\\W+permissions\\W+notice\\W+appear\\W+in\\W+all\\W+copies\\W+and\\W+derivatives"
                     "\\W+This\\W+software\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #10"
         ,
         "<P>Copyright <I>Author Date</I>. All rights reserved.</P>"
         "<P>Permission to use, copy, modify, and distribute this software for any "
         "purpose is hereby granted without fee, provided that this copyright and "
         "permissions notice appear in all copies and derivatives.</P>"
         "<P>This software is provided \"as is\" without express or implied warranty.</P>"
       )
      ,
      license_info( boost::regex("This\\W+material\\W+is\\W+provided\\W+as\\W+is\\W+with\\W+absolutely\\W+no\\W+warranty\\W+expressed"
                     "\\W+or\\W+implied\\W+Any\\W+use\\W+is\\W+at\\W+your\\W+own\\W+risk"
                     "\\W+Permission\\W+to\\W+use\\W+or\\W+copy\\W+this\\W+software\\W+for\\W+any\\W+purpose\\W+is\\W+hereby\\W+granted"
                     "\\W+without\\W+fee\\W+provided\\W+the\\W+above\\W+notices\\W+are\\W+retained\\W+on\\W+all\\W+copies"
                     "\\W+Permission\\W+to\\W+modify\\W+the\\W+code\\W+and\\W+to\\W+distribute\\W+modified\\W+code\\W+is\\W+granted"
                     "\\W+provided\\W+the\\W+above\\W+notices\\W+are\\W+retained\\W+and\\W+a\\W+notice\\W+that\\W+the\\W+code\\W+was"
                     "\\W+modified\\W+is\\W+included\\W+with\\W+the\\W+above\\W+copyright\\W+notice"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #11"
         ,
         "<P>This material is provided \"as is\", with absolutely no warranty expressed "
         "or implied. Any use is at your own risk.</P>"
         "<P>Permission to use or copy this software for any purpose is hereby granted "
         "without fee, provided the above notices are retained on all copies. "
         "Permission to modify the code and to distribute modified code is granted, "
         "provided the above notices are retained, and a notice that the code was "
         "modified is included with the above copyright notice.</P>"
       )
      ,
      license_info( boost::regex("Permission\\W+to\\W+copy\\W+use\\W+and\\W+distribute\\W+this\\W+software\\W+is\\W+granted\\W+provided"
                     "\\W+that\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies"
                     "\\W+Permission\\W+to\\W+modify\\W+the\\W+code\\W+and\\W+to\\W+distribute\\W+modified\\W+code\\W+is\\W+granted"
                     "\\W+provided\\W+that\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies\\W+and\\W+a\\W+notice"
                     "\\W+that\\W+the\\W+code\\W+was\\W+modified\\W+is\\W+included\\W+with\\W+the\\W+copyright\\W+notice"
                     "\\W+This\\W+software\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty\\W+and"
                     "\\W+with\\W+no\\W+claim\\W+as\\W+to\\W+its\\W+suitability\\W+for\\W+any\\W+purpose"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #12"
         ,
         "<P>Copyright (C) <I>Date Author</I></P><P>Permission to copy, use, and distribute this software is granted, provided "
         "that this copyright notice appears in all copies.<BR>"
         "Permission to modify the code and to distribute modified code is granted, "
         "provided that this copyright notice appears in all copies, and a notice "
         "that the code was modified is included with the copyright notice.</P>"
         "<P>This software is provided \"as is\" without express or implied warranty, and "
         "with no claim as to its suitability for any purpose.</P>"
       )
      ,
      license_info( boost::regex("Permission\\W+to\\W+copy\\W+and\\W+use\\W+this\\W+software\\W+is\\W+granted"
                                 "\\W+provided\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies"
                                 "\\W+Permission\\W+to\\W+modify\\W+the\\W+code\\W+and\\W+to\\W+distribute\\W+modified\\W+code\\W+is\\W+granted"
                                 "\\W+provided\\W+this\\W+copyright\\W+notice\\W+appears\\W+in\\W+all\\W+copies\\W+and\\W+a\\W+notice"
                                 "\\W+that\\W+the\\W+code\\W+was\\W+modified\\W+is\\W+included\\W+with\\W+the\\W+copyright\\W+notice"
                                 "\\W+This\\W+software\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty"
                                 "\\W+and\\W+with\\W+no\\W+claim\\W+as\\W+to\\W+its\\W+suitability\\W+for\\W+any\\W+purpose"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #13"
         ,
         "<P>Copyright (C) <I>Date Author</I></P>"
         "<P>Permission to copy and use this software is granted, "
         "provided this copyright notice appears in all copies. "
         "Permission to modify the code and to distribute modified code is granted, "
         "provided this copyright notice appears in all copies, and a notice "
         "that the code was modified is included with the copyright notice.</P>"
         "<P>This software is provided \"as is\" without express or implied warranty, "
         "and with no claim as to its suitability for any purpose.</P>"
       )
      ,
      license_info( boost::regex("Copyright\\W+Kevlin\\W+Henney\\W+2000\\W+All\\W+rights\\W+reserved\\W+"
                                 "Permission\\W+to\\W+use\\W+copy\\W+modify\\W+and\\W+distribute\\W+this\\W+software\\W+for\\W+any"
                                 "\\W+purpose\\W+is\\W+hereby\\W+granted\\W+without\\W+fee\\W+provided\\W+that\\W+this\\W+copyright\\W+and"
                                 "\\W+permissions\\W+notice\\W+appear\\W+in\\W+all\\W+copies\\W+and\\W+derivatives\\W+and\\W+that\\W+no"
                                 "\\W+charge\\W+may\\W+be\\W+made\\W+for\\W+the\\W+software\\W+and\\W+its\\W+documentation\\W+except\\W+to\\W+cover"
                                 "\\W+cost\\W+of\\W+distribution"
                                 "\\W+This\\W+software\\W+is\\W+provided\\W+as\\W+is\\W+without\\W+express\\W+or\\W+implied\\W+warranty\\W+"
                     , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Old style Boost license #14"
         ,
         "<P>Copyright The Author, The Date. All rights reserved.</P>"
         "<P>Permission to use, copy, modify, and distribute this software for any"
         " purpose is hereby granted without fee, provided that this copyright and"
         " permissions notice appear in all copies and derivatives, and that no"
         " charge may be made for the software and its documentation except to cover"
         " cost of distribution.</P>"
         "<P>This software is provided \"as is\" without express or implied warranty.</P>"
       )
      ,
      license_info( boost::regex("preprocessed\\W+version\\W+of\\W+boost/mpl/.*\\.hpp\\W+header\\W+see\\W+the\\W+original\\W+for\\W+copyright\\W+information", boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "SGI Style Licence (MPL preprocessed file)"
         ,
         "<P>Copyright (c) <I>Date</I><BR>"
         "<I>Author</I><BR>"
         "<BR>"
         "Permission to use, copy, modify, distribute and sell this software "
         "and its documentation for any purpose is hereby granted without fee, "
         "provided that the above copyright notice appear in all copies and "
         "that both that copyright notice and this permission notice appear "
         "in supporting documentation.  <I>Author</I> makes no representations "
         "about the suitability of this software for any purpose. "
         "It is provided \"as is\" without express or implied warranty.</P>"
       )
      ,
      license_info( boost::regex(
            "This\\W+file\\W+is\\W+part\\W+of\\W+jam\\W+"
            "License\\W+is\\W+hereby\\W+granted\\W+to\\W+use\\W+this\\W+software\\W+and\\W+distribute\\W+it\\W+"
            "freely\\W+as\\W+long\\W+as\\W+this\\W+copyright\\W+notice\\W+is\\W+retained\\W+and\\W+modifications\\W+"
            "are\\W+clearly\\W+marked\\W+"
            "ALL\\W+WARRANTIES\\W+ARE\\W+HEREBY\\W+DISCLAIMED"
            "|"
            "This\\W+file\\W+is\\W+part\\W+of\\W+Jam\\W+see\\W+jam\\.c\\W+for\\W+Copyright\\W+information"
            "|This file has been donated to Jam"
            "|Generated by mkjambase from Jambase" , boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig + std::string("|(Craig\\W+W\\W+McPheeters\\W+Alias\\W+Wavefront)|(Generated by mkjambase from Jambase)"), boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format + std::string("(?4Craig W. McPheeters, Alias|Wavefront)(?5Christopher Seiwald and Perforce Software, Inc)")
         ,
         "Perforce Jam License"
         ,
         "<P>Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.</P>"
         "<P>This file is part of jam.</P>"
         "<P>License is hereby granted to use this software and distribute it "
         "freely, as long as this copyright notice is retained and modifications "
         " are clearly marked.</P>"
         "<P>ALL WARRANTIES ARE HEREBY DISCLAIMED</P>"
       )
      ,
      license_info( boost::regex(
            "Permission\\W+is\\W+granted\\W+to\\W+anyone\\W+to\\W+use\\W+this\\W+software\\W+for\\W+any\\W+"
            "purpose\\W+on\\W+any\\W+computer\\W+system\\W+and\\W+to\\W+redistribute\\W+it\\W+freely\\W+"
            "subject\\W+to\\W+the\\W+following\\W+restrictions\\W+"
            "1\\W+The\\W+author\\W+is\\W+not\\W+responsible\\W+for\\W+the\\W+consequences\\W+of\\W+use\\W+of\\W+"
            "this\\W+software\\W+no\\W+matter\\W+how\\W+awful\\W+even\\W+if\\W+they\\W+arise\\W+"
            "from\\W+defects\\W+in\\W+it\\W+"
            "2\\W+The\\W+origin\\W+of\\W+this\\W+software\\W+must\\W+not\\W+be\\W+misrepresented\\W+either\\W+"
            "by\\W+explicit\\W+claim\\W+or\\W+by\\W+omission\\W+"
            "3\\W+Altered\\W+versions\\W+must\\W+be\\W+plainly\\W+marked\\W+as\\W+such\\W+and\\W+must\\W+not\\W+"
            "be\\W+misrepresented\\W+as\\W+being\\W+the\\W+original\\W+software"
            "|Definitions\\W+etc\\W+for\\W+regexp\\W+3\\W+routines", boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig + std::string("|(Definitions\\W+etc\\W+for\\W+regexp\\W+3\\W+routines)"), boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format + std::string("(?4University of Toronto)")
         ,
         "BSD Regex License"
         ,
         "<P>Copyright (c) 1986 by University of Toronto.</P>"
         "<P>Written by Henry Spencer.  Not derived from licensed software.</P>"
         "<P>Permission is granted to anyone to use this software for any"
         "purpose on any computer system, and to redistribute it freely,"
         "subject to the following restrictions:</P>"
         "<P>The author is not responsible for the consequences of use of"
         "this software, no matter how awful, even if they arise"
         "from defects in it.</P>"
         "<p>The origin of this software must not be misrepresented, either"
         "by explicit claim or by omission.</p>"
         "<p>Altered versions must be plainly marked as such, and must not"
         "be misrepresented as being the original software.</P>"
       )
      ,
      license_info( boost::regex(
            "Skeleton\\W+parser\\W+for\\W+Yacc\\W+like\\W+parsing\\W+with\\W+Bison\\W+"
            "Copyright.{0,100}Free\\W+Software\\W+Foundation\\W+Inc\\W+"
         "\\W+This\\W+program\\W+is\\W+free\\W+software\\W+you\\W+can\\W+redistribute\\W+it\\W+and\\W+or\\W+modify\\W+"
         "it\\W+under\\W+the\\W+terms\\W+of\\W+the\\W+GNU\\W+General\\W+Public\\W+License\\W+as\\W+published\\W+by\\W+"
         "the\\W+Free\\W+Software\\W+Foundation\\W+either\\W+version\\W+2\\W+or\\W+at\\W+your\\W+option\\W+"
         "any\\W+later\\W+version"
         "|"
         // this part matches the start of jamgramtab.h which is under the same licence
         // but bison does not output it's usual licence declaration:
         "\\{\\s*\"!\"\\s*,\\s*_BANG_t\\s*\\}", boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig + std::string("|(\\{\\s*\"!\"\\s*,\\s*_BANG_t\\s*\\})"), boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format + std::string("(?4Free Software Foundation, Inc)")
         ,
         "GNU Parser Licence"
         ,
         "<P>Skeleton parser for Yacc-like parsing with Bison,<BR>"
         "Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.</P>"
         "<P>This program is free software; you can redistribute it and/or modify"
         "it under the terms of the GNU General Public License as published by"
         "the Free Software Foundation; either version 2, or (at your option)"
         "any later version.</P>"
         "<P>This program is distributed in the hope that it will be useful,"
         "but WITHOUT ANY WARRANTY; without even the implied warranty of"
         "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"
         "GNU General Public License for more details.</P>"
         "<P>You should have received a copy of the GNU General Public License"
         "along with this program; if not, write to the Free Software"
         "Foundation, Inc., 59 Temple Place - Suite 330,"
         "Boston, MA 02111-1307, USA.</P>"
         "<P>As a special exception, when this file is copied by Bison into a"
         "Bison output file, you may use that output file without restriction."
         "This special exception was added by the Free Software Foundation"
         "in version 1.24 of Bison.</P>"
       )
      ,
      license_info( boost::regex(
            "(?:The|This)\\W+code\\W+is\\W+considered\\W+to\\W+be\\W+in\\W+the\\W+public\\W+domain", boost::regex::perl | boost::regex::icase)
         ,
         boost::regex(generic_author_sig, boost::regex::perl | boost::regex::icase)
         ,
         generic_author_format
         ,
         "Public Domain"
         ,
         "<P>The code has no license terms, it has been explicity placed in the\n"
         "public domain by it's author(s).</P>"
       )
      ,
   };
   return std::pair<const license_info*, int>(licenses, static_cast<int>(sizeof(licenses)/sizeof(licenses[0])));
}

std::string format_authors_name(const std::string& name)
{
   // put name into a consistent format, so that we don't get too much
   // of a proliferation of names (lots of versions of the same basic form).

   static const boost::regex e("(^)?[^-(<a-zA-ZÀ-þ]+(([(<].*)?$)?");
   static const char* formatter = "(?1:(?2: ))";

   return boost::regex_replace(name, e, formatter, boost::match_default | boost::format_all);
}

