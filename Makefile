debug: QMAKE_CONFIG=-config debug
release: QMAKE_CONFIG=-config release

all:
	$(MAKE) -C rpc
	$(MAKE) -C common
	$(MAKE) -C server
	$(MAKE) -C client

release: qmake all
debug: qmake all

clean:
	-$(MAKE) -C client $@
	-$(MAKE) -C server $@
	-$(MAKE) -C common $@
	-$(MAKE) -C rpc $@

distclean:
	-$(MAKE) -C client $@
	-$(MAKE) -C server $@
	-$(MAKE) -C common $@
	-$(MAKE) -C rpc $@

qmake:
	cd rpc && qmake $(QMAKE_CONFIG) && cd ..
	cd common && qmake $(QMAKE_CONFIG) && cd ..
	cd server && qmake $(QMAKE_CONFIG) && cd ..
	cd client && qmake $(QMAKE_CONFIG) && cd ..

