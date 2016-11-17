Vectortile-Generator
====================

The Vectortile Generator is a Osmium-based C++ program to create vectortiles in
OpenStreetMap's common data formats – OSM XML and OSM PBF. Its data source is a
PostgreSQL database which was populated using the Cerepso import tool.


Dependencies
------------

You have to install following dependencies

* libosmium
* postgres-drivers
* libpq (use the packages of your distribution)
* C++11 compiler, e.g. g++4.6 or newer


Building
--------

```
mkdir build
cd build
ccmake ..
make
```


Unit Tests
----------

Unit tests are located in the `test/` directory. We use the Catch framework, all
necessary depencies are included in this repository.


License
-------
This program is available under the terms of GNU General Public License 2.0 or
newer. For the legal code of GNU General Public License 2.0 see the file COPYING.md in this directory.
