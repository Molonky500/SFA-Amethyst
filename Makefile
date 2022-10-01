PATCHES=bootstrap src
include $(PWD)/config.mk

.PHONY: all clean setup install run iso patch setup

ORIGISO ?= original.iso
PATCHISO ?= patched.iso
DATA ?= ../browser/data
ORIGDOL = $(DISCROOT)/../sys/main.dol.orig
NEWDOL = $(DISCROOT)/../sys/main.dol

all: $(BUILDDIR) $(BUILDDIR)/bootstrap.bin $(patsubst %,$(BUILDDIR)/%.elf,$(PATCHES))
	@echo "[*] Done."

$(BUILDDIR): $(ORIGDOL)
	@mkdir -p $(BUILDDIR)
	@mkdir -p patchfiles/sys/
	@mkdir -p patchfiles/files/
	#./tools/genSymbols.py include/sfa/ build/

# generate symbols.s for inclusion in assembler files.
# this lets us reference them without the need for relocation.
$(BUILDDIR)/symbols.s: $(BUILDDIR) $(GLOBALSYMS)
	@echo "[*] Generate symbols.s"
	@sed -n -E 's/(\S+)\s+=\s+(0x........);/.set \1, \2/p' $(GLOBALSYMS) > $@
	@echo '' >> $@

# build bootstrap file that the game will load at startup
$(BUILDDIR)/bootstrap.bin: $(BUILDDIR) $(BUILDDIR)/bootstrap.elf
	@$(TOOL)objcopy -O binary $(BUILDDIR)/bootstrap.elf $(BUILDDIR)/bootstrap.bin
	#@rm $(BUILDDIR)/bootstrap.elf

$(BUILDDIR)/%.elf: $(BUILDDIR) %/main.c
	@$(MAKE) --eval="NAME=$*" -f elf.mk

$(BUILDDIR)/%.elf: $(BUILDDIR) $(BUILDDIR)/symbols.s %/main.s
	@$(MAKE) --eval="NAME=$*" -f elf.mk

# remove build files (but not discroot)
clean:
	rm -rf $(BUILDDIR)
	find . -name \*.o -delete
	@echo "[*] Clean: OK"

# build patch, and install files into discroot
install: all
	@echo "[*] Installing..."
	@./tools/elf2patch.py $(BUILDDIR)/src.elf $(BUILDDIR)/bootstrap.bin $(BUILDDIR)/patch.bin
	@./tools/patchdol.py $(ORIGDOL) $(BUILDDIR)/patch.bin $(NEWDOL)
	@./tools/makebitnames.py $(DATA)/U0/gamebits.xml $(DISCROOT)/bitnames.dat
#	@./tools/mkwiifiles.py $(DISCROOT)/../

# extract ISO
$(ORIGDOL): $(ORIGISO)
	@if test -z "$(DEVKITPRO)"; then echo "DEVKITPRO environment variable is not set"; exit 1; fi
	@echo "[*] Verifying ISO..."
	@md5sum --quiet -c checksums.md5 || (echo "Missing 'original.iso', wrong version, or corrupted file"; exit 1)
	@mkdir -p $(DISCROOT)
	@echo "[*] Extracting ISO..."
	@./tools/isobuilder/__main__.py extractIso $(ORIGISO) $(DISCROOT)/../
	@cp $(DISCROOT)/../sys/main.dol $(ORIGDOL)

# build patch and run it
# dolphin seems to only work with absolute paths for this
run: install
	dolphin-emu -d $(abspath $(NEWDOL))

# build patched ISO
# using bigger compression window (1GB) is necessary to get a patch that's
# ~4MB instead of ~400MB
iso: install
	@echo "[*] Preparing files..."
	@find . -name \*debug.log -delete
	@cp "$(DISCROOT)/bitnames.dat" patchfiles/files/
	@cp "$(DISCROOT)/../sys/main.dol" patchfiles/sys/
	@echo "[*] Building ISO..."
	./tools/isobuilder/__main__.py overwrite setName "Star Fox Adventures: Amethyst Edition" patchIso $(ORIGISO) $(PATCHISO) ./patchfiles

# make patch file
patch: iso
	@echo "[*] Creating patch..."
	@xdelta3 -f -n -B 1073741824 -s $(ORIGISO) $(PATCHISO) patch.xdelta

# set up project (not tested)
setup:
	@mkdir -p patchfiles/files/gametext
	@./tools/gametext/__main__.py injectAllCharsRecursive $(DISCROOT)/gametext patchfiles/files/gametext/
