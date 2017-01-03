#pragma once

#define FUSE_USE_VERSION 29
#include <fuse.h>

void g2f_clear_ops(fuse_operations* ops);
void g2f_init_ops(fuse_operations* ops);
