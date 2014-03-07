# UDP / Datagram Sockets

    Stability: 3 - Stable

<!-- name=dgram -->

Datagram sockets are available through `require('dgram')`.

Important note: the behavior of `dgram.Socket#bind()` has changed in v0.10
and is always asynchronous now.  If you have code that looks like this:

    var s = dgram.createSocket('udp4');
    s.bind(1234);
    s.addMembership('224.0.0.114');

You have to change it to this:

    var s = dgram.createSocket('udp4');
    s.bind(1234, function() {
      s.addMembership('224.0.0.114');
    });


## dgram.createSocket(type, [callback])

* `type` String. Either 'udp4' or 'udp6'
* `callback` Function. Attached as a listener to `message` events.
  Optional
* Returns: Socket object

Creates a datagram Socket of the specified types.  Valid types are `udp4`
and `udp6`.

Takes an optional callback which is added as a listener for `message` events.

Call `socket.bind` if you want to receive datagrams. `socket.bind()` will bind
to the "all interfaces" address on a random port (it does the right thing for
both `udp4` and `udp6` sockets). You can then retrieve the address and port
with `socket.address().address` and `socket.address().port`.

## Class: dgram.Socket

The dgram Socket class encapsulates the datagram functionality.  It
should be created via `dgram.createSocket(type, [callback])`.

### Event: 'message'

* `msg` Buffer object. The message
* `rinfo` Object. Remote address information

Emitted when a new datagram is available on a socket.  `msg` is a `Buffer` and `rinfo` is
an object with the sender's address information and the number of bytes in the datagram.

### Event: 'listening'

Emitted when a socket starts listening for datagrams.  This happens as soon as UDP sockets
are created.

### Event: 'close'

Emitted when a socket is closed with `close()`.  No new `message` events will be emitted
on this socket.

### Event: 'error'

* `exception` Error object

Emitted when an error occurs.

### socket.send(buf, offset, length, port, address, [callback])

* `buf` Buffer object.  Message to be sent
* `offset` Integer. Offset in the buffer where the message starts.
* `length` Integer. Number of bytes in the message.
* `port` Integer. destination port
* `address` String. destination IP
* `callback` Function. Callback when message is done being delivered.
  Optional.

For UDP sockets, the destination port and IP address must be specified.  A string
may be supplied for the `address` parameter, and it will be resolved with DNS.  An
optional callback may be specified to detect any DNS errors and when `buf` may be
re-used.  Note that DNS lookups will delay the time that a send takes place, at
least until the next tick.  The only way to know for sure that a send has taken place
is to use the callback.

If the socket has not been previously bound with a call to `bind`, it's
assigned a random port number and bound to the "all interfaces" address
(0.0.0.0 for `udp4` sockets, ::0 for `udp6` sockets).

Example of sending a UDP packet to a random port on `localhost`;

    var dgram = require('dgram');
    var message = new Buffer("Some bytes");
    var client = dgram.createSocket("udp4");
    client.send(message, 0, message.length, 41234, "localhost", function(err, bytes) {
      client.close();
    });

**A Note about UDP datagram size**

The maximum size of an `IPv4/v6` datagram depends on the `MTU` (_Maximum Transmission Unit_)
and on the `Payload Length` field size.

- The `Payload Length` field is `16 bits` wide, which means that a normal payload
  cannot be larger than 64K octets including internet header and data
  (65,507 bytes = 65,535 − 8 bytes UDP header − 20 bytes IP header);
  this is generally true for loopback interfaces, but such long datagrams
  are impractical for most hosts and networks.

- The `MTU` is the largest size a given link layer technology can support for datagrams.
  For any link, `IPv4` mandates a minimum `MTU` of `68` octets, while the recommended `MTU`
  for IPv4 is `576` (typically recommended as the `MTU` for dial-up type applications),
  whether they arrive whole or in fragments.

  For `IPv6`, the minimum `MTU` is `1280` octets, however, the mandatory minimum
  fragment reassembly buffer size is `1500` octets.
  The value of `68` octets is very small, since most current link layer technologies have
  a minimum `MTU` of `1500` (like Ethernet).

