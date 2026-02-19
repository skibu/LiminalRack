# Liminal Virtual Keyboard

Need a touch input virtual keyboard for Liminal so that user can easily type info. 
That way can have clear way to save/load settings, filter modules, and so much more.
Raspberry Pi OS now comes with squeekbox, which is a good keyboard. But the default
doesn't have special characters that are important, like @ and /. Therefore want to
provide a custom keyboard that has these characters.

The way a custom keyboard is provided is by putting a custom yaml config file named `us_wide.yaml` 
at `/usr/share/misc/squeekboard/keyboards/`. If you just search for the proper directory you
might get wrong answers like `~/.local/share/squeekboard/keyboards/` but that is wrong. 
You must put the yaml file in the `/usr/share` directory. Also, it must be called `us_wide.yaml` to
be used in landscape mode. Using `us.yaml` will not work!

## Tutorial
See https://world.pages.gitlab.gnome.org/Phosh/squeekboard/tutorial.html

## Customizing Squeekboard 

One can really customize the look of Squeekboard keyboard. Was quite hard to figure out 
how to do so, but with these instructions it is easy to do.

There are two components of configuring the keyboard: 1) the css for visual details like colors; 
and 2) the yaml file for specifying keyboard layout.

## Setting look via css
This is the most complicated part. The css has a default, and you cannot override the defaults. 
But you can override colors, well, sort of.

Since you cannot override default css you need to know where the default is. The style.css file is 
actually compiled into the squeekboard application. But you can see the source file at
https://github.com/raspberrypi-ui/squeekboard/blob/master/data/style.css .

### Creating your own css file
First thing to note is that putting a css file into `~/.config/gtk-3.0/`, contrary to what 
Mr Google says. Turns out that gtk wipes out that directory on bootup and your css file would 
simply be erased.

Instead, you need to modify the `/usr/share/themes/<THEME>/gtk-3.0/gtk.css` file to also include your
custom css file. The way you determine the name of your theme is to use:
```
gsettings get org.gnome.desktop.interface gtk-theme
```
For my Raspberry Pi the theme turns out to be `PiXonyx` so the include file I needed to edit was 
`/usr/share/themes/PiXonyx/gtk-3.0/gtk.css`. You should add a line at the end like:
```
@import url("~/LiminalRack/res/squeekboardCusomtization.css");
```

Next you need to create a customized `squeekboardCusomtization.css` file. In that file you
can override gtk colors (the ones that start with an '@'). You can also specify css directly
for css elements that were not already set in the compiled in `styles.css` file.

### Changing colors
If you want to change a color that has already been defined, such as:
```
background-color: mix(@theme_base_color, @theme_fg_color, 0.1);
```
you cannot change an existing css specification, like `mix(@theme_base_color, @theme_fg_color, 0.1)` 
but you can change the gtk colors such as `@theme_base_color` and `@theme_fg_color`. This will get
you quite far.

### Cannot change fonts
At least I could not figure out a way to change the font. It seems to be hardcoded.

## Specifying keyboard layout via yaml file
Since main desire is to have the keys `@` & `.` for email address for VCV Rack login, 
plus `/` for file names, can start with the email version at 
`/usr/share/misc/squeekboard/keyboards/email/us_wide.yaml` and modify it to add the `.`
and `/` next to the `@` key. Also, added left and right arrow keys and a few other things.
The following is the resulting yaml file to be stored at 
`/usr/share/misc/squeekboard/keyboards/us_wide.yaml`. It is also available in the repo at `LiminalRack/res/keyboard/us_wide.yaml` .

```
---
outlines:
    default:       { width:  48.0, height: 40 }
    shift:         { width:  50.0, height: 40 }
    show-numbers:  { width:  60.0, height: 40 }
    spacebar:      { width: 280.0, height: 40 }
    preferences:   { width:  35.0, height: 40 }
    return:        { width:  94.0, height: 40 }
    backspace:     { width:  56.0, height: 40 }
    secondary-key: { width:  32.0, height: 40 }
    spacer:        { width:   4.0, height: 40 }

views:
    base:
        - "sp q w e r t y u i o p sp sp BackSpace sp"
        - "sp sp sp a s d f g h j k l sp sp sp sp sp sp sp sp sp sp sp left_arrow right_arrow"
        - "show_upper z x c v b n m sp sp sp sp dash underbar slash at dot sp sp"
        - "show_numbers preferences sp sp sp sp space sp sp sp sp return sp sp"
    upper:
        - "sp Q W E R T Y U I O P sp sp BackSpace sp"
        - "sp sp sp A S D F G H J K L sp sp sp sp sp sp sp sp sp sp sp left_arrow right_arrow"
        - "show_upper Z X C V B N M sp sp sp sp dash underbar slash at dot sp sp"
        - "show_numbers preferences sp sp sp sp space sp sp sp sp return sp sp"
    numbers:
        - "sp 1 2 3 4 5 6 7 8 9 0 sp sp sp BackSpace"
        - "sp sp sp @ # $ % & - _ + ( ) sp sp left_arrow right_arrow"
        - "show_symbols , \" ' : ; ! ? sp sp sp sp dash underbar slash at dot sp sp"
        - "show_letters preferences sp sp sp sp space sp sp sp sp return sp sp"
    symbols:
        - "sp ~ ` | · √ π τ ÷ × ¶ sp sp BackSpace"
        - "sp sp sp © ® £ € ¥ ^ ° * { } sp sp left_arrow right_arrow"
        - "show_numbers_from_symbols \\ / < > = [ ]"
        - "show_letters preferences sp sp sp sp space sp sp sp sp return sp sp"

buttons:
    show_upper:
        action:
            locking:
                lock_view: "upper"
                unlock_view: "base"
        outline: "shift"
        icon: "key-shift"
    BackSpace:
        outline: "backspace"
        icon: "edit-clear-symbolic"
        action: "erase"
    # The world
    preferences:
        action: "show_prefs"
        outline: "preferences"
        icon: "keyboard-mode-symbolic"
    show_numbers:
        action:
            set_view: "numbers"
        outline: "show-numbers"
        label: "123"
    show_numbers_from_symbols:
        action:
            set_view: "numbers"
        outline: "show-numbers"
        label: "123"
    show_letters:
        action:
            set_view: "base"
        outline: "show-numbers"
        label: "ABC"
    show_symbols:
        action:
            set_view: "symbols"
        outline: "show-numbers"
        label: "*/="
    space:
        outline: "spacebar"
        text: " "
    return:
        outline: "return"
        icon: "key-enter"
        keysym: "Return"
    up_arrow:
        outline: "secondary-key"
        label: "↑"
        keysym: "Up"
    down_arrow:
        outline: "secondary-key"
        label: "↓"
        keysym: "Down"
    left_arrow:
        outline: "secondary-key"
        label: "←"
        keysym: "Left"
    right_arrow:
        outline: "secondary-key"
        label: "→"
        keysym: "Right"
    at:
        outline: "secondary-key"
        text: "@"
    dot:
        outline: "secondary-key"
        text: "."
    underbar:
        outline: "secondary-key"
        text: "_"
    dash:
        outline: "secondary-key"
        text: "-"
    slash:
        outline: "secondary-key"
        text: "/"
    sp:
        outline: "spacer"
        text: ""
```

## Testing/Iterating
You will likely need to make many iterations in order to get the look exactly as you want it.
To test out your configuration you can using a terminal window kill the `/usr/bin/squeekboard` 
process and then start it up again using `/usr/bin/squeekboard &`
