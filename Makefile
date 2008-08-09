all:
	$(MAKE) -C rpc install
	$(MAKE) -C common
	$(MAKE) -C server
	$(MAKE) -C client
