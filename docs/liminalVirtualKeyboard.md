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

## Recommended
Since main desire is to have the keys `@` & `.` for email address for VCV Rack login, 
plus `/` for file names, can start with the email version at 
`/usr/share/misc/squeekboard/keyboards/email/us_wide.yaml` and modify it to add the `.`
and `/` next to the `@` key. The following is the resulting yaml file to be stored at 
`/usr/share/misc/squeekboard/keyboards/us_wide.yaml`:

```
---
outlines:
    default:       { width: 53.76,  height: 42 }
    change-view:   { width: 80.64,  height: 42 }
    change-view-2: { width: 94.08,  height: 42 }
    spaceline:     { width: 188.16, height: 42 }
    special:       { width: 53.76,  height: 42 }
    special-2:     { width: 94.08,  height: 42 }

views:
    base:
        - "q w e r t y u i o p"
        - "a s d f g h j k l"
        - "show_upper z x c v b n m BackSpace"
        - "show_numbers preferences space @ . / Return"
    upper:
        - "Q W E R T Y U I O P"
        - "A S D F G H J K L"
        - "show_upper Z X C V B N M BackSpace"
        - "show_numbers preferences space @ . / Return"
    numbers:
        - "1 2 3 4 5 6 7 8 9 0"
        - "@ # $ % & - _ + ( )"
        - "show_symbols , \" ' : ; ! ? BackSpace"
        - "show_letters preferences space @ . / Return"
    symbols:
        - "~ ` | · √ π τ ÷ × ¶"
        - "© ® £ € ¥ ^ ° * { }"
        - "show_numbers_from_symbols \\ / < > = [ ] BackSpace"
        - "show_letters preferences space @ . / Return"

buttons:
    show_upper:
        action:
            locking:
                lock_view: "upper"
                unlock_view: "base"
        outline: "change-view"
        icon: "key-shift"
    BackSpace:
        outline: "special-2"
        icon: "edit-clear-symbolic"
        action: "erase"
    preferences:
        action: "show_prefs"
        outline: "special"
        icon: "keyboard-mode-symbolic"
    show_numbers:
        action:
            set_view: "numbers"
        outline: "change-view-2"
        label: "123"
    show_numbers_from_symbols:
        action:
            set_view: "numbers"
        outline: "change-view"
        label: "123"
    show_letters:
        action:
            set_view: "base"
        outline: "change-view-2"
        label: "ABC"
    show_symbols:
        action:
            set_view: "symbols"
        outline: "change-view"
        label: "*/="
    space:
        outline: "spaceline"
        text: " "
    Return:
        outline: "special-2"
        icon: "key-enter"
        keysym: "Return"
```
