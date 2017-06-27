#pragma once

#include <iostream>
#include <string>

//=================================================
/// Print an error message and exit
inline void fatal(const std::string & msg)
{
    using namespace std;
    cerr << msg << endl;
    exit(1);
}
//=================================================
/// Print a 2-part error message and exit
inline void fatal2(const std::string & msg1, const std::string & msg2)
{
    using namespace std;
    cerr << msg1 << " " << msg2 << endl;
    exit(1);
}
//=================================================
