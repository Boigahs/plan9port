#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Box Box;

struct Box
{
	Control	control;
	int		border;
	CImage	*bordercolor;
	CImage	*image;
	int		align;
};

enum{
	EAlign,
	EBorder,
	EBordercolor,
	EFocus,
	EHide,
	EImage,
	ERect,
	EReveal,
	EShow,
	ESize,
};

static char *cmds[] = {
	[EAlign] =		"align",
	[EBorder] =	"border",
	[EBordercolor] ="bordercolor",
	[EFocus] = 	"focus",
	[EHide] =		"hide",
	[EImage] =	"image",
	[ERect] =		"rect",
	[EReveal] =	"reveal",
	[EShow] =		"show",
	[ESize] =		"size",
	nil
};

static void
boxkey(Control *c, Rune *rp)
{
	Box *b;

	b = (Box*)c;
	chanprint(b->control.event, "%q: key 0x%x", b->control.name, rp[0]);
}

static void
boxmouse(Control *c, Mouse *m)
{
	Box *b;

	b = (Box*)c;
	if (ptinrect(m->xy,b->control.rect))
		chanprint(b->control.event, "%q: mouse %P %d %ld", b->control.name,
			m->xy, m->buttons, m->msec);
}

static void
boxfree(Control *c)
{
	_putctlimage(((Box*)c)->image);
}

static void
boxshow(Box *b)
{
	Image *i;
	Rectangle r;

	if(b->control.hidden)
		return;
	if(b->border > 0){
		border(b->control.screen, b->control.rect, b->border, b->bordercolor->image, ZP);
		r = insetrect(b->control.rect, b->border);
	}else
		r = b->control.rect;
	i = b->image->image;
	/* BUG: ALIGNMENT AND CLIPPING */
	draw(b->control.screen, r, i, nil, ZP);
}

static void
boxctl(Control *c, CParse *cp)
{
	int cmd;
	Rectangle r;
	Box *b;

	b = (Box*)c;
	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", b->control.name, cp->str);
		break;
	case EAlign:
		_ctlargcount(&b->control, cp, 2);
		b->align = _ctlalignment(cp->args[1]);
		break;
	case EBorder:
		_ctlargcount(&b->control, cp, 2);
		if(cp->iargs[1] < 0)
			ctlerror("%q: bad border: %c", b->control.name, cp->str);
		b->border = cp->iargs[1];
		break;
	case EBordercolor:
		_ctlargcount(&b->control, cp, 2);
		_setctlimage(&b->control, &b->bordercolor, cp->args[1]);
		break;
	case EFocus:
		_ctlargcount(&b->control, cp, 2);
		chanprint(b->control.event, "%q: focus %s", b->control.name, cp->args[1]);
		break;
	case EHide:
		_ctlargcount(&b->control, cp, 1);
		b->control.hidden = 1;
		break;
	case EImage:
		_ctlargcount(&b->control, cp, 2);
		_setctlimage(&b->control, &b->image, cp->args[1]);
		break;
	case ERect:
		_ctlargcount(&b->control, cp, 5);
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		r.max.x = cp->iargs[3];
		r.max.y = cp->iargs[4];
		if(Dx(r)<0 || Dy(r)<0)
			ctlerror("%q: bad rectangle: %s", b->control.name, cp->str);
		b->control.rect = r;
		break;
	case EReveal:
		_ctlargcount(&b->control, cp, 1);
		b->control.hidden = 0;
		boxshow(b);
		break;
	case EShow:
		_ctlargcount(&b->control, cp, 1);
		boxshow(b);
		break;
	case ESize:
		if (cp->nargs == 3)
			r.max = Pt(0x7fffffff, 0x7fffffff);
		else{
			_ctlargcount(&b->control, cp, 5);
			r.max.x = cp->iargs[3];
			r.max.y = cp->iargs[4];
		}
		r.min.x = cp->iargs[1];
		r.min.y = cp->iargs[2];
		if(r.min.x<=0 || r.min.y<=0 || r.max.x<=0 || r.max.y<=0 || r.max.x < r.min.x || r.max.y < r.min.y)
			ctlerror("%q: bad sizes: %s", b->control.name, cp->str);
		b->control.size.min = r.min;
		b->control.size.max = r.max;
		break;
	}
}

Control*
createbox(Controlset *cs, char *name)
{
	Box *b;

	b = (Box *)_createctl(cs, "box", sizeof(Box), name);
	b->image = _getctlimage("white");
	b->bordercolor = _getctlimage("black");
	b->align = Aupperleft;
	b->control.key = boxkey;
	b->control.mouse = boxmouse;
	b->control.ctl = boxctl;
	b->control.exit = boxfree;
	return (Control *)b;
}
