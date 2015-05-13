/* messages.cpp: Functions for end-of-game messages.
 *
 * Copyright (C) 2014 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include "messages.h"

#include "fileio.h"
#include "res.h"

#include <algorithm>
#include <bitset>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using std::bitset;
using std::ifstream;
using std::find;
using std::getline;
using std::istringstream;
using std::strcpy;
using std::string;
using std::vector;

namespace
{
    vector<string> messages;
    vector<size_t> typeindex[MessageTypeCount];
    size_t current[MessageTypeCount];
}

int const maxMessageSize = 511;
char const * messageTypeNames[MessageTypeCount] = { "win", "die", "time" };

int loadmessagesfromfile(char const *filename)
{
    char *fname = getpathforfileindir(resdir, filename);
    ifstream in(fname);
    free(fname);

    if (!in)
        return FALSE;

    vector<string> newmessages;
    vector<size_t> newtypeindex[MessageTypeCount];
    bitset<MessageTypeCount> isactive;
    isactive.set(MessageDie);
    string line;
    while (getline(in, line))
    {
        // Just in case DOS line endings on Linux. Not sure if needed.
        string::iterator rpos(find(line.begin(), line.end(), '\r'));
        if (rpos != line.end())
            line.erase(rpos);

        if (line.empty()) continue;
        if (line[0] == ':')
	{
            isactive.reset();

            istringstream in(line);
            in.get(); // Discard ':'
            string type;
            while (in >> type)
            {
                int typenum = find(messageTypeNames,
                    messageTypeNames + MessageTypeCount, type)
                    - messageTypeNames;
                if (typenum < MessageTypeCount)
                    isactive.set(typenum);
            }
        }
        else
	{
            for (size_t i = 0; i < isactive.size(); ++i)
            {
                if (isactive[i])
                    newtypeindex[i].push_back(newmessages.size());
            }
            line = line.substr(0, maxMessageSize+1);
            newmessages.push_back(line);
	}
    }

    messages.swap(newmessages);
    for (size_t i = 0; i < MessageTypeCount; ++i)
    {
        typeindex[i].swap(newtypeindex[i]);
        current[i] = 0;
    }

    return TRUE;
}

char const *getmessage(int type)
{
    static char buf[maxMessageSize+1];

    if ((type < 0) || (type >= MessageTypeCount) 
        || typeindex[type].size() == 0)
        return NULL;

    size_t const mnum = typeindex[type][current[type]];
    char const *s = messages[mnum].c_str();
    current[type] = (current[type] + 1) % typeindex[type].size();

    strcpy(buf, s);
    return buf;
}
