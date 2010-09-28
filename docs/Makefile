CTEX=pdflatex
FLAGS=
IMG=../img/ensimag.png ../img/microblaze.png
DEPS=../img/head.tex $(IMG)
LOG=/dev/null

.PHONY: clean all


all: conception.pdf utilisateur.pdf

%.pdf: %.tex $(DEPS)
	@(echo -n "Compilation de la doc $*...")
	@($(CTEX) $< > $(LOG))
	@($(CTEX) $< > $(LOG))
	@($(CTEX) $< > $(LOG))
	@(rm $*.{aux,toc,log,out})
	@(echo "[DONE]")

clean:
	@(echo -n "Suppress all generated pdf files...")
	@(rm *.pdf 2> $(LOG) || echo -n " No pdf files...")
	@(echo "[DONE]")
