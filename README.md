Cerepso2vt
==========

Cerepso2vt is a Osmium-based C++ program to create vector tiles in
OpenStreetMap's common data formats – OSM XML and OSM PBF. Cerepso2vt
supports all formats Osmium can write and was designed to be flexible.
It should be easy to add different export formats.
Cerepso2vt's data source is a PostgreSQL database which was imported
using Cerepso.


Dependencies
------------

You have to install following dependencies

* libosmium
* libproj-dev
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
