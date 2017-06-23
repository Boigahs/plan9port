#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

typedef struct Radio Radio;

struct Radio
{
	Control	control;
	int		value;
	int		lastbut;
	Control	**buttons;
	int		nbuttons;
	char		*eventstr;
};

enum{
	EAdd,
	EButton,
	EFocus,
	EFormat,
	EHide,
	ERect,
	EReveal,
	EShow,
	ESize,
	EValue,
};

static char *cmds[] = {
	[EAdd] =		"add",
	[EButton] =	"button",
	[EFocus] = 	"focus",
	[EFormat] = 	"format",
	[EHide] =		"hide",
	[ERect] =		"rect",
	[EReveal] =	"reveal",
	[EShow] =		"show",
	[ESize] =		"size",
	[EValue] =		"value",
	nil
};

static void	radioshow(Radio*);

static void
radiomouse(Control *c, Mouse *m)
{
	Radio *r;
	int i;

	r = (Radio*)c;
	for(i=0; i<r->nbuttons; i++)
		if(ptinrect(m->xy, r->buttons[i]->rect) && r->buttons[i]->mouse){
			(r->buttons[i]->mouse)(r->buttons[i], m);
			break;
		}
}

static void
radioshow(Radio *r)
{
	int i;

	if (r->control.hidden)
		return;
	for(i=0; i<r->nbuttons; i++){
		_ctlprint(r->buttons[i], "value %d", (r->value==i));
		_ctlprint(r->buttons[i], "show");
	}
}

static void
radioctl(Control *c, CParse *cp)
{
	int cmd, i;
	Rectangle rect;
	Radio *r;
	char fmt[256];

	r = (Radio*)c;

	cmd = _ctllookup(cp->args[0], cmds, nelem(cmds));
	switch(cmd){
	default:
		ctlerror("%q: unrecognized message '%s'", r->control.name, cp->str);
		break;
	case EAdd:
		_ctlargcount(&r->control, cp, 2);
		c = controlcalled(cp->args[1]);
		if(c == nil)
			ctlerror("%q: can't add %s: %r", r->control.name, cp->args[1]);
		snprint(fmt, sizeof fmt, "%%q: %q button %%d", r->control.name);
		_ctlprint(c, "format %q", fmt);
		controlwire(c, "event", c->controlset->ctl);
		r->buttons = ctlrealloc(r->buttons, (r->nbuttons+1)*sizeof(Control*));
		r->buttons[r->nbuttons] = c;
		r->nbuttons++;
		r->value = -1;
		radioshow(r);
		break;
	case EButton:
		if (cp->sender == nil)
			ctlerror("%q: senderless buttonevent: %q", r->control.name, cp->str);
		c = controlcalled(cp->sender);
		for(i=0; i<r->nbuttons; i++)
			if (c == r->buttons[i])
				break;
		if (i == r->nbuttons)
			ctlerror("%q: not my event: %q", r->control.name, cp->str);
		if(cp->iargs[1] == 0){
			/* button was turned off; turn it back on */
			_ctlprint(c, "value 1");
		}else{
			r->value = i;
			chanprint(r->control.event, r->control.format, r->control.name, i);
			radioshow(r);
		}
		break;
	case EFormat:
		_ctlargcount(&r->control, cp, 2);
		r->control.format = ctlstrdup(cp->args[1]);
		break;
	case EHide:
		_ctlargcount(&r->control, cp, 1);
		r->control.hidden = 1;
		break;
	case EFocus:
		/* ignore focus change */
		break;
	case ERect:
		_ctlargcount(&r->control, cp, 5);
		rect.min.x = cp->iargs[1];
		rect.min.y = cp->iargs[2];
		rect.max.x = cp->iargs[3];
		rect.max.y = cp->iargs[4];
		r->control.rect = rect;
		break;
	case EReveal:
		_ctlargcount(&r->control, cp, 1);
		r->control.hidden = 0;
		radioshow(r);
		break;
	case EShow:
		_ctlargcount(&r->control, cp, 1);
		radioshow(r);
		break;
	case ESize:
		if (cp->nargs == 3)
			rect.max = Pt(0x7fffffff, 0x7fffffff);
		else{
			_ctlargcount(&r->control, cp, 5);
			rect.max.x = cp->iargs[3];
			rect.max.y = cp->iargs[4];
		}
		rect.min.x = cp->iargs[1];
		rect.min.y = cp->iargs[2];
		if(rect.min.x<=0 || rect.min.y<=0 || rect.max.x<=0 || rect.max.y<=0 || rect.max.x < rect.min.x || rect.max.y < rect.min.y)
			ctlerror("%q: bad sizes: %s", r->control.name, cp->str);
		r->control.size.min = rect.min;
		r->control.size.max = rect.max;
		break;
	case EValue:
		_ctlargcount(&r->control, cp, 2);
		r->value = cp->iargs[1];
		radioshow(r);
		break;
	}
}

Control*
createradiobutton(Controlset *cs, char *name)
{
	Radio *r;

	r = (Radio*)_createctl(cs, "label", sizeof(Radio), name);
	r->control.format = ctlstrdup("%q: value %d");
	r->value = -1;	/* nobody set at first */
	r->control.mouse = radiomouse;
	r->control.ctl = radioctl;
	return (Control*)r;
}
