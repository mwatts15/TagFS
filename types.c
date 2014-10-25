#include "types.h"

const char *values[] =
{
    "",
    "",
    "0",
    "",
    "",
    "error"
};

const char *default_value(int type)
{
    return values[type];
}
