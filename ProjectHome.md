File notification that aims to be more powerful than the default QFileSystemWatcher class.  Uses whatever native API is available to implement a consistent yet powerful interface.

Currently, inotify interface is complete - 1 minor assertion failure when in debug mode.

TODO:
Windows implementation:  Should be much faster since the supporting code should be complete & windows supports native recursive folders.  Issues with case sensitivity may come up.

MacOSX implementation: File System Events API.  Should be very similar to inotify structure.