// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_INT_H
#define JRTC_INT_H

char*
concat(const char* s1, const char* s2);

int
start_jrtc(int argc, char* argv[]);

void
stop_jrtc();

#endif
