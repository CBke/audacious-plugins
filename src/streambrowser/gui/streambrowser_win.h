
#ifndef STREAMBROWSER_WIN_H
#define STREAMBROWSER_WIN_H

#include "../streamdir.h"


void			streambrowser_win_init();
void			streambrowser_win_done();
void			streambrowser_win_show();
void			streambrowser_win_hide();

void			streambrowser_win_set_streamdir(streamdir_t *streamdir, gchar *icon_filename);
void			streambrowser_win_set_category(streamdir_t *streamdir, category_t *category);
void			streambrowser_win_set_update_function(void (* update_function) (streamdir_t *streamdir, category_t *category, streaminfo_t *streaminfo));


#endif	// STREAMBROWSER_WIN_H

