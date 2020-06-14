# X11 Window System #
## Encodings ##
Some standard encodings

- *X Portable Character Set*: 97 characters, assumed to exist in all locales. Contains:
> a..z A..Z 0..9 !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~  
as well as space, tab, and newline
- *Host Portable Character Encoding*: encoding of the X Portable Character Set on the host. Encoding must be the same across all supported locales on the host.
- *Latin-1*: ISO8859-1 standard Latin character set
- *Latin Portable Character Encoding*: encoding of the X Portable Character Set, using Latin-1 codepoints plus ASCII control characters.

## Locales ##
Localisation in X is based on _locales_, which define:

- Encoding and processing of input method text
- Encoding of resource files and values
- Encoding and imaging of text strings
- Encoding and decoding for inter-client text communication

To manage text input and output, Xlib provides an X Input Method (*XIM*) and an X Output Method (*XOM*) respectively.

### Management ###
According to ISO C all programs use the standard `C` locale by default, not that set in the locale environment variables. The library function `setlocale` is used to announce the locale according to an environment variable, and `XSupportsLocale` is used to determine if the locale is supported by X.


## Keyboard Input ##
To pick up keyboard events we need to register for `KeyPress` and `KeyRelease` events and access the xkeymap element of the corresponding XEvent, which is an XKeyEvent. From this we can get all necessary information about the pressed key, of which the most important (for us) are:

> unsigned int state;
> unsigned int keycode;

The `state` indicates the logical state of modifier keys immediately prior to the envent, consisting of the bitwise inclusive OR or one or more modifier key masks:

> ShiftMask 	      (1<<0)
> LockMask	      (1<<1)
> ControlMask	      (1<<2)
> Mod1Mask	      (1<<3)
> Mod2Mask	      (1<<4)
> Mod3Mask	      (1<<5)
> Mod4Mask	      (1<<6)
> Mod5Mask	      (1<<7)

A KeyCode is an arbitrary numerical representation for a key on the keyboard, lying in the range [8,255] inclusive, and carries no intrinsic information. By contrast a KeySym is an encoding of an actual symbol, and a list of KeySyms is associated with each KeyCode. We can get a KeySym directly from an XKeyEvent with

> KeySym XLookupKeysym(XKeyEvent *key_event, int index)

where `index` is the index into the KeySyms list.

### Latin-1 Keyboard Events ###
To handle standard Latin-1 and ASCII semantics, can use `XLookupString` which to translate a key event to a KeySym and a string. 