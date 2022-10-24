### ScottPlayer

####WIP
Barebones FSEQ Sequence Player and Scheduler. 
Only Supports Artnet, E131, and DDP Outputs. Supports v2 ZSTD, v2 Uncompressed, and v1 FSEQs, not v2 zlib 

####TODO
 - Finish Scheduler
 - Add Multisync
 - Fix ZSTD dependency install issues

####Other Goals
 - Remote Falcon Support
 - RDS FM Support
 - TPLink Support
 - Colorlight Support

### Building
Uses C++20, QT 5.15, spdlog, and cMake.

```git clone https://github.com/computergeek1507/ScottPlayer.git```

To build on Windows, use Visual Studio 2022

```VS2022.bat```

If you get a qt cmake error, update the QT location in batch file.

To build on Linux with g++(tested on Mint Linux 21).

```
mkdir build
cd build
cmake ..
cmake --build .
./ScottPlayer
```