Note that it's impossible to know in advance the MTU of each link through which
a packet might travel, and that generally sending a datagram greater than
the (receiver) `MTU` won't work (the packet gets silently dropped, without
informing the source that the data did not reach its intended recipient).

### socket.bind(port, [address], [callback])

* `port` Integer
* `address` String, Optional
* `callback` Function, Optional

For UDP sockets, listen for datagrams on a named `port` and optional `address`.
If `address` is not specified, the OS will try to listen on all addresses.

The `callback` argument, if provided, is added as a one-shot `'listening'`
event listener.

Example of a UDP server listening on port 41234:

    var dgram = require("dgram");

    var server = dgram.createSocket("udp4");

    server.on("message", function (msg, rinfo) {
      console.log("server got: " + msg + " from " +
        rinfo.address + ":" + rinfo.port);
    });

    server.on("listening", function () {
      var address = server.address();
      console.log("server listening " +
          address.address + ":" + address.port);
    });

    server.bind(41234);
    // server listening 0.0.0.0:41234


### socket.close()

Close the underlying socket and stop listening for data on it.

### socket.address()

Returns an object containing the address information for a socket.  For UDP sockets,
this object will contain `address` , `family` and `port`.

### socket.setBroadcast(flag)

* `flag` Boolean

Sets or clears the `SO_BROADCAST` socket option.  When this option is set, UDP packets
may be sent to a local interface's broadcast address.

### socket.setTTL(ttl)

* `ttl` Integer

Sets the `IP_TTL` socket option.  TTL stands for "Time to Live," but in this context it
specifies the number of IP hops that a packet is allowed to go through.  Each router or
gateway that forwards a packet decrements the TTL.  If the TTL is decremented to 0 by a
router, it will not be forwarded.  Changing TTL values is typically done for network
probes or when multicasting.

The argument to `setTTL()` is a number of hops between 1 and 255.  The default on most
systems is 64.

### socket.setMulticastTTL(ttl)

* `ttl` Integer

Sets the `IP_MULTICAST_TTL` socket option.  TTL stands for "Time to Live," but in this
context it specifies the number of IP hops that a packet is allowed to go through,
specifically for multicast traffic.  Each router or gateway that forwards a packet
decrements the TTL. If the TTL is decremented to 0 by a router, it will not be forwarded.

The argument to `setMulticastTTL()` is a number of hops between 0 and 255.  The default on most
systems is 1.

### socket.setMulticastLoopback(flag)

* `flag` Boolean

Sets or clears the `IP_MULTICAST_LOOP` socket option.  When this option is set, multicast
packets will also be received on the local interface.

### socket.addMembership(multicastAddress, [multicastInterface])

* `multicastAddress` String
* `multicastInterface` String, Optional

Tells the kernel to join a multicast group with `IP_ADD_MEMBERSHIP` socket option.

If `multicastInterface` is not specified, the OS will try to add membership to all valid
interfaces.

### socket.dropMembership(multicastAddress, [multicastInterface])

* `multicastAddress` String
* `multicastInterface` String, Optional

Opposite of `addMembership` - tells the kernel to leave a multicast group with
`IP_DROP_MEMBERSHIP` socket option. This is automatically called by the kernel
when the socket is closed or process terminates, so most apps will never need to call
this.

If `multicastInterface` is not specified, the OS will try to drop membership to all valid
interfaces.

### socket.unref()

Calling `unref` on a socket will allow the program to exit if this is the only
active socket in the event system. If the socket is already `unref`d calling
`unref` again will have no effect.

### socket.ref()

Opposite of `unref`, calling `ref` on a previously `unref`d socket will *not*
let the program exit if it's the only socket left (the default behavior). If
the socket is `ref`d calling `ref` again will have no effect.
