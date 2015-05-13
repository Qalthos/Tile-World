/* settings.h: Functions for managing settings.
 *
 * Copyright (C) 2014 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef HEADER_settings_h_
#define HEADER_settings_h_

#ifdef __cplusplus
extern "C" {
#endif

void loadsettings(void);

void savesettings(void);

int getintsetting(char const * name);
void setintsetting(char const * name, int val);

#ifdef __cplusplus
}
#endif

#endif
