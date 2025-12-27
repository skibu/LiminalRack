# Liminal Rack Change Log

- 12/27/25
  - Shadows added to knobs to provide a more interesting 3D look
  
- 12/24/25
  - Changed to use a more "artistic" font, Oregano, so that doesn't look so much like a computer. Also adjusted font heights.
   - Improved handling of keeping cables visible. Now only straightens cables when it is actually useful.

- 12/23/25
  - Added to module Browser window text labels for each module. It can be quite nice to see text for some modules that don't clearly indicate what they are. Nice complement to the tooltip info.
  - Improved module shadow in Browser window. Originally shadow was on all 4 sides, which defies physics and good UI design. So now only have shadows on bottom and left side, which is pretty standard.

- 12/22/25
  - Now can set colors for any scrollbar. Used this to have very light opacity for the rack scroll widget so that scrollbars don't cover cables. But use regular opacity for other scrollbars so that they are easy to see.
  - Fixed module Browser window so that modules displayed when select "All brands". 

- 12/14/25
  - Fixed changes to Widget and other classes so that 3rd-party plugin libraries will work without requiring them to be recompiled to Liminal Rack.
  - Got the default patch working again.

- 12/10/25 
  - Improved VCV Rack login menu

- 12/9/25 
  - Changed so when module added from browser window it is selected so that user can easily see what was added and where.
  - Changed select module color to green instead of red. Green is consistent with other selections.
  - Made rack scrollbars even more transparent so that can see cables at bottom of screen better.
  - Fixed initialization so that Core and Fundamental modules actually show up in Browser window.

- 12/8/25
  - Now kind of runs on Raspberry Pi. But touchscreen commands don't really work.

- 11/11/25
  - Added splashscreen, with a png image even!
  
- 11/10/25
  - Added basic splash screen.Worked hard to make it appear as quickly as possible.

- 11/8/25
  - Improved spotlight and room lighting. Changed so that spotlight 
    will show up even if room lighting is bright. It will just be 
    pretty dim. And changed spotlight color so that it is a bit red,
    which looks nice. Made code clearer.
  - Highlighting knobs and sliders so it is really easy to see which 
    knob or slider is being manipulated. Really useful yet simple enhancement.

- 11/6/25
  - Relative small UI change of preventing cables from drooping below where they are fully visible. This way can better see which ports are connected together. But this required quite a lot of code cleanup.
  - Added random factor slider for cable tension so that there can be some natural variability in tension for the cables. It is hoped that this makes the cables look a bit more natural.

- 10/26/25
  - Big changes to how fps and cpu displayed in menu bar. Changed code to determine the values accurately while using limited processing. And changed what is displayed so that the info is more useful. Now displaying fps, potential fps, and cpu %.
  - Fixed updating of plugins. Previously system would always say that Bastl plugin should be updated, when actually the problem was that the plugin was never successfully loaded in the first place and now is no longer available.
  - Added ERROR logging. Not sure why that did not previously exist!

- 10/22/25 - was hella sick for many weeks
  - Major code cleanup.
    - Added comments when could.
    - Reformatted code to follow google style. 
    - Changed from using APP and accessing members directly. Now access goes through methods, which means that can add breakpoints or debug statements.
    - Changed math.hpp Vec and Rect classes so that the members are not access directly, but instead by methods.  While this change was big, it really does make the code more like proper c++
    - Changed use of Internal structure in all classes so that it is less accessible. Made it private and changed member name to internal_ to make it clear it is a member. 
    - Cleaned up initialization of app.

- 9/13/25
  - Big refactoring of Browser and TextWidget
    - Now have control over colors and font size used for TextField
    - For single line TextField it limits user to text that will on line.
    - Greatly cleaned up code to make it more C++ like. Added lots of comments.

- 9/6/25
  - Can now use any font face for a label. Nice so that can make a header label bold.
  - Tooltips no positioned out of the way of a finger if using a touch screen. Also, finished cleaning up of tooltip related code.

- 9/5/25
  - Improved tooltips.
    - Separate color to distinguish them from clickable items like menus. The value is in settings. Picked nice dark gray.
    - Separate and smaller font since can get a good amount of text and the user doesn't need to click on parts of it. The value is in settings. 
    - Got outline of tooltip to actually be noticeable
    - Got text to be centered perfectly within the box
    - Limited width so that don't get absurdly wide tooltips
    - Not displaying "Hardware clone" type since that doesn't seem to be really useful to user and the list of types/tags can be overly long. 
  - Major code cleanup. Refactored a lot of the cringy UI related structs and converted them to classes. And cleaned up and commented a lot of code. This will make future changes easier to implement.
  - Changed "Tags" to "Types" in Browser window in en.json. Seemed bit unfriendly to user CS term for a music device. Know your audience!

- 9/1/25
  - Scrollbar colors and opacity changed so that they are much more visible

