#include "tagdb.h"

q%rename 2 I%
{
    Tag *t = lookup_tag(db, argv[0]);
    set_tag_name(t, argv[1], db);
    qr%TO_P(TRUE)%
}


