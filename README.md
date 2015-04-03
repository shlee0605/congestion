Congestion Control
==================

To run the tester,
```sh
./tester -w 10 3a/reference -w 10 -t 300
```

Note: include "reliable.c" file only when you do any commit.

## Caveats

According to the instructions,
conn_input() should return 0 when no data is available,
but it seems it actually returns -1
because read() is causing EAGAIN for some reason;
it should return -1 when EOF is met,
but it seems it actually returns 0
because read() is causing EAGAIN for some reason.
So our rel_read() adjusts to this observation
instead of following the instructions.

## About Cmake

[Cmake](http://www.cmake.org) is a meta-build tool
that can generate makefiles, Xcode projects,
Visual Studio projects, Eclipse projects, etc.
It is an ideal tool for a team
where members use different IDEs.