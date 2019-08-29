This is rpld, as it currently is a dirty hack between radvd (log handling,
socket, etc) and unstrung (rpl.h).

As radvd requires only libc dependencies, we have more fancy stuff to make
high level handling easier, so we need:

 - lua (version >= XXX) for config parsing
 - ev (version >= XXX) for high level timer, loop handling
 - mnl (version >= XXX) for high level netlink handling

As this version is a hack and shows my experiments... it will do something
to have a routable thing afterwards in a Linux IEEE 802.15.4 6LoWPAN network.

I have 1001 ideas to make the handling better, but this comes all afterwards.

Build:

$ mkdir build

$ meson build

$ ninja -C build

Testing:

If you want to test the implementation just invoke tests/start script.

Topology (default example, can be changed by changing the script):

             0
            / \
           2   1
             \ | \
               3  |
               | /
               4
               |
               5

The numbers are the corresponding namespace ns#.

RPL ranks and resulting DODAG is:

             1 <--- root
            / \
           2   2
               | \
               3  |
                 /
               3
               |
               4

OR (depends on first come first serve)

             1 <--- root
            / \
           2   2
             \   \
               3  |
                 /
               3
               |
               4

I did here mostly the same behaviour as unstrung implementation
by Michael Richardson https://github.com/AnimaGUS-minerva/unstrung

However you can do a:

$ ip -n ns5 a

remember the IPv6 address of lowpan interface.

Then try a:

$ ip netns exec ns0 traceroute -6 $IPV6_ADDRESS

Then you see some hops to reach the node of ns4 from the root.

HOW IT WORKS?

It uses link-local address for as nexthop addresses. It setups a prefix
carried by DIO and setups necessary routing tables.

TODO

This stuff is all early state, it has no clever logic of avoiding netlink
messages. I have 1001 ideas to make some timeout handling so repairing
will work, but that's all future stuff. Sorry, this implementation was
only made to figure out how this routing protocol works...

Several memory leaks and some bufferoverflows, the RPL messages need to
be very tight and setting the right flags with the right additional data
otherwise it will not work. Everything can be added but this works for
now.

Parent Acknowledge? I tried to implement it, I stopped working because
it begun to finally working... sure there is a lot of security stuff
it's one of these routing protocol implementation which just blindly
accept router discovery messages.

The fact that we only use stateless configuration we could use stateful
compression for the used global prefix, but this will only work for
Linux to Linux (not sure about that ... this implementation is only
tested between Linux systems so... maybe future work but needs a
netlink interface for stateful compression inside the Linux kernel)
