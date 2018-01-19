# Main routine of the C symbols parser.
#
# - Created by tigaliang on 20171111.

DOGGY=doggy

CC=gcc
LEX=lex

SDIR=src
IDIR=src/include
ODIR=obj
ILEX=$(SDIR)/p.l
OLEX=$(SDIR)/p.yy.c

CFLAGS=-I$(IDIR)
LIBS=-ll

SRCS=$(shell ls $(SDIR)/*.c)
OBJS=$(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(SRCS))

HDRS=$(shell ls $(IDIR)/*)

.PHONY: all
all: _lex pre
	@$(MAKE) $(DOGGY)

$(ODIR)/%.o: $(SDIR)/%.c $(HDRS)
	@$(CC) -o $@ $< $(CFLAGS) -c

$(DOGGY): $(OBJS)
	@$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

_lex: $(ILEX)
	@$(LEX) -o $(OLEX) $(ILEX)

pre:
	@mkdir -p $(ODIR)

.PHONY: clean
clean:
	@rm -rf $(ODIR) ; rm -f $(DOGGY) $(OLEX)