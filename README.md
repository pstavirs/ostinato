<a href='http://www.addthis.com/bookmark.php?v=250&amp;username=pstavirs&amp;url=http%3A%2F%2Fostinato.googlecode.com&amp;title=' title='Bookmark and Share'><img src='http://s7.addthis.com/static/btn/v2/lg-share-en.gif' alt='Bookmark and Share' width='125' height='16' /></a> 

---

**Breaking News (2015-06-15): Ostinato version 0.7.1 released!** [ChangeLog](http://ostinato.org/wiki/ChangeLog)

---

&lt;wiki:gadget url="http://hosting.gmodules.com/ig/gadgets/file/117254926214212765027/image\_selector.xml" width="100%" height="100" border="0" /&gt;

# Introduction #

Ostinato is an open-source, cross-platform network packet crafter/traffic generator and analyzer with a friendly GUI. Craft and send packets of several streams with different protocols at different rates. For the full feature list see [below](#Features.md).

Ostinato aims to be "Wireshark in Reverse" and become complementary to Wireshark.

Here's a screencast showing basic usage -

<a href='http://www.youtube.com/watch?feature=player_embedded&v=On64lQYEFlY' target='_blank'><img src='http://img.youtube.com/vi/On64lQYEFlY/0.jpg' width='640' height=385 /></a>

# Features #
  * Runs on Windows, Linux, BSD and Mac OS X (Will probably run on other platforms also with little or no modification but this hasn't been tested)
  * Open, edit, replay and save PCAP files
  * Support for the most common standard protocols
    * Ethernet/802.3/LLC SNAP
    * VLAN (with QinQ)
    * ARP, IPv4, IPv6, IP-in-IP a.k.a IP Tunnelling (6over4, 4over6, 4over4, 6over6)
    * TCP, UDP, ICMPv4, ICMPv6, IGMP, MLD
    * Any text based protocol (HTTP, SIP, RTSP, NNTP etc.)
    * More protocols in the works ...
  * Modify any field of any protocol (some protocols allow changing packet fields with every packet at run time e.g. changing IP/MAC addresses)
  * User provided Hex Dump - specify some or all bytes in a packet
  * User defined script to substitute for an unimplemented protocol (EXPERIMENTAL)
  * Stack protocols in any arbitrary order
  * Create and configure multiple streams
  * Configure stream rates, bursts, no. of packets
  * Single client can control and configure multiple ports on multiple computers generating traffic
  * Exclusive control of a port to prevent the OS from sending stray packets provides a controlled testing environment
  * Statistics Window shows realtime port receive/transmit statistics and rates
  * Capture packets and view them (needs Wireshark to view the captured packets)
  * Framework to add new protocol builders easily

Some screenshots (click to view larger image) -

| ![![](http://wiki.ostinato.googlecode.com/hg/screenshots/thumbs/mainWin.png)](http://wiki.ostinato.googlecode.com/hg/screenshots/mainWin.png) | ![![](http://wiki.ostinato.googlecode.com/hg/screenshots/thumbs/scdProtoSimple.png)](http://wiki.ostinato.googlecode.com/hg/screenshots/scdProtoSimple.png) | ![![](http://wiki.ostinato.googlecode.com/hg/screenshots/thumbs/scdProtoData.png)](http://wiki.ostinato.googlecode.com/hg/screenshots/scdProtoData.png) |
|:----------------------------------------------------------------------------------------------------------------------------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------------------|:--------------------------------------------------------------------------------------------------------------------------------------------------------|
| Main Window                                                                                                                                   | Stream Configuration -<br>Protocol Selection (Simple Mode)                                                                                                  <table><thead><th> Stream Configuration -<br>Protocol Data                                                                                                                 </th></thead><tbody>
<tr><td> <a href='http://wiki.ostinato.googlecode.com/hg/screenshots/scdControl.png'><img src='http://wiki.ostinato.googlecode.com/hg/screenshots/thumbs/scdControl.png' /></a> </td><td> <a href='http://wiki.ostinato.googlecode.com/hg/screenshots/scdPktView.png'><img src='http://wiki.ostinato.googlecode.com/hg/screenshots/thumbs/scdPktView.png' /></a> </td><td> <a href='Screenshots.md'>More Screenshots ...</a>                                                                                                       </td></tr>
<tr><td> Stream Configuration -<br>Stream Control                                                                                                      </td><td> Stream Configuration -<br>Packet View                                                                                                                       </td><td>                                                                                                                                                         </td></tr></tbody></table>


<h1>Testimonials</h1>
<i>...thanks for developing ostinato: this was a really missing piece in the open-source networking world</i> - Luca Deri (ntop.org) <a href='http://groups.google.com/group/ostinato/browse_thread/thread/65d891d995ddb747'>Email</a>

<i>Before I go any further I need to give another shout out to an excellent open source piece of software I found. Ostinato turns you into a packet ninja. There’s literally no limit to what you can do with it. Without Ostinato I could have never gotten beyond this point</i> - Kristian Kielhofner ("Packets of Death" AstLinux, Star2Star) <a href='http://blog.krisk.org/2013/02/packets-of-death.html'>Blog</a>

<i>Napatech supports a number of open-source software applications that can also help customers accelerate their development ... Ostinato for Traffic Generation</i> <a href='http://www.hpcwire.com/hpcwire/2011-02-15/napatech_releases_new_software_suite_for_network_appliances.html'>Article</a>

<i>Ostinato is more stable <code>[</code>than similar tools<code>]</code> and has a more complete roadmap</i> <a href='http://www.iniqua.com/2010/12/09/packeth-vs-ostinato/?lang=en#comment-1796'>Blog</a>

<i>If your <code>[</code>sic<code>]</code> looking for GUI based packet generator then Ostinato is one of the best option</i> <a href='http://baluenigma.blogspot.com/2010/06/ostinato-packet-generater.html'>Blog</a>

<i>First - great program - and I'm so happy to see that it's actively being developed</i> <a href='http://groups.google.com/group/ostinato/browse_thread/thread/5b1af73fe9e06c40/5b24e127be862db4?lnk=gst&q=it%27s+actively+being+developed#5b24e127be862db4'>Email</a>

<i>Many thanks for developing this great software and giving good support</i> <a href='http://groups.google.com/group/ostinato/browse_thread/thread/1955b62fbce4a994/003a664120e7a421?lnk=gst&q=great+software+and+giving+good+support#003a664120e7a421'>Email</a>

<h1>News</h1>
<h2>June 2015</h2>
<ul><li>Ostinato podcast with [twitter.com/ecbanks @ecbanks] <a href='http://packetpushers.net/pq-show-52-using-ostinato-to-craft-your-own-packets/'>PacketPushers</a>
</li><li><b>0.7.1 released</b> <a href='http://code.google.com/p/ostinato/wiki/ChangeLog#2015-06-16_version_0.7.1'>ChangeLog</a>
</li><li><b>0.7 released</b> <a href='http://code.google.com/p/ostinato/wiki/ChangeLog#2015-06-09_version_0.7'>ChangeLog</a>
<h2>December 2014</h2>
</li><li>vDrone - drone only appliance published<br>
<h2>August 2014</h2>
</li><li><a href='http://www.slideshare.net/pstavirs/dpdk-accelerated-ostinato'>DPDK accelerated Ostinato</a> - Solo Prize Winner of 6WIND: Speed Matters DPDK design contest<br>
<wiki:gadget url="http://hosting.gmodules.com/ig/gadgets/file/108621208120033273647/my_gadgets_gs.xml" width="425" height="355" border="0" up_File="http://www.slideshare.net/slideshow/embed_code/38396194" up_ContainerCol="#ffffff" up_FWidth="425" up_FHeight="355" up_FScroll="no" up_Text="DPDK accelerated Ostinato Slides"/><br>
<h2>July 2014</h2>
</li><li><b>0.6 released</b> <a href='http://code.google.com/p/ostinato/wiki/ChangeLog#2014-07-07_version_0.6'>ChangeLog</a>
<h2>June 2014</h2>
</li><li><b>Ostinato Python Bindings 0.6beta release available on PyPi</b> <a href='https://groups.google.com/forum/#!topic/ostinato/pty-caEgmbk'>Details</a>
<h2>May 2013</h2>
</li><li>Linux binary packages now available for recent distros - Debian 7.0, Fedora 18, openSUSE 12.3, Ubuntu 12.10, Ubuntu 13.04<br>
<h2>March 2013</h2>
</li><li>Ostinato is now available in the <a href='http://svnweb.freebsd.org/ports/head/net/ostinato/'>FreeBSD ports collection</a>
<h2>August 2012</h2>
</li><li><b>0.5.1 released</b> <a href='http://code.google.com/p/ostinato/wiki/ChangeLog#2012-08-01_version_0.5.1'>ChangeLog</a>
<h2>June 2012</h2>
</li><li>Linux binary packages available for Fedora 17 and Ubuntu 12.04</li></ul>

<a href='http://code.google.com/p/ostinato/wiki/News'>Older News</a>

<a href='http://www.twitter.com/ostinato'><img src='http://twitter-badges.s3.amazonaws.com/t_mini-a.png' alt='Follow ostinato on Twitter' /></a> Follow Ostinato News on <a href='http://www.twitter.com/ostinato'>Twitter</a>

<h1>Documentation</h1>
<ol><li>A <a href='UserGuide#Quickstart.md'>Quickstart</a> for the impatient<br>
</li><li><a href='UserGuide.md'>User Guide</a>
</li><li><a href='UserScriptHOWTO.md'>Writing a script to fill-in for an unimplemented protocol</a>
</li><li><i>Developers:</i> read <a href='ProtocolBuilderHOWTO.md'>Writing a Protocol Builder</a></li></ol>

For more, see the <a href='http://code.google.com/p/ostinato/w/list'>full list</a> of documents in the wiki.<br>
<br>
Slides from the FOSS.IN 2010 Conference -<br>
<wiki:gadget url="http://hosting.gmodules.com/ig/gadgets/file/108621208120033273647/my_gadgets_gs.xml" width="425" height="355" border="0" up_File="http://www.slideshare.net/slideshow/embed_code/key/4GL89vItSSaxZD" up_ContainerCol="#ffffff" up_FWidth="425" up_FHeight="355" up_FScroll="no" up_Text="Ostinato @ FOSS.IN 2010"/><br>
<br>
<h1>Getting Ostinato</h1>
Source and binary packages are available for several platforms/distros. See <a href='http://code.google.com/p/ostinato/wiki/Downloads?tm=2'>Downloads</a>

<h1>Mailing List</h1>
For queries/bugs/feedback/suggestions, send an email to the mailing list - ostinato@googlegroups.com<br>
<br>
You can also <a href='http://groups.google.com/group/ostinato/subscribe'>join</a> the mailing list or read the <a href='http://groups.google.com/group/ostinato/topics'>archives</a>.<br>
<br>
<h1>Contributing to Ostinato</h1>
For source code contributions (fixes/features etc.) -<br>
<ul><li>Build instructions are available <a href='BuildingFromSource.md'>here</a><br>
</li><li>Documentation to add support for a protocol is available <a href='ProtocolBuilderHOWTO.md'>here</a>
</li><li>See ContributingCodeToOstinato for the process to follow and other important information</li></ul>

You can also contribute to Ostinato in other ways such as testing, reporting bugs, improving the documentation, spreading the word, build/maintain packages for different platforms/distributions etc. - for all such contributions, please send an email to ostinato at googlegroups dot com<br>
<br>
<br>
<wiki:gadget url="http://hosting.gmodules.com/ig/gadgets/file/117254926214212765027/addThisNoHover.xml" width="100%" height="20" border="0"/>