- 8/31/25
  - Rewrite of SequentialLayout so that it has additional features
    - Can now evenly space items so that they take full width. Great for displaying modules in Browser.
    - Changed Browser header so that buttons are divided evenly between two rows, which looks significantly better.
    - Code more C++ like and actually commented
  - Many changes to module browser window
    - Changed margins and other things in module browser window to make it look significantly better.
    - Improved and simplified zoom code
    - Code more C++ like and actually commented
  - Can set more colors via settings. This way don't want to figure out how colors are set in seemingly random places in the code.

- 8/19/25 
  - Can use larger font for Module Browser header. Will work better on a touch screen.

- 8/18/25
  - Increased contrast between Module Browser and the rack below. This way it is easier for user to understand the context, that the Browser Window is in view but that the separate Rack window is still there, but below and inactive, yet accessible. Also added comments to code to make future changes easier.
  - Added title to the Module Browser window to explain that this window is used to add modules to one's rack.
  - Fixed Label so that can center text even when different font is used.
  - Changed color of the Module Browser window to be medium grey. This color can be used for both light and dark modes. And it is good to look different from the rest of the app so user immediately understands where they are. And looks nice. Also, made some improvements to the border around the Module Browser window.

- 8/17/25
  - Differentiated MenuLabels so that they look different (usually darker background) than disabled buttons. This really helps reduce confusion about what the menu elements.

- 8/16/25
  - Created kludge so that RACK_VERSION set to reasonable value via Makefile even when VSCode or other IDE being used. This is important so that updating plugin libraries can work.
  - Improved menus to make it clear how to add a module, what libraries were about, and how to take action on a module. Should make learning about those functionalities much easier.
  - Improved what was the Library menu. Previously was difficult for user to understand how to add a module since had to right-click on background, something there were no visual hints for. So change the Library menu to 'Add Module" and added a "Add module to rack" button to make things clear and easy. Also changed names and ordering of the VCV rack items to make it clear they are for the different VCV Rack library.

- 8/15/25
  - Improved help menu so doesn't show VCV rack version info when running branch
  - Improved menus by getting text centered and margins just right. Also commented some code since the uncommented original was hard to understand.
  - Improved some wording for menu labels to make things more clear.

- 8/14/25
  - If window should be full screen mode, which is default for Liminal Rack, then it is done automatically at startup. This way seems less like a computer application and more like a dedicated tool.

- 8/13/25
  - For Liminal, when zoom to modules now making modules as large as possible instead of adding a 24 unit margin. Important since using smaller screen for Liminal.
  - Improved cable shadows so there is a light one for dark panels. Provides a more consistent look that is nice.
  - For when connecting a cable to ports, the ports that can be used are now colored depending on whether they are input or output ports. Input ports are colored green, as before. But output ports are colored yellow/gold. Didn't use red for output ports since want things to look good for Instruo modules.
  - Logging changed so can set logging level and only have DEBUG statements output if debug level enabled. Can enable debug level via standalone.cpp by using command line option -b.
  - When creating cable system determines best color. Uses info from the ports and modules to try to see what kind of signal will be handled, like CV, pitch, or gate. And then looks at the cable lables to determine the corresponding cable. If system cannot make a match then it simply uses default color. Hopefully makes it so that without user having to specify colors the functionality of the cables will be quite organized.

- 8/11/25
  - Reduced droopiness of cables. Previously the cables were absurdly droopy when tension set to 0.
  - Improved drawing of cable plugs so that they clearly show which is an input and which is an output, and also which way the signal flows.
  - Improved display of fps and cpu in menu bar

- 8/10/25
  - Improved drawing of ports when dragging a cable. Previously the ports that couldn't connect to were really grayed out. But drawing them really dark drew attention to the ports that couldn't connect to instead of to the ones that could. So for ports where can't connect just fading out the ports by setting alpha to 0.4. And for the ports where can make a connection tinted them green so it is obvious that can connect to them.

- 8/9/25
  - Improved rendering of cables by having better default params and by reducing shadow droop so that shadow looks more associated with cable.

- 8/8/25
  - Slight improvements to CPU display in menu bar.
  - Got changing of font size for touch screen fully working

- 8/7/25
  - Changed tooltips to only display 2 digit precision for floating point numbers
  - Greatly pruned down sample rate choices because there were so many of them
  - Changed oui-blendish so that text for slider menu items is left justified instead of centered so that the menus with sliders look better and aligned.
  - Pruned down unnecessary menu items so that users aren't overwhelmed
  - Increased width of menu separators so that they are more visible
  - Turned off tipsOnLaunch

- 8/6/25
  - Can use larger font for menus
  - Got rid of keyboard shortcuts in menus for when no keyboard being used (touch screen)
  - Improved scroll widgets in menus so that text doesn't jiggle. Did this by not displaying fractional values.
  - Eliminated more compiler error/warning messages

- 8/4/25
  - Eliminated compiler error/warning messages
