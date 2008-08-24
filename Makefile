all:
	$(MAKE) -C rpc install
	$(MAKE) -C common
	$(MAKE) -C server
	$(MAKE) -C client

clean:
	$(MAKE) -C rpc $@
	$(MAKE) -C common $@
	$(MAKE) -C server $@
	$(MAKE) -C client $@

qmake:
	for %%d in (rpc common server client) cd %%d; qmake; cd..;	
