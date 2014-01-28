quassel-irssi
=============

An irssi plugin to connect to quassel core

You can discuss about it on #quassel-irssi on Freenode.

Huge thanks to QuasselDroid. QuasselDroid protocol code is way easier to read than Quassel's.

How to build
------------

    git clone https://github.com/phhusson/quassel-irssi.git
    cd quassel-irssi
    git submodule init
    git submodule update
    cd core
    make
    cp libquassel_core.so ~/.irssi/modules/

How to use
----------

Edit ~/.irssi/config and add a quassel chatnet with Quassel type, and your quassel server accordingly:

    chatnets = {
      quassel_phh = {
        type = "Quassel";
      };
    };

    servers = (
      { address = "quassel.phh.me"; port = "4242"; chatnet = "quassel_phh"; password="password"; use_ssl="yes"; }
    );

You may disable ssl if you really need to.

You also need to have your login equal to your nick:

    settings = {
      core = { nick = "phh"; };
    }
  
(I'm guessing it's not needed, you can specifiy it on /connect command)

Then, launch irssi, and do:

    /load quassel
    /connect quassel_phh
  
(where quassel_phh is the chatnet name)

To automtically connect to it, you can add:

    load quassel
    connect quassel_phh
  
in your ~/.irssi/startup

If someone knows how to do all this 100% from irssi text frontend, i'd appreciate it :-)
