.SUFFIXES: .ltx .ps .pdf .svg

.svg.pdf:
	inkscape --export-pdf=$@ $<

LATEX_FILES = cheri_compartments.ltx

DIAGRAMS =

all: cheri_compartments.pdf

clean:
	rm -rf ${DIAGRAMS} ${DIAGRAMS:S/.pdf/.eps/}
	rm -rf cheri_compartments.aux cheri_compartments.bbl cheri_compartments.blg cheri_compartments.dvi cheri_compartments.log cheri_compartments.ps cheri_compartments.pdf cheri_compartments.toc cheri_compartments.out cheri_compartments.snm cheri_compartments.nav cheri_compartments.vrb cheri_compartments_preamble.fmt texput.log

cheri_compartments.pdf: bib.bib ${LATEX_FILES} ${DIAGRAMS} cheri_compartments_preamble.fmt
	pdflatex cheri_compartments.ltx
	bibtex cheri_compartments
	pdflatex cheri_compartments.ltx
	pdflatex cheri_compartments.ltx

cheri_compartments_preamble.fmt: cheri_compartments_preamble.ltx
	set -e; \
	  tmpltx=`mktemp`; \
	  cat ${@:fmt=ltx} > $${tmpltx}; \
	  grep -v "%&${@:_preamble.fmt=}" ${@:_preamble.fmt=.ltx} >> $${tmpltx}; \
	  pdftex -ini -jobname="${@:.fmt=}" "&pdflatex" mylatexformat.ltx $${tmpltx}; \
	  rm $${tmpltx}

bib.bib: softdevbib/softdev.bib
	softdevbib/bin/prebib softdevbib/softdev.bib > bib.bib
