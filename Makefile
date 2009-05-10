all:
	$(MAKE) -C rpc
	$(MAKE) -C common
	$(MAKE) -C server
	$(MAKE) -C client

clean:
	$(MAKE) -C rpc $@
	$(MAKE) -C common $@
	$(MAKE) -C server $@
	$(MAKE) -C client $@

distclean:
	$(MAKE) -C rpc $@
	$(MAKE) -C common $@
	$(MAKE) -C server $@
	$(MAKE) -C client $@

qmake:
	cd rpc && qmake && cd ..
	cd common && qmake && cd ..
	cd server && qmake && cd ..
	cd client && qmake && cd ..
