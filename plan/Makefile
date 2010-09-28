CTEX=pdflatex
FLAGS=--interaction nonstopmode
IMG=../img/ensimag.png ../img/microblaze.png
DEPS=../img/head.tex $(IMG)
LOG=/dev/null

.PHONY: clean all


all: plan_de_validation.pdf cahier_des_charges.pdf planning.pdf

%.pdf: %.tex $(DEPS)
	@(echo -n "Compilation de la doc $*...")
	@($(CTEX) $(FLAGS) $< > $(LOG) || echo "[FAIL]" && exit)
	@($(CTEX) $(FLAGS) $< > $(LOG) || echo "[FAIL]" && exit)
	@($(CTEX) $(FLAGS) $< > $(LOG) || echo "[FAIL]" && exit)
	@(rm $*.{aux,toc,log,out})
	@(echo "[DONE]")

clean:
	@(echo -n "Suppress all generated pdf files...")
	@(rm *.pdf 2> $(LOG) || echo -n " No pdf files...")
	@(echo "[DONE]")
