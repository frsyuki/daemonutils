daemonutils
===========

## Overview

daemonutils is simplified re-implementation of daemontools.


## Installation

Simply run ./configure && make && make install.

    $ ./configure
    $ make
    $ sudo make install


## Usage

Put executable '.run' files into $prefix/etc/serve directory.

Example:
    /usr/local/kumo-manager.run
    /usr/local/kumo-gateway.run

    $ cat /usr/local/kumo-manager.run
    #!/bin/sh
    exec /usr/local/bin/kumo-manager -l 127.0.0.1

    $ cat /usr/local/kumo-gateway.run
    #!/bin/sh
    exec /usr/local/bin/kumo-gateway -m 127.0.0.1

To start services, run serve-kickstart command as following:

    $ serve-kickstart -s /usr/local/etc/service

Use serve command to get status of or control services:

    $ serve kumo-manager stat
    $ serve kumo-manager stop
    $ serve kumo-manager start

Use serve-list command to get status of all serveces.

   $ serve-list /usr/local/etc/serve


## License

    Copyright (c) 2009 FURUHASHI Sadayuki
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

