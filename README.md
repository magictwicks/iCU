# Team Leopard - iCU
#### By Levi Wicks, David Melesse, & Ryan Magnuson
This is the iCU project submissions for Team Leopard. By compiling `client.c` and `server.c` together,
we create a simultaneous server/client iCU application.  

## Dependencies 

This program requires libcurl in order to make HTTPS requests and libpcap for packet sniffing. 
Make sure the `libcurl`, `libcurl-devel`, & `libpcap-devel` libraries are installed. 

## Compiling
While everything is already compiled for your ease of use, you can use `make` to compile this project.
The resulting `icu` file is the one you want to run.

## Running
Run with `sudo ./icu`. You'll likely need to change your network interface in `client.c` and then recompile
in order for the program to actually run properly after compiling. You can find your network interface by running
`ifconfig`.
