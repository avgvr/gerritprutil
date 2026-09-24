// Copyright (c) 2026, Alexey Gavrilov

#include "socket.h"

int main()
{
    sock::ActiveDedicatedSocket<sock::socktype::stream, sock::domain::inet> actives;
    sock::ActiveDedicatedSocket<sock::socktype::stream, sock::domain::inet>::Address a;
    a.setPlacement("127.0.0.1", 45000);
    actives.bind(a);
    for(;;);
};
