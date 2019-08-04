.PHONY: all clean

all:
	$(MAKE) -C hotspot
	$(MAKE) -C me
	$(MAKE) -C rap
	$(MAKE) -C splash2/codes

clean:
	$(MAKE) -C hotspot clean
	$(MAKE) -C me clean
	$(MAKE) -C rap clean
	$(MAKE) -C splash2/codes clean
