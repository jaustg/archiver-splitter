# archiver-splitter
Archiver-splitter is a Windows console program that creates a number of archives of a predefined target size from the contents of a given directory. Its first successful build was in the summer of 2014, and several fixes have been made since then. Now it's public.

Program features:
- Takes a directory and puts the contents into ZIP or 7Z files
- Compression or no compression can be configured
- Target archive size can be specified (based on original contents)
- Password for the archives can be specified
- Customizable naming format.

Design shortcomings:
- Requires the use of a work directory
- Platform is currently restricted to Windows
- Sends commands to 7-zip
- Cannot guarantee that the target archive will be smaller than the target archive size

It's currently possible for all the "design shortcomings" to be addressed, with time an effort.
